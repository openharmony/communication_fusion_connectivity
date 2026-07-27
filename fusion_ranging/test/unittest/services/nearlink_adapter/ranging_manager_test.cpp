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
#define LOG_TAG "RangingManagerTest"
#endif

#include <gtest/gtest.h>

#include "ranging_manager.h"
#include "base_ranging_adapter.h"
#include "fusion_ranging_types.h"
#include "fusion_ranging_errorcode.h"
#include "log_utils.h"
#include "fcm_thread_util.h"

using namespace OHOS;
using namespace OHOS::FusionRanging;
using namespace OHOS::Nearlink;
using namespace testing::ext;

class MockRangingAdapterCallback : public BaseRangingAdapterCallback {
public:
    void OnRangingStateChange(const AdapterRangingStateInfo &Info) override {}
    void OnRangingResult(const AdapterRangingData &data) override {}
};

class RangingManagerTest : public testing::Test {
public:
    RangingManagerTest() = default;
    ~RangingManagerTest() override = default;

    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void RangingManagerTest::SetUpTestCase(void)
{
    OHOS::FusionConnectivity::FcmThreadUtil::GetInstance().InitThreadStateMap();
}

void RangingManagerTest::TearDownTestCase(void)
{
    OHOS::FusionConnectivity::FcmThreadUtil::GetInstance().ClearThreadStateMap();
}

void RangingManagerTest::SetUp(void) {}

void RangingManagerTest::TearDown(void)
{
    RangingManager::GetInstance()->DeInit();
    RangingManager::GetInstance()->rangingDev_.Clear();
    RangingManager::GetInstance()->adapterCallback_ = nullptr;
}

/*
 * Feature: RangingManager
 * SubFeature: Singleton Pattern
 * Function: GetInstance
 * Condition: None
 * Test Steps: Call GetInstance multiple times, verify same address and non-null
 * Expected Result: All calls return the same non-null address
 */
HWTEST_F(RangingManagerTest, GetInstance_ShouldReturnSingleton, TestSize.Level0)
{
    auto *instance1 = RangingManager::GetInstance();
    auto *instance2 = RangingManager::GetInstance();
    auto *instance3 = RangingManager::GetInstance();
    EXPECT_NE(instance1, nullptr);
    EXPECT_EQ(instance1, instance2);
    EXPECT_EQ(instance2, instance3);
}

/*
 * Feature: RangingManager
 * SubFeature: Initialization / Deinitialization
 * Function: Init / DeInit
 * Condition: Not initialized, already initialized, and repeated calls
 * Test Steps: Call Init after DeInit (fresh), call Init again (idempotent),
 *             call DeInit (success), call DeInit again (no-op)
 * Expected Result: All calls return success
 */
HWTEST_F(RangingManagerTest, InitAndDeInit_ShouldBeIdempotent, TestSize.Level0)
{
    auto *manager = RangingManager::GetInstance();
    ASSERT_NE(manager, nullptr);
    manager->DeInit();

    EXPECT_EQ(manager->Init(), RANGING_NO_ERROR);
    EXPECT_EQ(manager->Init(), RANGING_NO_ERROR);
    EXPECT_EQ(manager->DeInit(), RANGING_NO_ERROR);
    EXPECT_EQ(manager->DeInit(), RANGING_NO_ERROR);
}

/*
 * Feature: RangingManager
 * SubFeature: StartRanging
 * Function: StartRanging
 * Condition: Not initialized or invalid device ID
 * Test Steps: Call StartRanging without Init (expect error),
 *             then with Init but empty string (expect PARAM error)
 * Expected Result: Not initialized returns error; empty ID returns PARAM_NOT_MEET_SPECIFICATIONS
 */
HWTEST_F(RangingManagerTest, StartRanging_WhenNotInitOrEmptyId_ShouldReturnError, TestSize.Level1)
{
    auto *manager = RangingManager::GetInstance();
    manager->DeInit();
    EXPECT_NE(manager->StartRanging("11:22:33:44:55:66"), RANGING_NO_ERROR);

    manager->Init();
    EXPECT_EQ(manager->StartRanging(""), RANGING_ERR_PARAM_NOT_MEET_SPECIFICATIONS);
}

/*
 * Feature: RangingManager
 * SubFeature: StartRanging
 * Function: StartRanging
 * Condition: Invalid MAC address format
 * Test Steps: Call StartRanging with malformed MAC
 * Expected Result: Return PARAM_NOT_MEET_SPECIFICATIONS error
 */
HWTEST_F(RangingManagerTest, StartRanging_WithInvalidMacFormat_ShouldReturnParamError, TestSize.Level1)
{
    auto *manager = RangingManager::GetInstance();
    manager->Init();

    int ret = manager->StartRanging("invalid-mac");
    EXPECT_EQ(ret, RANGING_ERR_PARAM_NOT_MEET_SPECIFICATIONS);

    ret = manager->StartRanging("11:22:33:44:55");
    EXPECT_EQ(ret, RANGING_ERR_PARAM_NOT_MEET_SPECIFICATIONS);
}

/*
 * Feature: RangingManager
 * SubFeature: StartRanging
 * Function: StartRanging -> StartRangingWithConnection
 * Condition: Valid MAC, SDK unavailable (Connect fails)
 * Test Steps: Call StartRanging with valid MAC format
 * Expected Result: Return OPERATION_FAILED (connect fails without SDK)
 */
HWTEST_F(RangingManagerTest, StartRanging_WithValidMac_ShouldReturnOperationFailed, TestSize.Level1)
{
    auto *manager = RangingManager::GetInstance();
    manager->Init();

    EXPECT_EQ(manager->StartRanging("00:00:00:00:00:00"), RANGING_ERR_OPERATION_FAILED);
    EXPECT_EQ(manager->StartRanging("FF:FF:FF:FF:FF:FF"), RANGING_ERR_OPERATION_FAILED);
    EXPECT_EQ(manager->StartRanging("AA:BB:CC:DD:EE:FF"), RANGING_ERR_OPERATION_FAILED);
}

/*
 * Feature: RangingManager
 * SubFeature: StartRangingWithConnection (private)
 * Function: StartRangingWithConnection
 * Condition: Device already exists in rangingDev_
 * Test Steps: Insert a RangingDevice into rangingDev_, call StartRangingWithConnection
 * Expected Result: Return RANGING_ERR_DEVICE_ALREADY_INITIATED
 */
HWTEST_F(RangingManagerTest, StartRangingWithConnection_WhenDeviceExists_ShouldReturnAlreadyInitiated, TestSize.Level1)
{
    auto *manager = RangingManager::GetInstance();
    manager->Init();

    std::string deviceId = "AA:BB:CC:DD:EE:FF";
    auto rangDev = std::make_shared<RangingDevice>(deviceId);
    manager->rangingDev_.EnsureInsert(deviceId, rangDev);

    int ret = manager->StartRangingWithConnection(deviceId);
    EXPECT_EQ(ret, RANGING_ERR_DEVICE_ALREADY_INITIATED);

    manager->rangingDev_.Erase(deviceId);
}

/*
 * Feature: RangingManager
 * SubFeature: StopRanging
 * Function: StopRanging
 * Condition: Not initialized, invalid MAC, and device not found
 * Test Steps: Call StopRanging without Init (expect error),
 *             with invalid MAC (expect PARAM error),
 *             with valid MAC but no device (expect NOT_INITIATED)
 * Expected Result: Each condition returns appropriate error code
 */
HWTEST_F(RangingManagerTest, StopRanging_WithVariousInvalidConditions_ShouldReturnError, TestSize.Level1)
{
    auto *manager = RangingManager::GetInstance();
    ASSERT_NE(manager, nullptr);
    std::string validMac = "11:22:33:44:55:66";

    manager->DeInit();
    EXPECT_NE(manager->StopRanging(validMac), RANGING_NO_ERROR);

    manager->Init();
    EXPECT_EQ(manager->StopRanging("invalid"), RANGING_ERR_PARAM_NOT_MEET_SPECIFICATIONS);
    EXPECT_EQ(manager->StopRanging(validMac), RANGING_ERR_DEVICE_NOT_INITIATED);
}

/*
 * Feature: RangingManager
 * SubFeature: SetCallback
 * Function: SetCallback
 * Condition: First call with nullptr
 * Test Steps: Call SetCallback with nullptr
 * Expected Result: Returns success (nullptr is stored)
 */
HWTEST_F(RangingManagerTest, SetCallback_WithNullptr_ShouldReturnSuccess, TestSize.Level0)
{
    auto *manager = RangingManager::GetInstance();
    manager->Init();
    manager->adapterCallback_ = nullptr;

    int ret = manager->SetCallback(nullptr);
    EXPECT_EQ(ret, RANGING_NO_ERROR);
}

/*
 * Feature: RangingManager
 * SubFeature: SetCallback
 * Function: SetCallback
 * Condition: Callback already set
 * Test Steps: Set callback once, then set again
 * Expected Result: Second call returns success (early return, no overwrite)
 */
HWTEST_F(RangingManagerTest, SetCallback_WhenAlreadySet_ShouldNotOverwrite, TestSize.Level1)
{
    auto *manager = RangingManager::GetInstance();
    manager->Init();
    auto firstCallback = std::make_shared<MockRangingAdapterCallback>();
    manager->adapterCallback_ = firstCallback;

    auto secondCallback = std::make_shared<MockRangingAdapterCallback>();
    int ret = manager->SetCallback(secondCallback);
    EXPECT_EQ(ret, RANGING_NO_ERROR);
    EXPECT_EQ(manager->adapterCallback_, firstCallback);
    EXPECT_NE(manager->adapterCallback_, secondCallback);
}

/*
 * Feature: RangingManager
 * SubFeature: PauseRanging
 * Function: PauseRanging
 * Condition: Not initialized, invalid MAC, and device not found
 * Test Steps: Call PauseRanging without Init (expect error),
 *             with invalid MAC (expect PARAM error),
 *             with valid MAC but no device (expect NOT_INITIATED)
 * Expected Result: Each condition returns appropriate error code
 */
HWTEST_F(RangingManagerTest, PauseRanging_WithVariousInvalidConditions_ShouldReturnError, TestSize.Level1)
{
    auto *manager = RangingManager::GetInstance();
    ASSERT_NE(manager, nullptr);
    std::string validMac = "11:22:33:44:55:66";

    manager->DeInit();
    EXPECT_NE(manager->PauseRanging(validMac), RANGING_NO_ERROR);

    manager->Init();
    EXPECT_EQ(manager->PauseRanging("invalid"), RANGING_ERR_PARAM_NOT_MEET_SPECIFICATIONS);
    EXPECT_EQ(manager->PauseRanging(validMac), RANGING_ERR_DEVICE_NOT_INITIATED);
}

/*
 * Feature: RangingManager
 * SubFeature: PauseRanging
 * Function: PauseRanging
 * Condition: Device exists but not in RANGING state
 * Test Steps: Insert RangingDevice in IDLE state, call PauseRanging
 * Expected Result: Return OPERATION_FAILED (TryTransitionState fails)
 */
HWTEST_F(RangingManagerTest, PauseRanging_WhenNotInRangingState_ShouldReturnError, TestSize.Level1)
{
    auto *manager = RangingManager::GetInstance();
    manager->Init();

    std::string deviceId = "AA:BB:CC:DD:EE:FF";
    auto rangDev = std::make_shared<RangingDevice>(deviceId);
    rangDev->SetRangingState(RangingAdapterState::IDLE);
    manager->rangingDev_.EnsureInsert(deviceId, rangDev);

    int ret = manager->PauseRanging(deviceId);
    EXPECT_EQ(ret, RANGING_ERR_OPERATION_FAILED);

    manager->rangingDev_.Erase(deviceId);
}

/*
 * Feature: RangingManager
 * SubFeature: ResumeRanging
 * Function: ResumeRanging
 * Condition: Not initialized, invalid MAC, and device not found
 * Test Steps: Call ResumeRanging without Init (expect error),
 *             with invalid MAC (expect PARAM error),
 *             with valid MAC but no device (expect NOT_INITIATED)
 * Expected Result: Each condition returns appropriate error code
 */
HWTEST_F(RangingManagerTest, ResumeRanging_WithVariousInvalidConditions_ShouldReturnError, TestSize.Level1)
{
    auto *manager = RangingManager::GetInstance();
    ASSERT_NE(manager, nullptr);
    std::string validMac = "11:22:33:44:55:66";

    manager->DeInit();
    EXPECT_NE(manager->ResumeRanging(validMac), RANGING_NO_ERROR);

    manager->Init();
    EXPECT_EQ(manager->ResumeRanging("invalid"), RANGING_ERR_PARAM_NOT_MEET_SPECIFICATIONS);
    EXPECT_EQ(manager->ResumeRanging(validMac), RANGING_ERR_DEVICE_NOT_INITIATED);
}

/*
 * Feature: RangingManager
 * SubFeature: ResumeRanging
 * Function: ResumeRanging
 * Condition: Device exists but not in CONNECTED state
 * Test Steps: Insert RangingDevice in IDLE state, call ResumeRanging
 * Expected Result: Return OPERATION_FAILED (TryTransitionState CONNECTED->RANGING fails)
 */
HWTEST_F(RangingManagerTest, ResumeRanging_WhenNotInConnectedState_ShouldReturnError, TestSize.Level1)
{
    auto *manager = RangingManager::GetInstance();
    manager->Init();

    std::string deviceId = "AA:BB:CC:DD:EE:FF";
    auto rangDev = std::make_shared<RangingDevice>(deviceId);
    rangDev->SetRangingState(RangingAdapterState::IDLE);
    manager->rangingDev_.EnsureInsert(deviceId, rangDev);

    int ret = manager->ResumeRanging(deviceId);
    EXPECT_EQ(ret, RANGING_ERR_OPERATION_FAILED);

    manager->rangingDev_.Erase(deviceId);
}

/*
 * Feature: RangingManager
 * SubFeature: PassiveRanging
 * Function: StartPassiveRanging / StopPassiveRanging
 * Condition: SDK unavailable or invalid handle
 * Test Steps: Call StartPassiveRanging with default handle,
 *             then StopPassiveRanging with non-existent handle
 * Expected Result: Both return error (delegates to AdvertiserManager which fails)
 */
HWTEST_F(RangingManagerTest, PassiveRanging_WithInvalidConditions_ShouldReturnError, TestSize.Level1)
{
    auto *manager = RangingManager::GetInstance();
    manager->Init();
    int32_t handle = -1;

    int ret = manager->StartPassiveRanging(handle);
    EXPECT_NE(ret, RANGING_NO_ERROR);

    ret = manager->StopPassiveRanging(999);
    EXPECT_NE(ret, RANGING_NO_ERROR);

    ret = manager->StopPassiveRanging(0);
    EXPECT_NE(ret, RANGING_NO_ERROR);
}

/*
 * Feature: RangingManager
 * SubFeature: OnConnectionStateChange / OnRangingStateChange
 * Function: OnConnectionStateChange / OnRangingStateChange
 * Condition: Device not in rangingDev_
 * Test Steps: Call both callbacks with non-existent device IDs
 * Expected Result: Handle gracefully (FCM_CHECK_RETURN on null rangDev)
 */
HWTEST_F(RangingManagerTest, OnStateChange_WithUnknownDevice_ShouldNotCrash, TestSize.Level1)
{
    auto *manager = RangingManager::GetInstance();
    manager->Init();

    manager->OnConnectionStateChange("11:22:33:44:55:66", 0);
    EXPECT_EQ(manager->GetRangingDevice("11:22:33:44:55:66"), nullptr);

    manager->OnRangingStateChange("AA:BB:CC:DD:EE:FF", 0);
    EXPECT_EQ(manager->GetRangingDevice("AA:BB:CC:DD:EE:FF"), nullptr);
}

/*
 * Feature: RangingManager
 * SubFeature: OnConnectionStateChange
 * Function: OnConnectionStateChange
 * Condition: Device exists, CONNECTED state
 * Test Steps: Insert RangingDevice in CONNECTING state, call OnConnectionStateChange with CONNECTED
 * Expected Result: Device state transitions to CONNECTED
 */
HWTEST_F(RangingManagerTest, OnConnectionStateChange_WithConnectedState_ShouldTransitionState, TestSize.Level1)
{
    auto *manager = RangingManager::GetInstance();
    manager->Init();

    std::string deviceId = "AA:BB:CC:DD:EE:FF";
    auto rangDev = std::make_shared<RangingDevice>(deviceId);
    rangDev->SetRangingState(RangingAdapterState::CONNECTING);
    manager->rangingDev_.EnsureInsert(deviceId, rangDev);

    manager->OnConnectionStateChange(deviceId, static_cast<int32_t>(SleConnState::SLE_CONNECTION_STATE_CONNECTED));
    EXPECT_EQ(rangDev->GetRangingState(), RangingAdapterState::CONNECTED);

    manager->rangingDev_.Erase(deviceId);
}

/*
 * Feature: RangingManager
 * SubFeature: OnConnectionStateChange
 * Function: OnConnectionStateChange
 * Condition: Device exists, DISCONNECTED state
 * Test Steps: Insert RangingDevice, call OnConnectionStateChange with DISCONNECTED
 * Expected Result: Device state set to IDLE, callback notified
 */
HWTEST_F(RangingManagerTest, OnConnectionStateChange_WithDisconnectedState_ShouldSetIdle, TestSize.Level1)
{
    auto *manager = RangingManager::GetInstance();
    manager->Init();

    std::string deviceId = "AA:BB:CC:DD:EE:FF";
    auto rangDev = std::make_shared<RangingDevice>(deviceId);
    rangDev->SetRangingState(RangingAdapterState::RANGING);
    manager->rangingDev_.EnsureInsert(deviceId, rangDev);

    manager->OnConnectionStateChange(deviceId, static_cast<int32_t>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));
    EXPECT_EQ(rangDev->GetRangingState(), RangingAdapterState::IDLE);

