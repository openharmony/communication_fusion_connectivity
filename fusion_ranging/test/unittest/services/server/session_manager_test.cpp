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
#define LOG_TAG "SessionManagerTest"
#endif

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <thread>
#include "session_manager.h"
#include "ranging_result_observer_stub.h"
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

class SessionManagerTest : public testing::Test {
public:
    SessionManagerTest() = default;
    ~SessionManagerTest() override = default;

    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    std::unique_ptr<SessionManager> sessionManager_;
    sptr<MockRangingResultObserver> mockObserver_;
};

void SessionManagerTest::SetUpTestCase(void)
{}

void SessionManagerTest::TearDownTestCase(void)
{}

void SessionManagerTest::SetUp()
{
    sessionManager_ = std::make_unique<SessionManager>();
    mockObserver_ = new MockRangingResultObserver();
}

void SessionManagerTest::TearDown()
{
    sessionManager_->ClearAll();
    sessionManager_.reset();
    mockObserver_ = nullptr;
}

/**
 * @tc.name: AddSessionWithValidParamsShouldSuccess
 * @tc.desc: 测试用例1：验证添加会话成功
 * @tc.type: FUNC
 */
HWTEST_F(SessionManagerTest, AddSessionWithValidParamsShouldSuccess, TestSize.Level0)
{
    EXPECT_CALL(*mockObserver_, AsObject()).WillRepeatedly(Return(nullptr));
    
    bool result = sessionManager_->AddSession(1000, "00:11:22:33:44:55", mockObserver_);
    EXPECT_TRUE(result);
    EXPECT_EQ(sessionManager_->GetSessionCount(), 1);
}

/**
 * @tc.name: AddSessionWithEmptyDeviceIdShouldFail
 * @tc.desc: 测试用例2：验证空设备ID添加会话失败
 * @tc.type: FUNC
 */
HWTEST_F(SessionManagerTest, AddSessionWithEmptyDeviceIdShouldFail, TestSize.Level0)
{
    bool result = sessionManager_->AddSession(1000, "", mockObserver_);
    EXPECT_FALSE(result);
    EXPECT_EQ(sessionManager_->GetSessionCount(), 0);
}

/**
 * @tc.name: AddSessionWithNullObserverShouldFail
 * @tc.desc: 测试用例3：验证空observer添加会话失败
 * @tc.type: FUNC
 */
HWTEST_F(SessionManagerTest, AddSessionWithNullObserverShouldFail, TestSize.Level0)
{
    bool result = sessionManager_->AddSession(1000, "00:11:22:33:44:55", nullptr);
    EXPECT_FALSE(result);
    EXPECT_EQ(sessionManager_->GetSessionCount(), 0);
}

/**
 * @tc.name: AddDuplicateSessionShouldFail
 * @tc.desc: 测试用例4：验证重复添加会话失败
 * @tc.type: FUNC
 */
HWTEST_F(SessionManagerTest, AddDuplicateSessionShouldFail, TestSize.Level0)
{
    sessionManager_->AddSession(1000, "00:11:22:33:44:55", mockObserver_);
    bool result = sessionManager_->AddSession(2000, "00:11:22:33:44:55", mockObserver_);
    EXPECT_FALSE(result);
    EXPECT_EQ(sessionManager_->GetSessionCount(), 1);
}

/**
 * @tc.name: RemoveSessionShouldSuccess
 * @tc.desc: 测试用例5：验证移除会话成功
 * @tc.type: FUNC
 */
HWTEST_F(SessionManagerTest, RemoveSessionShouldSuccess, TestSize.Level0)
{
    sessionManager_->AddSession(1000, "00:11:22:33:44:55", mockObserver_);
    sessionManager_->RemoveSession("00:11:22:33:44:55");
    EXPECT_EQ(sessionManager_->GetSessionCount(), 0);
    EXPECT_FALSE(sessionManager_->HasSession("00:11:22:33:44:55"));
}

/**
 * @tc.name: RemoveSessionByUidShouldSuccess
 * @tc.desc: 测试用例6：验证按UID移除会话成功
 * @tc.type: FUNC
 */
