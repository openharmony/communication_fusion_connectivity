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
#define LOG_TAG "TestFusionRangingService"
#endif

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <thread>
#include "fusion_ranging_service.h"
#include "ranging_params.h"
#include "ranging_result.h"
#include "ranging_measurement.h"
#include "base_ranging_adapter.h"
#include "ranging_adapter_factory.h"
#include "fusion_ranging_errorcode.h"
#include "log.h"
#include "ranging_observer_stub.h"
#include "iremote_object.h"
#include "fusion_ranging_types.h"
#include "fcm_thread_util.h"

using namespace OHOS;
using namespace OHOS::FusionRanging;
using namespace testing;
using namespace testing::ext;

constexpr int32_t CALLER_UID_A = 1000;
constexpr int32_t CALLER_UID_B = 1001;
constexpr int32_t CALLER_UID_C = 1002;
constexpr int32_t CALLER_UID_D = 1003;

class MockBaseRangingAdapter : public BaseRangingAdapter {
public:
    MOCK_METHOD(int32_t, Init, (), (override));
    MOCK_METHOD(int32_t, DeInit, (), (override));
    MOCK_METHOD(int32_t, StartRanging, (const std::string &deviceId), (override));
    MOCK_METHOD(int32_t, StopRanging, (const std::string &deviceId), (override));
    MOCK_METHOD(int32_t, StartPassiveRanging, (int32_t &handle), (override));
    MOCK_METHOD(int32_t, StopPassiveRanging, (int32_t handle), (override));
    MOCK_METHOD(int32_t, PauseRanging, (const std::string &deviceId), (override));
    MOCK_METHOD(int32_t, ResumeRanging, (const std::string &deviceId), (override));
    MOCK_METHOD(int32_t, SetCallback, (const std::shared_ptr<BaseRangingAdapterCallback> &callback), (override));
};

class NiceMockBaseRangingAdapter : public NiceMock<MockBaseRangingAdapter> {
public:
    NiceMockBaseRangingAdapter()
    {
        auto generateHandle = [](int32_t &handle) {
            static int32_t counter = 0;
            handle = ++counter;
            return 0;
        };
        ON_CALL(*this, Init).WillByDefault(Return(0));
        ON_CALL(*this, DeInit).WillByDefault(Return(0));
        ON_CALL(*this, StartRanging(_)).WillByDefault(Return(0));
        ON_CALL(*this, StopRanging(_)).WillByDefault(Return(0));
        ON_CALL(*this, StartPassiveRanging(_)).WillByDefault(Invoke(generateHandle));
        ON_CALL(*this, StopPassiveRanging(_)).WillByDefault(Return(0));
        ON_CALL(*this, PauseRanging(_)).WillByDefault(Return(0));
        ON_CALL(*this, ResumeRanging(_)).WillByDefault(Return(0));
        ON_CALL(*this, SetCallback(_)).WillByDefault(Return(0));
    }
};

class MockIRangingObserver : public IRangingObserver {
public:
    MOCK_METHOD(ErrCode, OnRangingResult, (const RangingResult &result), (override));
    MOCK_METHOD(ErrCode, OnRangingStateChanged, (const RangingStateChangeInfo &info), (override));
    MOCK_METHOD(sptr<IRemoteObject>, AsObject, (), (override));
};

class FusionRangingServiceTest : public testing::Test {
public:
    FusionRangingServiceTest() = default;
    ~FusionRangingServiceTest() override = default;

    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    static std::atomic<bool> adapterRegistered_;

    FusionRangingService *service_;
};

std::atomic<bool> FusionRangingServiceTest::adapterRegistered_{false};

void FusionRangingServiceTest::SetUpTestCase(void)
{
    if (!adapterRegistered_) {
        auto generator = []() -> std::shared_ptr<BaseRangingAdapter> {
            return std::make_shared<NiceMockBaseRangingAdapter>();
        };
        RangingAdapterFactory::Instance().RegisterRangingAdapter<NiceMockBaseRangingAdapter>(
            RangingTypes::NEARLINK_HADM, generator);
        RangingAdapterFactory::Instance().RegisterChecker(RangingTypes::NEARLINK_HADM, []() { return true; });
        adapterRegistered_ = true;
    }
}

void FusionRangingServiceTest::TearDownTestCase(void) {}