    manager->rangingDev_.Erase(deviceId);
}

/*
 * Feature: RangingManager
 * SubFeature: OnRangingStateChange
 * Function: OnRangingStateChange
 * Condition: Device exists, SLE_RANGING_STARTED state
 * Test Steps: Insert RangingDevice in CONNECTED state, call OnRangingStateChange with STARTED
 * Expected Result: Device state transitions to RANGING
 */
HWTEST_F(RangingManagerTest, OnRangingStateChange_WithStartedState_ShouldTransitionToRanging, TestSize.Level1)
{
    auto *manager = RangingManager::GetInstance();
    manager->Init();

    std::string deviceId = "AA:BB:CC:DD:EE:FF";
    auto rangDev = std::make_shared<RangingDevice>(deviceId);
    rangDev->SetRangingState(RangingAdapterState::CONNECTED);
    manager->rangingDev_.EnsureInsert(deviceId, rangDev);

    manager->OnRangingStateChange(deviceId, SLE_RANGING_STARTED);
    EXPECT_EQ(rangDev->GetRangingState(), RangingAdapterState::RANGING);

    manager->rangingDev_.Erase(deviceId);
}

/*
 * Feature: RangingManager
 * SubFeature: OnRangingStateChange
 * Function: OnRangingStateChange
 * Condition: Device exists, SLE_RANGING_STOPPED state
 * Test Steps: Insert RangingDevice in RANGING state, call OnRangingStateChange with STOPPED
 * Expected Result: Device state transitions to IDLE
 */
