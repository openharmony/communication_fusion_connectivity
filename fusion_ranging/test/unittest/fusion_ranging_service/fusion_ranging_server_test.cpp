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
#define LOG_TAG "FusionRangingServerTest"
#endif

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <thread>
#include "fusion_ranging_server.h"
#include "ranging_params.h"
#include "ranging_result.h"
#include "ranging_state_change_info.h"
#include "ranging_capability_supported.h"
#include "ranging_result_observer_stub.h"
#include "ranging_state_observer_stub.h"
#include "fusion_ranging_errorcode.h"
#include "log.h"

using namespace OHOS;
using namespace OHOS::FusionRanging;
using namespace testing;
using namespace testing::ext;

class MockRangingResultObserver : public IRangingResultObserver {
public:
    MOCK_METHOD(void, OnRangingResult, (const RangingResult &result), (override));
    MOCK_METHOD(sptr<IRemoteObject>, AsObject, (), (override));
};

class MockRangingStateObserver : public IRangingStateObserver {
public:
    MOCK_METHOD(void, OnRangingStateChanged, (const RangingStateChangeInfo &info), (override));
    MOCK_METHOD(sptr<IRemoteObject>, AsObject, (), (override));
};

class FusionRangingServerTest : public testing::Test {
public:
    FusionRangingServerTest() = default;
    ~FusionRangingServerTest() override = default;

    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    sptr<FusionRangingServer> server_;
    sptr<MockRangingResultObserver> mockResultObserver_;
    sptr<MockRangingStateObserver> mockStateObserver_;
};

void FusionRangingServerTest::SetUpTestCase(void)
{}

void FusionRangingServerTest::TearDownTestCase(void)
{}

void FusionRangingServerTest::SetUp()
{
    server_ = FusionRangingServer::GetInstance();
    mockResultObserver_ = new MockRangingResultObserver();
    mockStateObserver_ = new MockRangingStateObserver();
}

void FusionRangingServerTest::TearDown()
{
    server_->CleanupAll();
    mockResultObserver_ = nullptr;
    mockStateObserver_ = nullptr;
}

/**
 * @tc.name: GetInstanceShouldReturnNonNull
 * @tc.desc: 测试用例1：验证GetInstance返回非空实例
 * @tc.type: FUNC
 */
HWTEST_F(FusionRangingServerTest, GetInstanceShouldReturnNonNull, TestSize.Level0)
{
    auto instance = FusionRangingServer::GetInstance();
    EXPECT_NE(instance, nullptr);
}

/**
 * @tc.name: GetRangingCapabilityShouldSuccess
 * @tc.desc: 测试用例2：验证获取测距能力
 * @tc.type: FUNC
 */
HWTEST_F(FusionRangingServerTest, GetRangingCapabilityShouldSuccess, TestSize.Level0)
{
    RangingCapabilitySupported capability;
    ErrCode result = server_->GetRangingCapability(capability);
    EXPECT_EQ(result, static_cast<int32_t>(RangingErrCode::RANGING_NO_ERROR));
}

/**
 * @tc.name: StartRangingWithNullObserverShouldFail
 * @tc.desc: 测试用例3：验证空observer时StartRanging失败
 * @tc.type: FUNC
 */
HWTEST_F(FusionRangingServerTest, StartRangingWithNullObserverShouldFail, TestSize.Level0)
{
    RangingParams params("00:11:22:33:44:55", RangingRole::ROLE_INITIATOR, RangingTypes::NEARLINK_HADM);
    ErrCode result = server_->StartRanging(params, nullptr);
    EXPECT_EQ(result, static_cast<int32_t>(RangingErrCode::RANGING_ERR_INVALID_PARAM));
}

/**
 * @tc.name: RegisterStateObserverWithNullObserverShouldFail
 * @tc.desc: 测试用例4：验证空observer时注册失败
 * @tc.type: FUNC
 */
HWTEST_F(FusionRangingServerTest, RegisterStateObserverWithNullObserverShouldFail, TestSize.Level0)
{
    ErrCode result = server_->RegisterStateObserver(nullptr);
    EXPECT_EQ(result, static_cast<int32_t>(RangingErrCode::RANGING_ERR_INVALID_PARAM));
}