void FusionRangingServiceTest::SetUp()
{
    FusionConnectivity::FcmThreadUtil::GetInstance().ClearThreadStateMap();
    FusionConnectivity::FcmThreadUtil::GetInstance().threadStateMap_.EnsureInsert(
        FusionConnectivity::THREAD_ID_RANGING, FusionConnectivity::FcmThreadUtil::NOT_SWITCH_THREAD);
    FusionConnectivity::FcmThreadUtil::GetInstance().threadStateMap_.EnsureInsert(
        FusionConnectivity::THREAD_ID_MAIN, FusionConnectivity::FcmThreadUtil::NOT_SWITCH_THREAD);
    service_ = FusionRangingService::GetInstance();
}

void FusionRangingServiceTest::TearDown()
{
    service_->HandleProcessDeath(CALLER_UID_A);
    service_->HandleProcessDeath(CALLER_UID_B);
    service_->HandleProcessDeath(CALLER_UID_C);
    service_->HandleProcessDeath(CALLER_UID_D);
}

HWTEST_F(FusionRangingServiceTest, GetInstance_001, TestSize.Level0)
{
    auto instance = FusionRangingService::GetInstance();
    EXPECT_NE(instance, nullptr);
}

HWTEST_F(FusionRangingServiceTest, IsRangingSupported_001, TestSize.Level1)
{
    bool nearlinkSupported = service_->IsRangingSupported(RangingTypes::NEARLINK_HADM);
    bool unregisteredSupported = service_->IsRangingSupported(static_cast<RangingTypes>(999));
    EXPECT_TRUE(nearlinkSupported);
    EXPECT_FALSE(unregisteredSupported);
}

HWTEST_F(FusionRangingServiceTest, HandleProcessDeath_001, TestSize.Level2)
{
    service_->HandleProcessDeath(CALLER_UID_A);
    service_->HandleProcessDeath(2000);
    SUCCEED();
}

HWTEST_F(FusionRangingServiceTest, PauseRangingByUid_001, TestSize.Level2)
{
    service_->PauseRangingByUid(CALLER_UID_A);
    SUCCEED();
}

HWTEST_F(FusionRangingServiceTest, ResumeRangingByUid_001, TestSize.Level2)
{
    service_->ResumeRangingByUid(CALLER_UID_A);
    SUCCEED();
}

HWTEST_F(FusionRangingServiceTest, IsRangingEmpty_001, TestSize.Level2)
{
    EXPECT_TRUE(service_->IsRangingEmpty());
    RangingParams params("AA:BB:CC:DD:EE:FF", RangingTypes::NEARLINK_HADM);
    sptr<IRangingObserver> observer;
    service_->HandleStartRanging(params, observer, CALLER_UID_A);
    EXPECT_FALSE(service_->IsRangingEmpty());
}

HWTEST_F(FusionRangingServiceTest, OnRangingResult_001, TestSize.Level2)
{
    sptr<MockIRangingObserver> mockObserver = new MockIRangingObserver();
    EXPECT_CALL(*mockObserver, OnRangingResult(_)).Times(1);
    RangingParams params("AA:BB:CC:DD:EE:FF", RangingTypes::NEARLINK_HADM);
    int32_t startResult = service_->StartRanging(params, mockObserver, CALLER_UID_A);
    EXPECT_EQ(startResult, 0);
    RangingResult result;
    result.SetDeviceId("AA:BB:CC:DD:EE:FF");
    service_->OnRangingResult(result);
}

HWTEST_F(FusionRangingServiceTest, OnRangingResult_002, TestSize.Level3)
{
    RangingResult result;
    result.SetDeviceId("AA:BB:CC:DD:EE:FF");
    service_->OnRangingResult(result);
    EXPECT_EQ(service_->GetRangingDevice("AA:BB:CC:DD:EE:FF"), nullptr);
    RangingParams params("AA:BB:CC:DD:EE:FF", RangingTypes::NEARLINK_HADM);
    sptr<NiceMock<MockIRangingObserver>> mockObserver = new NiceMock<MockIRangingObserver>();
    service_->HandleStartRanging(params, mockObserver, CALLER_UID_A);
    EXPECT_NE(service_->GetRangingDevice("AA:BB:CC:DD:EE:FF"), nullptr);
    service_->OnRangingResult(result);
}