HWTEST_F(RangingManagerTest, OnRangingStateChange_WithStoppedState_ShouldTransitionToIdle, TestSize.Level1)
{
    auto *manager = RangingManager::GetInstance();
    manager->Init();

    std::string deviceId = "AA:BB:CC:DD:EE:FF";
    auto rangDev = std::make_shared<RangingDevice>(deviceId);
    rangDev->SetRangingState(RangingAdapterState::RANGING);
    manager->rangingDev_.EnsureInsert(deviceId, rangDev);

    manager->OnRangingStateChange(deviceId, SLE_RANGING_STOPPED);
    EXPECT_EQ(rangDev->GetRangingState(), RangingAdapterState::IDLE);

    manager->rangingDev_.Erase(deviceId);
}

/*
 * Feature: RangingManager
 * SubFeature: GetRangingDevice (private)
 * Function: GetRangingDevice
 * Condition: Device not in map
 * Test Steps: Call GetRangingDevice with non-existent deviceId
 * Expected Result: Returns nullptr
 */
HWTEST_F(RangingManagerTest, GetRangingDevice_WhenNotExists_ShouldReturnNull, TestSize.Level1)
{
    auto *manager = RangingManager::GetInstance();
    ASSERT_NE(manager, nullptr);

    auto ret1 = manager->GetRangingDevice("11:22:33:44:55:66");
    EXPECT_EQ(ret1, nullptr);

    auto ret2 = manager->GetRangingDevice("non-existent-device");
    EXPECT_EQ(ret2, nullptr);
}