HWTEST_F(SessionManagerTest, RemoveSessionByUidShouldSuccess, TestSize.Level0)
{
    sessionManager_->AddSession(1000, "00:11:22:33:44:55", mockObserver_);
    sessionManager_->AddSession(2000, "00:11:22:33:44:56", mockObserver_);
    sessionManager_->RemoveSessionByUid(1000);
    EXPECT_EQ(sessionManager_->GetSessionCount(), 1);
    EXPECT_FALSE(sessionManager_->HasSession("00:11:22:33:44:55"));
    EXPECT_TRUE(sessionManager_->HasSession("00:11:22:33:44:56"));
}

/**
 * @tc.name: GetSessionKeysByUidShouldReturnCorrectKeys
 * @tc.desc: 测试用例7：验证按UID获取会话键
 * @tc.type: FUNC
 */
HWTEST_F(SessionManagerTest, GetSessionKeysByUidShouldReturnCorrectKeys, TestSize.Level0)
{
    sessionManager_->AddSession(1000, "00:11:22:33:44:55", mockObserver_);
    sessionManager_->AddSession(1000, "00:11:22:33:44:56", mockObserver_);
    sessionManager_->AddSession(2000, "00:11:22:33:44:57", mockObserver_);
    
    auto keys = sessionManager_->GetSessionKeysByUid(1000);
    EXPECT_EQ(keys.size(), 2);
    
    keys = sessionManager_->GetSessionKeysByUid(2000);
    EXPECT_EQ(keys.size(), 1);
}

/**
 * @tc.name: GetAllSessionKeysShouldReturnAllKeys
 * @tc.desc: 测试用例8：验证获取所有会话键
 * @tc.type: FUNC
 */
HWTEST_F(SessionManagerTest, GetAllSessionKeysShouldReturnAllKeys, TestSize.Level0)
{
    sessionManager_->AddSession(1000, "00:11:22:33:44:55", mockObserver_);
    sessionManager_->AddSession(2000, "00:11:22:33:44:56", mockObserver_);
    
    auto keys = sessionManager_->GetAllSessionKeys();
    EXPECT_EQ(keys.size(), 2);
}

/**
 * @tc.name: HasSessionShouldReturnCorrectResult
 * @tc.desc: 测试用例9：验证会话存在检查
 * @tc.type: FUNC
 */
HWTEST_F(SessionManagerTest, HasSessionShouldReturnCorrectResult, TestSize.Level0)
{
    EXPECT_FALSE(sessionManager_->HasSession("00:11:22:33:44:55"));
    
    sessionManager_->AddSession(1000, "00:11:22:33:44:55", mockObserver_);
    EXPECT_TRUE(sessionManager_->HasSession("00:11:22:33:44:55"));
}

/**
 * @tc.name: HasSessionsByUidShouldReturnCorrectResult
 * @tc.desc: 测试用例10：验证按UID检查会话存在
 * @tc.type: FUNC
 */
HWTEST_F(SessionManagerTest, HasSessionsByUidShouldReturnCorrectResult, TestSize.Level0)
{
    EXPECT_FALSE(sessionManager_->HasSessionsByUid(1000));
    
    sessionManager_->AddSession(1000, "00:11:22:33:44:55", mockObserver_);
    EXPECT_TRUE(sessionManager_->HasSessionsByUid(1000));
    EXPECT_FALSE(sessionManager_->HasSessionsByUid(2000));
}

/**
 * @tc.name: ClearAllShouldRemoveAllSessions
 * @tc.desc: 测试用例11：验证清空所有会话
 * @tc.type: FUNC
 */
HWTEST_F(SessionManagerTest, ClearAllShouldRemoveAllSessions, TestSize.Level0)
{
    sessionManager_->AddSession(1000, "00:11:22:33:44:55", mockObserver_);
    sessionManager_->AddSession(2000, "00:11:22:33:44:56", mockObserver_);
    
    sessionManager_->ClearAll();
    EXPECT_EQ(sessionManager_->GetSessionCount(), 0);
}

/**
 * @tc.name: ThreadSafetyTest
 * @tc.desc: 测试用例12：线程安全测试
 * @tc.type: FUNC
 */
HWTEST_F(SessionManagerTest, ThreadSafetyTest, TestSize.Level0)
{
    auto thread1 = std::thread([&] {
        for (int i = 0; i < 100; ++i) {
            sessionManager_->AddSession(i, "device_" + std::to_string(i), mockObserver_);
        }
    });

    auto thread2 = std::thread([&] {
        for (int i = 0; i < 100; ++i) {
            sessionManager_->RemoveSession("device_" + std::to_string(i));
        }
    });

    thread1.join();
    thread2.join();
}