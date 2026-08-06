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
#define LOG_TAG "TestFusionRangingServer"
#endif

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <thread>
#include <atomic>
#include "fusion_connectivity_errorcode.h"
#include "fusion_ranging_server.h"
#include "ranging_params.h"
#include "ranging_result.h"
#include "ranging_state_change_info.h"
#include "ranging_capability_supported.h"
#include "ranging_observer_stub.h"
#include "fusion_ranging_errorcode.h"
#include "log.h"
#include "iremote_object.h"
#include "fusion_ranging_types.h"
#include "ranging_adapter_factory.h"
#include "base_ranging_adapter.h"
#include "fusion_ranging_service.h"
#include "ipc_skeleton.h"
#include "fcm_thread_util.h"

using namespace OHOS;
using namespace OHOS::FusionRanging;
using namespace testing;
using namespace testing::ext;

class MockIRangingObserver : public IRangingObserver {
public:
    MOCK_METHOD(ErrCode, OnRangingResult, (const RangingResult &result), (override));
    MOCK_METHOD(ErrCode, OnRangingStateChanged, (const RangingStateChangeInfo &info), (override));
    MOCK_METHOD(sptr<IRemoteObject>, AsObject, (), (override));
};

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

class FusionRangingServerTest : public testing::Test {
public:
    FusionRangingServerTest() = default;
    ~FusionRangingServerTest() override = default;

    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    static std::atomic<bool> adapterRegistered_;
    sptr<FusionRangingServer> server_;
    sptr<MockIRangingObserver> mockObserver_;
    bool observerRegistered_ = false;
};

std::atomic<bool> FusionRangingServerTest::adapterRegistered_{false};

void FusionRangingServerTest::SetUpTestCase(void)
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

void FusionRangingServerTest::TearDownTestCase(void) {}

void FusionRangingServerTest::SetUp()
{
    FusionConnectivity::FcmThreadUtil::GetInstance().ClearThreadStateMap();
    FusionConnectivity::FcmThreadUtil::GetInstance().threadStateMap_.EnsureInsert(
        FusionConnectivity::THREAD_ID_RANGING, FusionConnectivity::FcmThreadUtil::NOT_SWITCH_THREAD);
    FusionConnectivity::FcmThreadUtil::GetInstance().threadStateMap_.EnsureInsert(
        FusionConnectivity::THREAD_ID_MAIN, FusionConnectivity::FcmThreadUtil::NOT_SWITCH_THREAD);
    server_ = FusionRangingServer::GetInstance();
    mockObserver_ = new MockIRangingObserver();
    observerRegistered_ = false;
    EXPECT_CALL(*mockObserver_, AsObject()).WillRepeatedly(Return(nullptr));
}

void FusionRangingServerTest::TearDown()
{
    if (server_ != nullptr && observerRegistered_) {
        server_->DeregisterObserver(mockObserver_);
    }
    FusionRangingService::GetInstance()->HandleProcessDeath(IPCSkeleton::GetCallingUid());
    mockObserver_ = nullptr;
}

HWTEST_F(FusionRangingServerTest, GetInstance_001, TestSize.Level0)
{
    auto instance = FusionRangingServer::GetInstance();
    EXPECT_NE(instance, nullptr);
}

HWTEST_F(FusionRangingServerTest, GetRangingCapability_001, TestSize.Level1)
{
    RangingCapabilitySupported capability;
    ErrCode result = server_->GetRangingCapability(capability);
    EXPECT_EQ(result, static_cast<int32_t>(RangingErrCode::RANGING_NO_ERROR));
}

HWTEST_F(FusionRangingServerTest, HandleProcessDeath_001, TestSize.Level2)
{
    server_->HandleProcessDeath(1000);
    SUCCEED();
}

HWTEST_F(FusionRangingServerTest, CheckAndUnloadSA_001, TestSize.Level2)
{
    server_->CheckAndUnloadSA();
    SUCCEED();
}