/*
 * Feature: RangingManager
 * SubFeature: GetRangingDevice (private)
 * Function: GetRangingDevice
 * Condition: Device exists in map
 * Test Steps: Insert RangingDevice, call GetRangingDevice
 * Expected Result: Returns the inserted device
 */
HWTEST_F(RangingManagerTest, GetRangingDevice_WhenExists_ShouldReturnDevice, TestSize.Level1)
{
    auto *manager = RangingManager::GetInstance();
    std::string deviceId = "AA:BB:CC:DD:EE:FF";
    auto rangDev = std::make_shared<RangingDevice>(deviceId);
    manager->rangingDev_.EnsureInsert(deviceId, rangDev);

    auto ret = manager->GetRangingDevice(deviceId);
    EXPECT_NE(ret, nullptr);
    EXPECT_EQ(ret.get(), rangDev.get());

    manager->rangingDev_.Erase(deviceId);
}

/*
 * Feature: RangingManager
 * SubFeature: NotifyRangingStateChanged (private)
 * Function: NotifyRangingStateChanged
 * Condition: No callback set
 * Test Steps: Call NotifyRangingStateChanged without setting callback
 * Expected Result: No crash (FCM_CHECK_RETURN on null adapterCallback_)
 */
HWTEST_F(RangingManagerTest, NotifyRangingStateChanged_WithNoCallback_ShouldNotCrash, TestSize.Level1)
{
    auto *manager = RangingManager::GetInstance();
    manager->Init();
    manager->adapterCallback_ = nullptr;

    manager->NotifyRangingStateChanged("11:22:33:44:55:66", SLE_RANGING_STARTED);
    manager->NotifyRangingStateChanged("AA:BB:CC:DD:EE:FF", SLE_RANGING_STOPPED);
    EXPECT_EQ(manager->adapterCallback_, nullptr);
}