HWTEST_F(FusionRangingServiceTest, OnAdapterRangingStateChanged_001, TestSize.Level2)
{
    sptr<MockIRangingObserver> mockObserver = new MockIRangingObserver();
    EXPECT_CALL(*mockObserver, OnRangingStateChanged(_)).Times(2);
    RangingParams params("AA:BB:CC:DD:EE:FF", RangingTypes::NEARLINK_HADM);
    int32_t startResult = service_->StartRanging(params, mockObserver, CALLER_UID_A);
    EXPECT_EQ(startResult, 0);
    AdapterRangingStateInfo stoppedInfo(AdapterRangingType::ADAPTER_RANGING, "AA:BB:CC:DD:EE:FF", 0,
                                        RangingState::STATE_STOPPED);
    AdapterRangingStateInfo startedInfo(AdapterRangingType::ADAPTER_RANGING, "AA:BB:CC:DD:EE:FF", 1,
                                        RangingState::STATE_STARTED);
    service_->OnAdapterRangingStateChanged(stoppedInfo);
    service_->OnAdapterRangingStateChanged(startedInfo);
}

HWTEST_F(FusionRangingServiceTest, StartRanging_001, TestSize.Level1)
{
    RangingParams params("AA:BB:CC:DD:EE:FF", RangingTypes::NEARLINK_HADM);
    sptr<IRangingObserver> observer;
    int32_t result = service_->StartRanging(params, observer, CALLER_UID_A);
    EXPECT_EQ(result, 0);
}

HWTEST_F(FusionRangingServiceTest, StartRanging_002, TestSize.Level1)
{
    RangingParams params("BB:BB:CC:DD:EE:FF", RangingTypes::NEARLINK_HADM);
    sptr<IRangingObserver> observer;
    int32_t firstResult = service_->StartRanging(params, observer, CALLER_UID_B);
    EXPECT_EQ(firstResult, 0);
    int32_t secondResult = service_->StartRanging(params, observer, CALLER_UID_B);
    EXPECT_EQ(secondResult, static_cast<int32_t>(RangingErrCode::RANGING_ERR_DEVICE_ALREADY_INITIATED));
}

HWTEST_F(FusionRangingServiceTest, StartRanging_003, TestSize.Level1)
{
    RangingParams params("CC:BB:CC:DD:EE:FF", static_cast<RangingTypes>(999));
    sptr<IRangingObserver> observer;
    int32_t result = service_->StartRanging(params, observer, CALLER_UID_C);
    EXPECT_EQ(result, static_cast<int32_t>(RangingErrCode::RANGING_ERR_RANGING_SERVICE_DISABLED));
}

HWTEST_F(FusionRangingServiceTest, StartRanging_004, TestSize.Level2)
{
    RangingParams params("", RangingTypes::NEARLINK_HADM);
    sptr<IRangingObserver> observer;
    int32_t result = service_->StartRanging(params, observer, CALLER_UID_A);
    EXPECT_EQ(result, static_cast<int32_t>(RangingErrCode::RANGING_ERR_INVALID_PARAM));
}

HWTEST_F(FusionRangingServiceTest, StopRanging_001, TestSize.Level1)
{
    RangingParams params("AA:BB:CC:DD:EE:FF", RangingTypes::NEARLINK_HADM);
    sptr<IRangingObserver> observer;
    int32_t startResult = service_->StartRanging(params, observer, CALLER_UID_A);
    EXPECT_EQ(startResult, 0);
    int32_t stopResult = service_->StopRanging("AA:BB:CC:DD:EE:FF", CALLER_UID_A);
    EXPECT_EQ(stopResult, 0);
}

HWTEST_F(FusionRangingServiceTest, StopRanging_002, TestSize.Level2)
{
    int32_t stopResult = service_->StopRanging("", CALLER_UID_A);
    EXPECT_EQ(stopResult, static_cast<int32_t>(RangingErrCode::RANGING_ERR_INVALID_PARAM));
    stopResult = service_->StopRanging("AA:BB:CC:DD:EE:FF", CALLER_UID_A);
    EXPECT_EQ(stopResult, static_cast<int32_t>(RangingErrCode::RANGING_ERR_DEVICE_NOT_INITIATED));
    RangingParams params("AA:BB:CC:DD:EE:FF", RangingTypes::NEARLINK_HADM);
    sptr<IRangingObserver> observer;
    int32_t startResult = service_->StartRanging(params, observer, CALLER_UID_A);
    EXPECT_EQ(startResult, 0);
    stopResult = service_->StopRanging("AA:BB:CC:DD:EE:FF", CALLER_UID_A);
    EXPECT_EQ(stopResult, 0);
}