HWTEST_F(FusionRangingServerTest, StartRanging_001, TestSize.Level1)
{
    ErrCode registerResult = server_->RegisterObserver(mockObserver_);
    observerRegistered_ = true;
    EXPECT_EQ(registerResult, static_cast<int32_t>(RangingErrCode::RANGING_NO_ERROR));

    RangingParams params("11:BB:CC:DD:EE:FF", RangingTypes::NEARLINK_HADM);
    ErrCode result = server_->StartRanging(params);
    EXPECT_EQ(result, static_cast<int32_t>(RangingErrCode::RANGING_NO_ERROR));
}

HWTEST_F(FusionRangingServerTest, StartRanging_002, TestSize.Level2)
{
    RangingParams params("AA:BB:CC:DD:EE:FF", RangingTypes::NEARLINK_HADM);
    ErrCode result = server_->StartRanging(params);
    EXPECT_EQ(result, static_cast<int32_t>(RangingErrCode::RANGING_ERR_OPERATION_FAILED));
}

HWTEST_F(FusionRangingServerTest, StartRanging_003, TestSize.Level2)
{
    server_->RegisterObserver(mockObserver_);
    observerRegistered_ = true;
    RangingParams params("", RangingTypes::NEARLINK_HADM);
    ErrCode result = server_->StartRanging(params);
    EXPECT_EQ(result, static_cast<int32_t>(RangingErrCode::RANGING_ERR_PARAM_NOT_MEET_SPECIFICATIONS));
    int32_t callerUid = IPCSkeleton::GetCallingUid();
    FusionRangingService::GetInstance()->HandleProcessDeath(callerUid);
}

HWTEST_F(FusionRangingServerTest, StartRanging_004, TestSize.Level1)
{
    ErrCode registerResult = server_->RegisterObserver(mockObserver_);
    observerRegistered_ = true;
    EXPECT_EQ(registerResult, static_cast<int32_t>(RangingErrCode::RANGING_NO_ERROR));

    RangingParams params("44:BB:CC:DD:EE:FF", RangingTypes::NEARLINK_HADM);
    ErrCode result = server_->StartRanging(params);
    EXPECT_EQ(result, static_cast<int32_t>(RangingErrCode::RANGING_NO_ERROR));
    result = server_->StartRanging(params);
    EXPECT_EQ(result, static_cast<int32_t>(RangingErrCode::RANGING_ERR_DEVICE_ALREADY_INITIATED));
}

HWTEST_F(FusionRangingServerTest, StartRanging_005, TestSize.Level1)
{
    ErrCode registerResult = server_->RegisterObserver(mockObserver_);
    observerRegistered_ = true;
    EXPECT_EQ(registerResult, static_cast<int32_t>(RangingErrCode::RANGING_NO_ERROR));

    RangingParams params("55:BB:CC:DD:EE:FF", RangingTypes::NEARLINK_HADM);
    ErrCode firstResult = server_->StartRanging(params);
    EXPECT_EQ(firstResult, static_cast<int32_t>(RangingErrCode::RANGING_NO_ERROR));

    ErrCode stopResult = server_->StopRanging(params);
    EXPECT_EQ(stopResult, static_cast<int32_t>(RangingErrCode::RANGING_NO_ERROR));

    ErrCode secondResult = server_->StartRanging(params);
    EXPECT_EQ(secondResult, static_cast<int32_t>(RangingErrCode::RANGING_NO_ERROR));
}

HWTEST_F(FusionRangingServerTest, StopRanging_001, TestSize.Level1)
{
    RangingParams params("", RangingTypes::NEARLINK_HADM);
    ErrCode result = server_->StopRanging(params);
    EXPECT_EQ(result, static_cast<int32_t>(RangingErrCode::RANGING_ERR_PARAM_NOT_MEET_SPECIFICATIONS));
}

