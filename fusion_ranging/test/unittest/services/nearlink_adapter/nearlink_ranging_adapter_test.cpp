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
#define LOG_TAG "NearlinkRangingAdapterTest"
#endif

#include <gtest/gtest.h>

#include "nearlink_ranging_adapter.h"
#include "ranging_manager.h"
#include "ranging_adapter_factory.h"
#include "base_ranging_adapter.h"
#include "fusion_ranging_types.h"
#include "fusion_ranging_errorcode.h"
#include "log_utils.h"
#include "fcm_thread_util.h"

using namespace OHOS;
using namespace OHOS::FusionRanging;
using namespace testing::ext;

class MockRangingAdapterCallback : public BaseRangingAdapterCallback {
public:
    void OnRangingStateChange(const AdapterRangingStateInfo &Info) override {}
    void OnRangingResult(const AdapterRangingData &data) override {}
};

class NearlinkRangingAdapterTest : public testing::Test {
public:
    NearlinkRangingAdapterTest() = default;
    ~NearlinkRangingAdapterTest() override = default;

    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

private:
    std::shared_ptr<NearlinkRangingAdapter> adapter_;
};

void NearlinkRangingAdapterTest::SetUpTestCase(void)
{
    OHOS::FusionConnectivity::FcmThreadUtil::GetInstance().InitThreadStateMap();
}

void NearlinkRangingAdapterTest::TearDownTestCase(void)
{
    OHOS::FusionConnectivity::FcmThreadUtil::GetInstance().ClearThreadStateMap();
}

void NearlinkRangingAdapterTest::SetUp(void)
{
    adapter_ = std::make_shared<NearlinkRangingAdapter>();
}

void NearlinkRangingAdapterTest::TearDown(void)
{
    adapter_->DeInit();
}

/*
 * Feature: NearlinkRangingAdapter
 * SubFeature: Constructor/Destructor
 * Function: NearlinkRangingAdapter
 * Condition: None
 * Test Steps: Create NearlinkRangingAdapter instance, verify non-null and type
 * Expected Result: Instance created successfully
 */
HWTEST_F(NearlinkRangingAdapterTest, Constructor_ShouldCreateInstance, TestSize.Level0)
{
    auto adapter = std::make_shared<NearlinkRangingAdapter>();
    ASSERT_NE(adapter, nullptr);

    auto baseAdapter = std::dynamic_pointer_cast<BaseRangingAdapter>(adapter);
    EXPECT_NE(baseAdapter, nullptr);
    EXPECT_EQ(baseAdapter.get(), adapter.get());
}

/*
 * Feature: NearlinkRangingAdapter
 * SubFeature: Initialization
 * Function: Init / DeInit
 * Condition: Not initialized -> initialized -> deinitialized
 * Test Steps: Call Init, verify success; call DeInit, verify success
 * Expected Result: Both Init and DeInit return success
 */
HWTEST_F(NearlinkRangingAdapterTest, InitAndDeInit_ShouldReturnSuccess, TestSize.Level0)
{
    auto adapter = std::make_shared<NearlinkRangingAdapter>();
    ASSERT_NE(adapter, nullptr);

    int ret = adapter->Init();
    EXPECT_EQ(ret, RANGING_NO_ERROR);

    ret = adapter->DeInit();
    EXPECT_EQ(ret, RANGING_NO_ERROR);
}

/*
 * Feature: NearlinkRangingAdapter
 * SubFeature: StartRanging
 * Function: StartRanging
 * Condition: Empty device ID and invalid MAC format
 * Test Steps: Call StartRanging with empty string and malformed MAC
 * Expected Result: Returns PARAM_NOT_MEET_SPECIFICATIONS error
 */