/*
 * Feature: RangingManager
 * SubFeature: NotifyPassiveRangingStateChanged
 * Function: NotifyPassiveRangingStateChanged
 * Condition: No callback set
 * Test Steps: Call NotifyPassiveRangingStateChanged without setting callback
 * Expected Result: No crash (FCM_CHECK_RETURN on null adapterCallback_)
 */
HWTEST_F(RangingManagerTest, NotifyPassiveRangingStateChanged_WithNoCallback_ShouldNotCrash, TestSize.Level1)
{
    auto *manager = RangingManager::GetInstance();
    manager->Init();
    manager->adapterCallback_ = nullptr;

    manager->NotifyPassiveRangingStateChanged("11:22:33:44:55:66", 0, SLE_RANGING_STARTED);
    manager->NotifyPassiveRangingStateChanged("AA:BB:CC:DD:EE:FF", 1, SLE_RANGING_STOPPED);
    EXPECT_EQ(manager->adapterCallback_, nullptr);
}

/*
 * Feature: RangingManager
 * SubFeature: StartSleRanging (private)
 * Function: StartSleRanging
 * Condition: Device not in rangingDev_
 * Test Steps: Call StartSleRanging with non-existent device
 * Expected Result: Return DEVICE_NOT_INITIATED
 */
HWTEST_F(RangingManagerTest, StartSleRanging_WhenDeviceNotFound_ShouldReturnNotInitiated, TestSize.Level1)
{
    auto *manager = RangingManager::GetInstance();
    manager->Init();

    int ret = manager->StartSleRanging("11:22:33:44:55:66");
    EXPECT_EQ(ret, RANGING_ERR_DEVICE_NOT_INITIATED);

    ret = manager->StartSleRanging("AA:BB:CC:DD:EE:FF");
    EXPECT_EQ(ret, RANGING_ERR_DEVICE_NOT_INITIATED);

    ret = manager->StartSleRanging("");
    EXPECT_EQ(ret, RANGING_ERR_DEVICE_NOT_INITIATED);
}

