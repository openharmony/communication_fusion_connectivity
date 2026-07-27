/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LOG_TAG
#define LOG_TAG "AdvertiserManagerTest"
#endif

#include <gtest/gtest.h>

#include "advertiser_manager.cpp"
#include "fcm_thread_util.h"
#include "fusion_ranging_errorcode.h"
#include "log_utils.h"

using namespace OHOS;
using namespace OHOS::FusionRanging;
using namespace OHOS::Nearlink;
using namespace testing::ext;

class MockSleAdvertiseCallback : public SleAdvertiseCallback {
public:
    void OnStartResultEvent(int result, int advHandle) override {}
    void OnEnableResultEvent(int result, int advHandle) override {}
    void OnDisableResultEvent(int result, int advHandle) override {}
    void OnStopResultEvent(int result, int advHandle) override {}
    void OnGetAdvHandleEvent(int result, int advHandle) override {}
    void OnSetAdvDataEvent(int result) override {}
};

class AdvertiserManagerTest : public testing::Test {
public:
    AdvertiserManagerTest() = default;
    ~AdvertiserManagerTest() override = default;

    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void AdvertiserManagerTest::SetUpTestCase(void)
{
    OHOS::FusionConnectivity::FcmThreadUtil::GetInstance().InitThreadStateMap();
}

void AdvertiserManagerTest::TearDownTestCase(void)
{
    OHOS::FusionConnectivity::FcmThreadUtil::GetInstance().ClearThreadStateMap();
}

void AdvertiserManagerTest::SetUp(void) {}

void AdvertiserManagerTest::TearDown(void)
{
    AdvertiserManager::GetInstance()->pimpl->advInfos_.Clear();
}

/*
 * Feature: AdvertiserManager
 * SubFeature: Singleton Pattern & IsPassiveRangingActive
 * Function: GetInstance / IsPassiveRangingActive
 * Condition: No active advertising
 * Test Steps: Call GetInstance twice, call IsPassiveRangingActive directly
 * Expected Result: Same instance; IsPassiveRangingActive returns false (advInfos_ empty)
 */
HWTEST_F(AdvertiserManagerTest, GetInstanceAndIsPassiveRangingActive_WhenNoActiveAdv_ShouldReturnSingletonAndFalse,
         TestSize.Level0)
{
    auto *instance1 = AdvertiserManager::GetInstance();
    auto *instance2 = AdvertiserManager::GetInstance();
    EXPECT_EQ(instance1, instance2);
    EXPECT_NE(instance1, nullptr);

    EXPECT_EQ(instance1->IsPassiveRangingActive(), false);
}

/*
 * Feature: AdvertiserManager
 * SubFeature: StartPassiveRanging
 * Function: StartPassiveRanging
 * Condition: Nearlink SDK not available or init fails
 * Test Steps: Call StartPassiveRanging
 * Expected Result: Returns error (Nearlink SDK internal failure)
 */
HWTEST_F(AdvertiserManagerTest, StartPassiveRanging_WhenSdkFails_ShouldReturnError, TestSize.Level1)
{
    auto *manager = AdvertiserManager::GetInstance();
    ASSERT_NE(manager, nullptr);
    EXPECT_EQ(manager->IsPassiveRangingActive(), false);

    int32_t handle = -1;
    int ret = manager->StartPassiveRanging(handle);
    EXPECT_EQ(ret, RANGING_ERR_OPERATION_FAILED);
    EXPECT_EQ(manager->IsPassiveRangingActive(), false);
}

/*
 * Feature: AdvertiserManager
 * SubFeature: StartPassiveRanging
 * Function: StartPassiveRanging -> IsPassiveRangingActive guard
 * Condition: Manually insert advInfo to simulate "already active"
 * Test Steps: Insert a fake AdvInfos into pimpl->advInfos_, then call StartPassiveRanging
 * Expected Result: Returns RANGING_ERR_OPERATION_FAILED (already active path)
 */
HWTEST_F(AdvertiserManagerTest, StartPassiveRanging_WhenAlreadyActive_ShouldReturnError, TestSize.Level1)
{
    auto *manager = AdvertiserManager::GetInstance();
    ASSERT_NE(manager, nullptr);
    ASSERT_NE(manager->pimpl, nullptr);

    EXPECT_EQ(manager->pimpl->advInfos_.Size(), 0u);
    auto callback = std::make_shared<MockSleAdvertiseCallback>();
    auto advInfo = std::make_shared<AdvInfos>(0, callback);
    manager->pimpl->advInfos_.EnsureInsert(0, advInfo);
    EXPECT_EQ(manager->IsPassiveRangingActive(), true);

    int32_t handle = -1;
    int ret = manager->StartPassiveRanging(handle);
    EXPECT_EQ(ret, RANGING_ERR_OPERATION_FAILED);

    manager->pimpl->advInfos_.Clear();
    EXPECT_EQ(manager->IsPassiveRangingActive(), false);
}

/*
 * Feature: AdvertiserManager
 * SubFeature: StopPassiveRanging
 * Function: StopPassiveRanging
 * Condition: Invalid handle
 * Test Steps: Call StopPassiveRanging with non-existent handle
 * Expected Result: Return error (handle not found in advInfos_)
 */
HWTEST_F(AdvertiserManagerTest, StopPassiveRanging_WithInvalidHandle_ShouldReturnError, TestSize.Level1)
{
    auto *manager = AdvertiserManager::GetInstance();
    ASSERT_NE(manager, nullptr);
    ASSERT_NE(manager->pimpl, nullptr);
    EXPECT_EQ(manager->pimpl->advInfos_.Size(), 0u);

    int32_t invalidHandle = 0xFF;
    int ret = manager->StopPassiveRanging(invalidHandle);
    EXPECT_EQ(ret, RANGING_ERR_OPERATION_FAILED);
    EXPECT_EQ(manager->pimpl->advInfos_.Size(), 0u);
}

/*
 * Feature: AdvertiserManager
 * SubFeature: StopPassiveRanging
 * Function: StopPassiveRanging -> StopSsapServer -> StopAdvertising
 * Condition: advInfos_ has entry but advertiser_/ssapServer are null
 * Test Steps: Insert fake AdvInfos with null ssapServer, call StopPassiveRanging
 * Expected Result: Returns error (StopSsapServer fails on null server), advInfos_ erased
 */
HWTEST_F(AdvertiserManagerTest, StopPassiveRanging_WithFakeAdvInfo_ShouldCleanUp, TestSize.Level1)
{
    auto *manager = AdvertiserManager::GetInstance();
    ASSERT_NE(manager, nullptr);
    ASSERT_NE(manager->pimpl, nullptr);

    int32_t handle = 5;
    auto callback = std::make_shared<MockSleAdvertiseCallback>();
    auto advInfo = std::make_shared<AdvInfos>(handle, callback);
    manager->pimpl->advInfos_.EnsureInsert(handle, advInfo);
    EXPECT_EQ(manager->pimpl->advInfos_.Size(), 1u);

    int ret = manager->StopPassiveRanging(handle);
    EXPECT_NE(ret, RANGING_NO_ERROR);
    EXPECT_EQ(manager->pimpl->advInfos_.Size(), 0u);
}

/*
 * Feature: AdvertiserManager
 * SubFeature: StopSsapServer (private)
 * Function: StopSsapServer
 * Condition: Handle not found in advInfos_
 * Test Steps: Call StopSsapServer with invalid handle
 * Expected Result: Return RANGING_ERR_OPERATION_FAILED
 */
HWTEST_F(AdvertiserManagerTest, StopSsapServer_WithInvalidHandle_ShouldReturnError, TestSize.Level1)
{
    auto *manager = AdvertiserManager::GetInstance();
    EXPECT_EQ(manager->IsPassiveRangingActive(), false);

    int32_t handle = 0;
    int ret = manager->StopSsapServer(handle);
    EXPECT_EQ(ret, RANGING_ERR_OPERATION_FAILED);
}

/*
 * Feature: AdvertiserManager
 * SubFeature: StopSsapServer (private)
 * Function: StopSsapServer
 * Condition: AdvInfo exists but ssapServer is null
 * Test Steps: Insert AdvInfos with null ssapServer, call StopSsapServer
 * Expected Result: Return RANGING_ERR_OPERATION_FAILED (server nullptr)
 */
HWTEST_F(AdvertiserManagerTest, StopSsapServer_WithNullSsapServer_ShouldReturnError, TestSize.Level1)
{
    auto *manager = AdvertiserManager::GetInstance();
    ASSERT_NE(manager, nullptr);
    ASSERT_NE(manager->pimpl, nullptr);

    int32_t handle = 10;
    auto callback = std::make_shared<MockSleAdvertiseCallback>();
    auto advInfo = std::make_shared<AdvInfos>(handle, callback);
    manager->pimpl->advInfos_.EnsureInsert(handle, advInfo);

    int ret = manager->StopSsapServer(handle);
    EXPECT_EQ(ret, RANGING_ERR_OPERATION_FAILED);

    manager->pimpl->advInfos_.Erase(handle);
}

/*
 * Feature: AdvertiserManager
 * SubFeature: StopAdvertising (private)
 * Function: StopAdvertising
 * Condition: advertiser_ is not null but advHandle not in advInfos_
 * Test Steps: Call StopAdvertising with non-existent handle
 * Expected Result: Return RANGING_ERR_OPERATION_FAILED (not found adv)
 */
HWTEST_F(AdvertiserManagerTest, StopAdvertising_WithInvalidHandle_ShouldReturnError, TestSize.Level1)
{
    auto *manager = AdvertiserManager::GetInstance();
    ASSERT_NE(manager, nullptr);
    ASSERT_NE(manager->pimpl, nullptr);
    ASSERT_NE(manager->pimpl->advertiser_, nullptr);

    int32_t invalidHandle = 999;
    int ret = manager->StopAdvertising(invalidHandle);
    EXPECT_EQ(ret, RANGING_ERR_OPERATION_FAILED);
}

/*
 * Feature: AdvertiserManager
 * SubFeature: StartAdvertisingInternal (private)
 * Function: StartAdvertisingInternal
 * Condition: Nearlink SDK StartAdvertising fails
 * Test Steps: Call StartAdvertisingInternal
 * Expected Result: Returns RANGING_ERR_OPERATION_FAILED (SDK call fails)
 */
HWTEST_F(AdvertiserManagerTest, StartAdvertisingInternal_WhenSdkUnavailable_ShouldReturnError, TestSize.Level1)
{
    auto *manager = AdvertiserManager::GetInstance();
    ASSERT_NE(manager, nullptr);

    int32_t advHandle = -1;
    int ret = manager->StartAdvertisingInternal(advHandle);
    EXPECT_EQ(ret, RANGING_ERR_OPERATION_FAILED);
}

/*
 * Feature: AdvertiserManager
 * SubFeature: StartSsapServerInternal (private)
 * Function: StartSsapServerInternal
 * Condition: advHandle not found in advInfos_
 * Test Steps: Call StartSsapServerInternal with non-existent handle
 * Expected Result: Returns RANGING_ERR_OPERATION_FAILED (null advInfo)
 */
HWTEST_F(AdvertiserManagerTest, StartSsapServerInternal_WithInvalidHandle_ShouldReturnError, TestSize.Level1)
{
    auto *manager = AdvertiserManager::GetInstance();
    ASSERT_NE(manager, nullptr);

    int32_t invalidHandle = 888;
    int ret = manager->StartSsapServerInternal(invalidHandle);
    EXPECT_EQ(ret, RANGING_ERR_OPERATION_FAILED);
}

/*
 * Feature: AdvertiserManager
 * SubFeature: OnSsapConnectionStateUpdate
 * Function: OnSsapConnectionStateUpdate
 * Condition: Unknown handle and disconnected state
 * Test Steps: Call OnSsapConnectionStateUpdate with unknown handle, then with disconnected state
 * Expected Result: Both handle gracefully (FCM_CHECK_RETURN on null advInfo)
 */
HWTEST_F(AdvertiserManagerTest, OnSsapConnectionStateUpdate_WithUnknownHandleAndDisconnected_ShouldNotCrash,
         TestSize.Level1)
{
    auto *manager = AdvertiserManager::GetInstance();
    ASSERT_NE(manager, nullptr);
    std::string deviceId = "11:22:33:44:55:66";

    manager->OnSsapConnectionStateUpdate(deviceId, 999, 0, 0);
    SUCCEED();

    manager->OnSsapConnectionStateUpdate(deviceId, 0, 0, 0);
    SUCCEED();
}

/*
 * Feature: AdvertiserManager
 * SubFeature: OnAdvStateChanged
 * Function: OnAdvStateChanged
 * Condition: STARTED state
 * Test Steps: Call OnAdvStateChanged with ADV_STATE_STARTED, check pimpl->advHandle_ updated
 * Expected Result: advHandle_ set to the passed handle
 */
HWTEST_F(AdvertiserManagerTest, OnAdvStateChanged_WithStartedState_ShouldUpdateAdvHandle, TestSize.Level1)
{
    auto *manager = AdvertiserManager::GetInstance();
    ASSERT_NE(manager, nullptr);
    ASSERT_NE(manager->pimpl, nullptr);

    int32_t testHandle = 42;
    manager->pimpl->advHandle_ = -1;
    manager->OnAdvStateChanged(testHandle, ADV_STATE_STARTED);
    EXPECT_EQ(static_cast<int32_t>(manager->pimpl->advHandle_.load()), testHandle);
}

/*
 * Feature: AdvertiserManager
 * SubFeature: OnAdvStateChanged
 * Function: OnAdvStateChanged
 * Condition: STOPPED state
 * Test Steps: Call OnAdvStateChanged with ADV_STATE_STOPPED, verify advHandle_ unchanged
 * Expected Result: advHandle_ not modified (only STARTED updates it)
 */
HWTEST_F(AdvertiserManagerTest, OnAdvStateChanged_WithStoppedState_ShouldNotUpdateAdvHandle, TestSize.Level1)
{
    auto *manager = AdvertiserManager::GetInstance();
    ASSERT_NE(manager, nullptr);
    ASSERT_NE(manager->pimpl, nullptr);

    int32_t prevHandle = 99;
    manager->pimpl->advHandle_ = prevHandle;
    manager->OnAdvStateChanged(0, ADV_STATE_STOPPED);
    EXPECT_EQ(static_cast<int32_t>(manager->pimpl->advHandle_.load()), prevHandle);
}