HWTEST_F(NearlinkRangingAdapterTest, StartRanging_WithInvalidParam_ShouldReturnParamError, TestSize.Level1)
{
    adapter_->Init();
    EXPECT_EQ(adapter_->StartRanging(""), RANGING_ERR_PARAM_NOT_MEET_SPECIFICATIONS);
    EXPECT_EQ(adapter_->StartRanging("invalid"), RANGING_ERR_PARAM_NOT_MEET_SPECIFICATIONS);
    EXPECT_EQ(adapter_->StartRanging("11:22:33:44:55"), RANGING_ERR_PARAM_NOT_MEET_SPECIFICATIONS);
    EXPECT_EQ(adapter_->StartRanging("1234567890"), RANGING_ERR_PARAM_NOT_MEET_SPECIFICATIONS);
}

/*
 * Feature: NearlinkRangingAdapter
 * SubFeature: StartRanging / StopRanging / PauseRanging / ResumeRanging
 * Function: Multiple interfaces
 * Condition: Not initialized
 * Test Steps: Create adapter without Init, call StartRanging, StopRanging, PauseRanging, ResumeRanging
 * Expected Result: All return error (isInitialized_ is false)
 */
HWTEST_F(NearlinkRangingAdapterTest, RangingInterfaces_WhenNotInitialized_ShouldReturnError, TestSize.Level1)
{
    auto adapter = std::make_shared<NearlinkRangingAdapter>();
    ASSERT_NE(adapter, nullptr);
    std::string deviceId = "11:22:33:44:55:66";

    EXPECT_NE(adapter->StartRanging(deviceId), RANGING_NO_ERROR);
    EXPECT_NE(adapter->StopRanging(deviceId), RANGING_NO_ERROR);
    EXPECT_NE(adapter->PauseRanging(deviceId), RANGING_NO_ERROR);
    EXPECT_NE(adapter->ResumeRanging(deviceId), RANGING_NO_ERROR);
}

/*
 * Feature: NearlinkRangingAdapter
 * SubFeature: StopRanging
 * Function: StopRanging
 * Condition: Initialized but device not found
 * Test Steps: Call Init, then StopRanging for non-existent device
 * Expected Result: Returns DEVICE_NOT_INITIATED error
 */
HWTEST_F(NearlinkRangingAdapterTest, StopRanging_WhenDeviceNotFound_ShouldReturnNotInitiated, TestSize.Level1)
{
    adapter_->Init();
    std::string deviceId = "11:22:33:44:55:66";

    int ret = adapter_->StopRanging(deviceId);
    EXPECT_EQ(ret, RANGING_ERR_DEVICE_NOT_INITIATED);

    ret = adapter_->StopRanging("AA:BB:CC:DD:EE:FF");
    EXPECT_EQ(ret, RANGING_ERR_DEVICE_NOT_INITIATED);
}

/*
 * Feature: NearlinkRangingAdapter
 * SubFeature: SetCallback
 * Function: SetCallback
 * Condition: Initialized with nullptr and valid callback
 * Test Steps: Call SetCallback with nullptr, then with a valid callback object
 * Expected Result: Returns success for both calls
 */
HWTEST_F(NearlinkRangingAdapterTest, SetCallback_WithValidAndNullCallback_ShouldReturnSuccess, TestSize.Level0)
{
    adapter_->Init();

    int ret = adapter_->SetCallback(nullptr);
    EXPECT_EQ(ret, RANGING_NO_ERROR);

    auto callback = std::make_shared<MockRangingAdapterCallback>();
    ret = adapter_->SetCallback(callback);
    EXPECT_EQ(ret, RANGING_NO_ERROR);
}

/*
 * Feature: NearlinkRangingAdapter
 * SubFeature: StartPassiveRanging / StopPassiveRanging
 * Function: Passive ranging interfaces
 * Condition: Not initialized
 * Test Steps: Create adapter without Init, call StartPassiveRanging and StopPassiveRanging
 * Expected Result: Both return error (isInitialized_ is false)
 */
