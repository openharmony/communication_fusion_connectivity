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
#define LOG_TAG "FusionRangingServiceTest"
#endif

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <thread>
#include "fusion_ranging_service.h"
#include "ranging_params.h"
#include "ranging_result.h"
#include "base_ranging_adapter.h"
#include "ranging_adapter_factory.h"
#include "log.h"

using namespace OHOS;
using namespace OHOS::FusionRanging;
using namespace testing;
using namespace testing::ext;

class MockRangingAdapter : public BaseRangingAdapter {
public:
    MOCK_METHOD(int, Init, (), (override));
    MOCK_METHOD(int, DeInit, (), (override));
    MOCK_METHOD(int, StartRanging, (const std::string &deviceId, RangingRole role), (override));
    MOCK_METHOD(int, StopRanging, (const std::string &deviceId), (override));
    MOCK_METHOD(int, PauseRanging, (const std::string &deviceId), (override));
    MOCK_METHOD(int, ResumeRanging, (const std::string &deviceId), (override));
    MOCK_METHOD(int, SetCallback, (const std::shared_ptr<BaseRangingAdapterCallback> &callback), (override));
};

class FusionRangingServiceTest : public testing::Test {
public:
    FusionRangingServiceTest() = default;
    ~FusionRangingServiceTest() override = default;

    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    std::shared_ptr<FusionRangingService> service_;
    std::shared_ptr<MockRangingAdapter> mockAdapter_;
};

void FusionRangingServiceTest::SetUpTestCase(void) {}

void FusionRangingServiceTest::TearDownTestCase(void) {}

void FusionRangingServiceTest::SetUp()
{
    service_ = FusionRangingService::GetInstance();
    mockAdapter_ = std::make_shared<MockRangingAdapter>();
}

void FusionRangingServiceTest::TearDown() {}

/**
 * @tc.name: GetInstanceShouldReturnNonNull
 * @tc.desc: 测试用例1：验证GetInstance返回非空实例
 * @tc.type: FUNC
 */
HWTEST_F(FusionRangingServiceTest, GetInstanceShouldReturnNonNull, TestSize.Level0)
{
    auto instance = FusionRangingService::GetInstance();
    EXPECT_NE(instance, nullptr);
}

/**
 * @tc.name: StartRangingWithInvalidDeviceIdShouldFail
 * @tc.desc: 测试用例2：验证空设备ID时StartRanging失败
 * @tc.type: FUNC
 */
HWTEST_F(FusionRangingServiceTest, StartRangingWithInvalidDeviceIdShouldFail, TestSize.Level0)
{
    RangingParams params("", RangingRole::ROLE_INITIATOR, RangingTypes::NEARLINK_HADM);
    auto callback = [](const RangingResult &result) {
    };

    int ret = service_->StartRanging(params, callback);
    EXPECT_EQ(ret, static_cast<int>(RangingErrCode::RANGING_ERR_INVALID_PARAM));
}

/**
 * @tc.name: StopRangingWithInvalidDeviceIdShouldFail
 * @tc.desc: 测试用例3：验证空设备ID时StopRanging失败
 * @tc.type: FUNC
 */
HWTEST_F(FusionRangingServiceTest, StopRangingWithInvalidDeviceIdShouldFail, TestSize.Level0)
{
    int ret = service_->StopRanging("");
    EXPECT_EQ(ret, static_cast<int>(RangingErrCode::RANGING_ERR_INVALID_PARAM));
}

/**
 * @tc.name: StopRangingWithNonExistingDeviceShouldFail
 * @tc.desc: 测试用例4：验证不存在设备时StopRanging失败
 * @tc.type: FUNC
 */
HWTEST_F(FusionRangingServiceTest, StopRangingWithNonExistingDeviceShouldFail, TestSize.Level0)
{
    int ret = service_->StopRanging("00:11:22:33:44:55");
    EXPECT_EQ(ret, static_cast<int>(RangingErrCode::RANGING_ERR_DEVICE_NOT_FOUND));
}

/**
 * @tc.name: PauseRangingWithInvalidDeviceIdShouldFail
 * @tc.desc: 测试用例5：验证空设备ID时PauseRanging失败
 * @tc.type: FUNC
 */
HWTEST_F(FusionRangingServiceTest, PauseRangingWithInvalidDeviceIdShouldFail, TestSize.Level0)
{
    int ret = service_->PauseRanging("");
    EXPECT_EQ(ret, static_cast<int>(RangingErrCode::RANGING_ERR_INVALID_PARAM));
}

/**
 * @tc.name: ResumeRangingWithInvalidDeviceIdShouldFail
 * @tc.desc: 测试用例6：验证空设备ID时ResumeRanging失败
 * @tc.type: FUNC
 */
HWTEST_F(FusionRangingServiceTest, ResumeRangingWithInvalidDeviceIdShouldFail, TestSize.Level0)
{
    int ret = service_->ResumeRanging("");
    EXPECT_EQ(ret, static_cast<int>(RangingErrCode::RANGING_ERR_INVALID_PARAM));
}

/**
 * @tc.name: GetRangingDataWithInvalidDeviceIdShouldFail
 * @tc.desc: 测试用例7：验证空设备ID时GetRangingData失败
 * @tc.type: FUNC
 */
HWTEST_F(FusionRangingServiceTest, GetRangingDataWithInvalidDeviceIdShouldFail, TestSize.Level0)
{
    int32_t distance = 0;
    int32_t rssi = 0;
    int ret = service_->GetRangingData("", distance, rssi);
    EXPECT_EQ(ret, static_cast<int>(RangingErrCode::RANGING_ERR_INVALID_PARAM));
}

/**
 * @tc.name: OnRangingResultWithValidDataShouldCallCallback
 * @tc.desc: 测试用例8：验证OnRangingResult能正确调用回调
 * @tc.type: FUNC
 */
HWTEST_F(FusionRangingServiceTest, OnRangingResultWithValidDataShouldCallCallback, TestSize.Level0)
{
    bool callbackCalled = false;
    RangingParams params("00:11:22:33:44:55", RangingRole::ROLE_INITIATOR, RangingTypes::NEARLINK_HADM);

    auto callback = [&callbackCalled](const RangingResult &result) {
        callbackCalled = true;
    };

    RangingResult result;
    result.SetDeviceId("00:11:22:33:44:55");

    service_->OnRangingResult(result);
}

/**
 * @tc.name: OnAdapterRangingStateChangedShouldLogState
 * @tc.desc: 测试用例9：验证状态变更日志记录
 * @tc.type: FUNC
 */
HWTEST_F(FusionRangingServiceTest, OnAdapterRangingStateChangedShouldLogState, TestSize.Level0)
{
    HILOGI("OnAdapterRangingStateChanged test");
    service_->OnAdapterRangingStateChanged(0);
    service_->OnAdapterRangingStateChanged(1);
    service_->OnAdapterRangingStateChanged(2);
}

/**
 * @tc.name: ThreadSafetyTest
 * @tc.desc: 测试用例10：线程安全测试
 * @tc.type: FUNC
 */
HWTEST_F(FusionRangingServiceTest, ThreadSafetyTest, TestSize.Level0)
{
    auto thread1 = std::thread([&] {
        for (int i = 0; i < 100; ++i) {
            service_->OnAdapterRangingStateChanged(i % 3);
        }
    });

    auto thread2 = std::thread([&] {
        for (int i = 0; i < 100; ++i) {
            RangingResult result;
            result.SetDeviceId("test_device_" + std::to_string(i));
            service_->OnRangingResult(result);
        }
    });

    thread1.join();
    thread2.join();
}