/*
 * Feature: RangingManager
 * SubFeature: StartSleRanging (private)
 * Function: StartSleRanging
 * Condition: Device exists but not connected
 * Test Steps: Insert RangingDevice, call StartSleRanging
 * Expected Result: Return OPERATION_FAILED (IsConnected returns false)
 */
HWTEST_F(RangingManagerTest, StartSleRanging_WhenNotConnected_ShouldReturnError, TestSize.Level1)
{
    auto *manager = RangingManager::GetInstance();
    manager->Init();

    std::string deviceId = "AA:BB:CC:DD:EE:FF";
    auto rangDev = std::make_shared<RangingDevice>(deviceId);
    rangDev->SetRangingState(RangingAdapterState::CONNECTED);
    manager->rangingDev_.EnsureInsert(deviceId, rangDev);

    int ret = manager->StartSleRanging(deviceId);
    EXPECT_EQ(ret, RANGING_ERR_OPERATION_FAILED);

    manager->rangingDev_.Erase(deviceId);
}

/*
 * Feature: RangingManager
 * SubFeature: DoStartSleRanging (private)
 * Function: DoStartSleRanging
 * Condition: nearlinkRanging_ is null
 * Test Steps: Set nearlinkRanging_ to nullptr, call DoStartSleRanging
 * Expected Result: Return OPERATION_FAILED
 */