/**
 * @tc.name: UnregisterStateObserverWithNullObserverShouldFail
 * @tc.desc: 测试用例5：验证空observer时注销失败
 * @tc.type: FUNC
 */
HWTEST_F(FusionRangingServerTest, UnregisterStateObserverWithNullObserverShouldFail, TestSize.Level0)
{
    ErrCode result = server_->UnregisterStateObserver(nullptr);
    EXPECT_EQ(result, static_cast<int32_t>(RangingErrCode::RANGING_ERR_INVALID_PARAM));
}

/**
 * @tc.name: RegisterStateObserverShouldSuccess
 * @tc.desc: 测试用例6：验证注册状态观察者成功
 * @tc.type: FUNC
 */
HWTEST_F(FusionRangingServerTest, RegisterStateObserverShouldSuccess, TestSize.Level0)
{
    EXPECT_CALL(*mockStateObserver_, AsObject()).WillRepeatedly(Return(nullptr));
    
    ErrCode result = server_->RegisterStateObserver(mockStateObserver_);
    EXPECT_EQ(result, static_cast<int32_t>(RangingErrCode::RANGING_NO_ERROR));
}

/**
 * @tc.name: UnregisterStateObserverShouldSuccess
 * @tc.desc: 测试用例7：验证注销状态观察者成功
 * @tc.type: FUNC
 */
HWTEST_F(FusionRangingServerTest, UnregisterStateObserverShouldSuccess, TestSize.Level0)
{
    EXPECT_CALL(*mockStateObserver_, AsObject()).WillRepeatedly(Return(nullptr));
    
    server_->RegisterStateObserver(mockStateObserver_);
    ErrCode result = server_->UnregisterStateObserver(mockStateObserver_);
    EXPECT_EQ(result, static_cast<int32_t>(RangingErrCode::RANGING_NO_ERROR));
}

/**
 * @tc.name: CleanupAllShouldClearResources
 * @tc.desc: 测试用例8：验证清理所有资源
 * @tc.type: FUNC
 */
HWTEST_F(FusionRangingServerTest, CleanupAllShouldClearResources, TestSize.Level0)
{
    EXPECT_CALL(*mockStateObserver_, AsObject()).WillRepeatedly(Return(nullptr));
    
    server_->RegisterStateObserver(mockStateObserver_);
    server_->CleanupAll();
}

/**
 * @tc.name: CheckAndUnloadSAShouldWork
 * @tc.desc: 测试用例9：验证检查卸载SA
 * @tc.type: FUNC
 */
HWTEST_F(FusionRangingServerTest, CheckAndUnloadSAShouldWork, TestSize.Level0)
{
    server_->CheckAndUnloadSA();
}

/**
 * @tc.name: PauseRangingByUidShouldWork
 * @tc.desc: 测试用例10：验证按UID暂停测距
 * @tc.type: FUNC
 */
HWTEST_F(FusionRangingServerTest, PauseRangingByUidShouldWork, TestSize.Level0)
{
    server_->PauseRangingByUid(1000);
}

/**
 * @tc.name: ResumeRangingByUidShouldWork
 * @tc.desc: 测试用例11：验证按UID恢复测距
 * @tc.type: FUNC
 */
HWTEST_F(FusionRangingServerTest, ResumeRangingByUidShouldWork, TestSize.Level0)
{
    server_->ResumeRangingByUid(1000);
}

/**
 * @tc.name: StopRangingByUidShouldWork
 * @tc.desc: 测试用例12：验证按UID停止测距
 * @tc.type: FUNC
 */
HWTEST_F(FusionRangingServerTest, StopRangingByUidShouldWork, TestSize.Level0)
{
    server_->StopRangingByUid(1000);
}

/**
 * @tc.name: ThreadSafetyTest
 * @tc.desc: 测试用例13：线程安全测试
 * @tc.type: FUNC
 */
HWTEST_F(FusionRangingServerTest, ThreadSafetyTest, TestSize.Level0)
{
    auto thread1 = std::thread([&] {
        for (int i = 0; i < 50; ++i) {
            server_->PauseRangingByUid(i);
            server_->ResumeRangingByUid(i);
        }
    });

    auto thread2 = std::thread([&] {
        for (int i = 0; i < 50; ++i) {
            server_->StopRangingByUid(i);
        }
    });

    thread1.join();
    thread2.join();
}