HWTEST_F(FusionRangingServiceTest, StopRanging_003, TestSize.Level2)
{
    RangingParams params("AA:BB:CC:DD:EE:FF", RangingTypes::NEARLINK_HADM);
    sptr<IRangingObserver> observer;
    int32_t startResult = service_->StartRanging(params, observer, CALLER_UID_A);
    EXPECT_EQ(startResult, 0);
    int32_t stopResult = service_->StopRanging("AA:BB:CC:DD:EE:FF", CALLER_UID_B);
    EXPECT_EQ(stopResult, static_cast<int32_t>(RangingErrCode::RANGING_ERR_OPERATION_FAILED));
}

HWTEST_F(FusionRangingServiceTest, StartPassiveRanging_001, TestSize.Level1)
{
    int32_t handle = 0;
    sptr<IRangingObserver> observer;
    int32_t result = service_->StartPassiveRanging(RangingTypes::NEARLINK_HADM, handle, observer, CALLER_UID_C);
    EXPECT_EQ(result, 0);
    EXPECT_FALSE(service_->IsRangingEmpty());
}

HWTEST_F(FusionRangingServiceTest, StopPassiveRanging_001, TestSize.Level1)
{
    int32_t handle = 0;
    sptr<IRangingObserver> observer;
    int32_t startResult = service_->StartPassiveRanging(RangingTypes::NEARLINK_HADM, handle, observer, CALLER_UID_D);
    ASSERT_EQ(startResult, 0);
    ASSERT_NE(handle, 0);
    int32_t result = service_->StopPassiveRanging(RangingTypes::NEARLINK_HADM, handle, CALLER_UID_D);
    EXPECT_EQ(result, 0);
}

HWTEST_F(FusionRangingServiceTest, HandleStartRanging_001, TestSize.Level2)
{
    sptr<MockIRangingObserver> mockObserver = new MockIRangingObserver();
    EXPECT_CALL(*mockObserver, OnRangingStateChanged(_)).Times(0);
    RangingParams params("DD:BB:CC:DD:EE:FF", RangingTypes::NEARLINK_HADM);
    service_->HandleStartRanging(params, mockObserver, CALLER_UID_A);
    EXPECT_NE(service_->GetRangingDevice("DD:BB:CC:DD:EE:FF"), nullptr);
}

HWTEST_F(FusionRangingServiceTest, HandleStartRanging_002, TestSize.Level2)
{
    sptr<MockIRangingObserver> mockObserver = new MockIRangingObserver();
    EXPECT_CALL(*mockObserver, OnRangingStateChanged(_)).Times(0);
    RangingParams params("DD:BB:CC:DD:EE:FF", RangingTypes::NEARLINK_HADM);
    service_->HandleStartRanging(params, mockObserver, CALLER_UID_A);
    service_->HandleStartRanging(params, mockObserver, CALLER_UID_A);
    EXPECT_NE(service_->GetRangingDevice("DD:BB:CC:DD:EE:FF"), nullptr);
}

HWTEST_F(FusionRangingServiceTest, OnAdapterRangingStateChangedMultiThread_001, TestSize.Level3)
{
    auto thread1 = std::thread([this] {
        for (int32_t i = 0; i < 100; ++i) {
            AdapterRangingStateInfo info(AdapterRangingType::ADAPTER_RANGING,
                                         "AA:BB:CC:DD:EE:" + std::to_string(i % 100), i,
                                         static_cast<RangingState>(i % 2));
            service_->OnAdapterRangingStateChanged(info);
        }
    });
    auto thread2 = std::thread([this] {
        for (int32_t i = 0; i < 100; ++i) {
            RangingResult result;
            result.SetDeviceId("AA:BB:CC:DD:EE:" + std::to_string(i % 100));
            service_->OnRangingResult(result);
        }
    });
    thread1.join();
    thread2.join();
    SUCCEED();
}