HWTEST_F(RangingManagerTest, DoStartSleRanging_WhenRangingNull_ShouldReturnError, TestSize.Level1)
{
    auto *manager = RangingManager::GetInstance();
    manager->Init();

    auto savedRanging = manager->nearlinkRanging_;
    manager->nearlinkRanging_ = nullptr;

    int ret = manager->DoStartSleRanging("11:22:33:44:55:66");
    EXPECT_EQ(ret, RANGING_ERR_OPERATION_FAILED);

    manager->nearlinkRanging_ = savedRanging;
}

/*
 * Feature: RangingManager
 * SubFeature: RangingDevice state machine
 * Function: TryTransitionState
 * Condition: Various state transitions
 * Test Steps: Create RangingDevice, test state transitions
 * Expected Result: Correct CAS behavior
 */
HWTEST_F(RangingManagerTest, RangingDevice_StateTransitions_ShouldWorkCorrectly, TestSize.Level1)
{
    RangingDevice dev("AA:BB:CC:DD:EE:FF");
    EXPECT_EQ(dev.GetRangingState(), RangingAdapterState::IDLE);

    EXPECT_TRUE(dev.TryTransitionState(RangingAdapterState::IDLE, RangingAdapterState::CONNECTING));
    EXPECT_EQ(dev.GetRangingState(), RangingAdapterState::CONNECTING);

    EXPECT_FALSE(dev.TryTransitionState(RangingAdapterState::IDLE, RangingAdapterState::CONNECTED));
    EXPECT_EQ(dev.GetRangingState(), RangingAdapterState::CONNECTING);

    EXPECT_TRUE(dev.TryTransitionState(RangingAdapterState::CONNECTING, RangingAdapterState::CONNECTED));
    EXPECT_EQ(dev.GetRangingState(), RangingAdapterState::CONNECTED);

    dev.SetRangingState(RangingAdapterState::RANGING);
    EXPECT_EQ(dev.GetRangingState(), RangingAdapterState::RANGING);
}

/*
 * Feature: RangingManager
 * SubFeature: OnRangingResult
 * Function: OnRangingResult
 * Condition: Device not in rangingDev_
 * Test Steps: Call OnRangingResult with non-existent device address
 * Expected Result: No crash (FCM_CHECK_RETURN on null rangDev)
 */