HWTEST_F(FusionRangingServerTest, StopRanging_002, TestSize.Level1)
{
    RangingParams params("   ", RangingTypes::NEARLINK_HADM);
    ErrCode result = server_->StopRanging(params);
    EXPECT_EQ(result, static_cast<int32_t>(RangingErrCode::RANGING_ERR_PARAM_NOT_MEET_SPECIFICATIONS));
}

HWTEST_F(FusionRangingServerTest, StartPassiveRanging_001, TestSize.Level1)
{
    ErrCode registerResult = server_->RegisterObserver(mockObserver_);
    observerRegistered_ = true;
    EXPECT_EQ(registerResult, static_cast<int32_t>(RangingErrCode::RANGING_NO_ERROR));

    int32_t handle = 0;
    server_->StartPassiveRanging(static_cast<int32_t>(RangingTypes::NEARLINK_HADM), handle);
    SUCCEED();
}

HWTEST_F(FusionRangingServerTest, StopPassiveRanging_001, TestSize.Level1)
{
    ErrCode stopResult = server_->StopPassiveRanging(static_cast<int32_t>(RangingTypes::NEARLINK_HADM), -1);
    EXPECT_EQ(stopResult, static_cast<int32_t>(RangingErrCode::RANGING_ERR_PARAM_NOT_MEET_SPECIFICATIONS));
    server_->StopPassiveRanging(static_cast<int32_t>(RangingTypes::NEARLINK_HADM), 0);
}

HWTEST_F(FusionRangingServerTest, RegisterObserver_001, TestSize.Level1)
{
    ErrCode result = server_->RegisterObserver(mockObserver_);
    observerRegistered_ = true;
    EXPECT_EQ(result, static_cast<int32_t>(RangingErrCode::RANGING_NO_ERROR));
}

HWTEST_F(FusionRangingServerTest, RegisterObserver_002, TestSize.Level1)
{
    ErrCode firstResult = server_->RegisterObserver(mockObserver_);
    observerRegistered_ = true;
    EXPECT_EQ(firstResult, static_cast<int32_t>(RangingErrCode::RANGING_NO_ERROR));

    ErrCode secondResult = server_->RegisterObserver(mockObserver_);
    EXPECT_EQ(secondResult, static_cast<int32_t>(RangingErrCode::RANGING_ERR_OPERATION_FAILED));
}

HWTEST_F(FusionRangingServerTest, DeregisterObserver_001, TestSize.Level1)
{
    server_->RegisterObserver(mockObserver_);
    observerRegistered_ = true;
    ErrCode result = server_->DeregisterObserver(mockObserver_);
    observerRegistered_ = false;
    EXPECT_EQ(result, static_cast<int32_t>(RangingErrCode::RANGING_NO_ERROR));
}

HWTEST_F(FusionRangingServerTest, DeregisterObserver_002, TestSize.Level1)
{
    sptr<MockIRangingObserver> anotherObserver = new MockIRangingObserver();
    EXPECT_CALL(*anotherObserver, AsObject()).WillRepeatedly(Return(nullptr));
    server_->RegisterObserver(anotherObserver);
    ErrCode result = server_->DeregisterObserver(mockObserver_);
    EXPECT_EQ(result, static_cast<int32_t>(RangingErrCode::RANGING_ERR_OPERATION_FAILED));
}

HWTEST_F(FusionRangingServerTest, DeregisterObserver_003, TestSize.Level1)
{
    ErrCode result = server_->DeregisterObserver(mockObserver_);
    EXPECT_EQ(result, static_cast<int32_t>(RangingErrCode::RANGING_ERR_OPERATION_FAILED));
}

HWTEST_F(FusionRangingServerTest, HandleProcessDeathMultiThread_001, TestSize.Level3)
{
    auto thread1 = std::thread([this] {
        for (int32_t i = 0; i < 50; ++i) {
            server_->HandleProcessDeath(i);
        }
    });
    thread1.join();
    SUCCEED();
}