HWTEST_F(NearlinkRangingAdapterTest, PassiveRanging_WhenNotInitialized_ShouldReturnError, TestSize.Level1)
{
    auto adapter = std::make_shared<NearlinkRangingAdapter>();
    ASSERT_NE(adapter, nullptr);
    int32_t handle = -1;

    int ret = adapter->StartPassiveRanging(handle);
    EXPECT_NE(ret, RANGING_NO_ERROR);

    ret = adapter->StopPassiveRanging(0);
    EXPECT_NE(ret, RANGING_NO_ERROR);
}

/*
 * Feature: NearlinkRangingAdapter
 * SubFeature: Multiple Init/DeInit
 * Function: Init/DeInit
 * Condition: Repeated initialization
 * Test Steps: Call Init, DeInit, Init multiple times, verify idempotency
 * Expected Result: Each call returns success
 */
HWTEST_F(NearlinkRangingAdapterTest, MultipleInitDeInit_ShouldBeIdempotent, TestSize.Level1)
{
    adapter_->Init();
    EXPECT_EQ(adapter_->DeInit(), RANGING_NO_ERROR);
    EXPECT_EQ(adapter_->Init(), RANGING_NO_ERROR);
    EXPECT_EQ(adapter_->DeInit(), RANGING_NO_ERROR);
    EXPECT_EQ(adapter_->Init(), RANGING_NO_ERROR);
    EXPECT_EQ(adapter_->DeInit(), RANGING_NO_ERROR);
}

/*
 * Feature: NearlinkRangingAdapter
 * SubFeature: PauseRanging / ResumeRanging
 * Function: PauseRanging / ResumeRanging
 * Condition: Initialized but device not found
 * Test Steps: Call Init, then PauseRanging and ResumeRanging for non-existent device
 * Expected Result: Both return DEVICE_NOT_INITIATED error
 */
HWTEST_F(NearlinkRangingAdapterTest, PauseAndResume_WhenDeviceNotFound_ShouldReturnNotInitiated, TestSize.Level1)
{
    adapter_->Init();
    std::string deviceId = "11:22:33:44:55:66";

    int ret = adapter_->PauseRanging(deviceId);
    EXPECT_EQ(ret, RANGING_ERR_DEVICE_NOT_INITIATED);

    ret = adapter_->ResumeRanging(deviceId);
    EXPECT_EQ(ret, RANGING_ERR_DEVICE_NOT_INITIATED);

    ret = adapter_->PauseRanging("AA:BB:CC:DD:EE:FF");
    EXPECT_EQ(ret, RANGING_ERR_DEVICE_NOT_INITIATED);

    ret = adapter_->ResumeRanging("AA:BB:CC:DD:EE:FF");
    EXPECT_EQ(ret, RANGING_ERR_DEVICE_NOT_INITIATED);
}

/*
 * Feature: NearlinkRangingAdapter
 * SubFeature: PauseRanging / ResumeRanging
 * Function: PauseRanging / ResumeRanging
 * Condition: Initialized with invalid MAC address
 * Test Steps: Call Init, then PauseRanging and ResumeRanging with malformed MAC
 * Expected Result: Both return PARAM_NOT_MEET_SPECIFICATIONS error
 */
HWTEST_F(NearlinkRangingAdapterTest, PauseAndResume_WithInvalidMac_ShouldReturnParamError, TestSize.Level1)
{
    adapter_->Init();

    EXPECT_EQ(adapter_->PauseRanging("invalid"), RANGING_ERR_PARAM_NOT_MEET_SPECIFICATIONS);
    EXPECT_EQ(adapter_->ResumeRanging("invalid"), RANGING_ERR_PARAM_NOT_MEET_SPECIFICATIONS);
    EXPECT_EQ(adapter_->PauseRanging(""), RANGING_ERR_PARAM_NOT_MEET_SPECIFICATIONS);
    EXPECT_EQ(adapter_->ResumeRanging(""), RANGING_ERR_PARAM_NOT_MEET_SPECIFICATIONS);
}