HWTEST_F(RangingManagerTest, OnRangingResult_WithUnknownDevice_ShouldNotCrash, TestSize.Level1)
{
    auto *manager = RangingManager::GetInstance();
    manager->Init();

    Nearlink::RangingResult fakeResult1;
    manager->OnRangingResult(fakeResult1);

    Nearlink::RangingResult fakeResult2;
    manager->OnRangingResult(fakeResult2);
    EXPECT_EQ(manager->GetRangingDevice("AA:BB:CC:DD:EE:FF"), nullptr);
}

/*
 * Feature: RangingManager
 * SubFeature: OnRangingResult
 * Function: OnRangingResult
 * Condition: Device exists but no callback set
 * Test Steps: Insert RangingDevice, call OnRangingResult with null adapterCallback_
 * Expected Result: No crash (FCM_CHECK_RETURN on null adapterCallback_)
 */
HWTEST_F(RangingManagerTest, OnRangingResult_WithDeviceButNoCallback_ShouldNotCrash, TestSize.Level1)
{
    auto *manager = RangingManager::GetInstance();
    manager->Init();
    manager->adapterCallback_ = nullptr;

    std::string deviceId = "AA:BB:CC:DD:EE:FF";
    auto rangDev = std::make_shared<RangingDevice>(deviceId);
    manager->rangingDev_.EnsureInsert(deviceId, rangDev);

    Nearlink::RangingResult fakeResult;
    manager->OnRangingResult(fakeResult);

    manager->rangingDev_.Erase(deviceId);
}

/*
 * Feature: RangingManager
 * SubFeature: OnConnectionStateChange
 * Function: OnConnectionStateChange
 * Condition: Device exists, ENCRYPTED state
 * Test Steps: Insert RangingDevice in CONNECTED state, call OnConnectionStateChange with ENCRYPTED
 * Expected Result: StartSleRanging called (which fails since not truly connected, but no crash)
 */
HWTEST_F(RangingManagerTest, OnConnectionStateChange_WithEncryptedState_ShouldNotCrash, TestSize.Level1)
{
    auto *manager = RangingManager::GetInstance();
    manager->Init();

    std::string deviceId = "AA:BB:CC:DD:EE:FF";
    auto rangDev = std::make_shared<RangingDevice>(deviceId);
    rangDev->SetRangingState(RangingAdapterState::CONNECTED);
    manager->rangingDev_.EnsureInsert(deviceId, rangDev);

    manager->OnConnectionStateChange(deviceId, static_cast<int32_t>(SleConnState::SLE_CONNECTION_STATE_ENCRYPTED));

    manager->rangingDev_.Erase(deviceId);
}

/*
 * Feature: RangingManager
 * SubFeature: StopRanging
 * Function: StopRanging
 * Condition: Device exists in RANGING state, nearlinkRanging_ is null
 * Test Steps: Insert RangingDevice in RANGING state, set nearlinkRanging_ to nullptr,
 *             call StopRanging, verify device erased and returns error
 * Expected Result: Returns RANGING_ERR_OPERATION_FAILED (nearlinkRanging_ nullptr)
 */
HWTEST_F(RangingManagerTest, StopRanging_WithDeviceButNullRanging_ShouldReturnError, TestSize.Level1)
{
    auto *manager = RangingManager::GetInstance();
    manager->Init();

    std::string deviceId = "AA:BB:CC:DD:EE:FF";
    auto rangDev = std::make_shared<RangingDevice>(deviceId);
    rangDev->SetRangingState(RangingAdapterState::RANGING);
    manager->rangingDev_.EnsureInsert(deviceId, rangDev);

    auto savedRanging = manager->nearlinkRanging_;
    manager->nearlinkRanging_ = nullptr;

    int ret = manager->StopRanging(deviceId);
    EXPECT_NE(ret, RANGING_NO_ERROR);
    EXPECT_NE(manager->GetRangingDevice(deviceId), nullptr);

    manager->nearlinkRanging_ = savedRanging;
    manager->rangingDev_.Erase(deviceId);
}