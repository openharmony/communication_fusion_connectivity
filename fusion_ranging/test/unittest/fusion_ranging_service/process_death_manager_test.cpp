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
#define LOG_TAG "ProcessDeathManagerTest"
#endif

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <thread>
#include "process_death_manager.h"
#include "iremote_object.h"
#include "iremote_broker.h"
#include "log.h"

using namespace OHOS;
using namespace OHOS::FusionRanging;
using namespace testing;
using namespace testing::ext;

class MockRemoteObject : public IRemoteObject {
public:
    MockRemoteObject() : IRemoteObject(u"") {}

    MOCK_METHOD(int32_t, SendRequest, (uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option), (override));
    MOCK_METHOD(bool, AddDeathRecipient, (const sptr<IRemoteObject::DeathRecipient> &recipient), (override));
    MOCK_METHOD(bool, RemoveDeathRecipient, (const sptr<IRemoteObject::DeathRecipient> &recipient), (override));
    MOCK_METHOD(sptr<IRemoteBroker>, AsInterface, (), (override));
    MOCK_METHOD(bool, Marshalling, (Parcel &parcel), (const, override));
    MOCK_METHOD(int32_t, GetObjectRefCount, (), (override));
    MOCK_METHOD(int, Dump, (int fd, const std::vector<std::u16string> &args), (override));
};

class ProcessDeathManagerTest : public testing::Test {
public:
    ProcessDeathManagerTest() = default;
    ~ProcessDeathManagerTest() override = default;

    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    std::unique_ptr<ProcessDeathManager> deathManager_;
    sptr<MockRemoteObject> mockRemoteObject_;
    bool callbackCalled_;
    int32_t callbackUid_;
};

void ProcessDeathManagerTest::SetUpTestCase(void) {}

void ProcessDeathManagerTest::TearDownTestCase(void) {}

void ProcessDeathManagerTest::SetUp()
{
    deathManager_ = std::make_unique<ProcessDeathManager>();
    mockRemoteObject_ = new MockRemoteObject();
    callbackCalled_ = false;
    callbackUid_ = 0;

    auto callback = [this](int32_t uid) {
        callbackCalled_ = true;
        callbackUid_ = uid;
    };
    deathManager_->SetDeathCallback(callback);
}

void ProcessDeathManagerTest::TearDown()
{
    deathManager_->ClearAll();
    deathManager_.reset();
    mockRemoteObject_ = nullptr;
}

/**
 * @tc.name: SetDeathCallbackShouldWork
 * @tc.desc: 测试用例1：验证设置死亡回调
 * @tc.type: FUNC
 */
HWTEST_F(ProcessDeathManagerTest, SetDeathCallbackShouldWork, TestSize.Level0)
{
    EXPECT_FALSE(callbackCalled_);

    auto newCallback = [this](int32_t uid) {
        callbackCalled_ = true;
        callbackUid_ = uid + 1000;
    };
    deathManager_->SetDeathCallback(newCallback);
}

/**
 * @tc.name: RegisterProcessDeathWithValidParamsShouldSuccess
 * @tc.desc: 测试用例2：验证注册死亡监听成功
 * @tc.type: FUNC
 */
HWTEST_F(ProcessDeathManagerTest, RegisterProcessDeathWithValidParamsShouldSuccess, TestSize.Level0)
{
    EXPECT_CALL(*mockRemoteObject_, AddDeathRecipient(_)).WillOnce(Return(true));

    bool result = deathManager_->RegisterProcessDeath(1000, mockRemoteObject_);
    EXPECT_TRUE(result);
    EXPECT_TRUE(deathManager_->HasProcessDeathHandler(1000));
}

/**
 * @tc.name: RegisterProcessDeathWithNullRemoteObjectShouldFail
 * @tc.desc: 测试用例3：验证空RemoteObject注册失败
 * @tc.type: FUNC
 */
HWTEST_F(ProcessDeathManagerTest, RegisterProcessDeathWithNullRemoteObjectShouldFail, TestSize.Level0)
{
    bool result = deathManager_->RegisterProcessDeath(1000, nullptr);
    EXPECT_FALSE(result);
    EXPECT_FALSE(deathManager_->HasProcessDeathHandler(1000));
}

/**
 * @tc.name: RegisterProcessDeathWhenAddDeathRecipientFailShouldFail
 * @tc.desc: 测试用例4：验证添加死亡监听失败时注册失败
 * @tc.type: FUNC
 */
HWTEST_F(ProcessDeathManagerTest, RegisterProcessDeathWhenAddDeathRecipientFailShouldFail, TestSize.Level0)
{
    EXPECT_CALL(*mockRemoteObject_, AddDeathRecipient(_)).WillOnce(Return(false));

    bool result = deathManager_->RegisterProcessDeath(1000, mockRemoteObject_);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: RegisterDuplicateUidShouldFail
 * @tc.desc: 测试用例5：验证重复UID注册失败
 * @tc.type: FUNC
 */
HWTEST_F(ProcessDeathManagerTest, RegisterDuplicateUidShouldFail, TestSize.Level0)
{
    EXPECT_CALL(*mockRemoteObject_, AddDeathRecipient(_)).WillOnce(Return(true));
    deathManager_->RegisterProcessDeath(1000, mockRemoteObject_);

    bool result = deathManager_->RegisterProcessDeath(1000, mockRemoteObject_);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: UnregisterProcessDeathShouldSuccess
 * @tc.desc: 测试用例6：验证注销死亡监听成功
 * @tc.type: FUNC
 */
HWTEST_F(ProcessDeathManagerTest, UnregisterProcessDeathShouldSuccess, TestSize.Level0)
{
    EXPECT_CALL(*mockRemoteObject_, AddDeathRecipient(_)).WillOnce(Return(true));
    EXPECT_CALL(*mockRemoteObject_, RemoveDeathRecipient(_)).WillOnce(Return(true));

    deathManager_->RegisterProcessDeath(1000, mockRemoteObject_);
    bool result = deathManager_->UnregisterProcessDeath(1000);
    EXPECT_TRUE(result);
    EXPECT_FALSE(deathManager_->HasProcessDeathHandler(1000));
}

/**
 * @tc.name: UnregisterNonExistingUidShouldFail
 * @tc.desc: 测试用例7：验证注销不存在UID失败
 * @tc.type: FUNC
 */
HWTEST_F(ProcessDeathManagerTest, UnregisterNonExistingUidShouldFail, TestSize.Level0)
{
    bool result = deathManager_->UnregisterProcessDeath(1000);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: HasProcessDeathHandlerShouldReturnCorrectResult
 * @tc.desc: 测试用例8：验证死亡监听存在检查
 * @tc.type: FUNC
 */
HWTEST_F(ProcessDeathManagerTest, HasProcessDeathHandlerShouldReturnCorrectResult, TestSize.Level0)
{
    EXPECT_FALSE(deathManager_->HasProcessDeathHandler(1000));

    EXPECT_CALL(*mockRemoteObject_, AddDeathRecipient(_)).WillOnce(Return(true));
    deathManager_->RegisterProcessDeath(1000, mockRemoteObject_);
    EXPECT_TRUE(deathManager_->HasProcessDeathHandler(1000));
}

/**
 * @tc.name: ClearAllShouldRemoveAllHandlers
 * @tc.desc: 测试用例9：验证清空所有死亡监听
 * @tc.type: FUNC
 */
HWTEST_F(ProcessDeathManagerTest, ClearAllShouldRemoveAllHandlers, TestSize.Level0)
{
    EXPECT_CALL(*mockRemoteObject_, AddDeathRecipient(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockRemoteObject_, RemoveDeathRecipient(_)).WillRepeatedly(Return(true));

    deathManager_->RegisterProcessDeath(1000, mockRemoteObject_);
    deathManager_->RegisterProcessDeath(2000, mockRemoteObject_);

    deathManager_->ClearAll();
    EXPECT_FALSE(deathManager_->HasProcessDeathHandler(1000));
    EXPECT_FALSE(deathManager_->HasProcessDeathHandler(2000));
}

/**
 * @tc.name: ThreadSafetyTest
 * @tc.desc: 测试用例10：线程安全测试
 * @tc.type: FUNC
 */
HWTEST_F(ProcessDeathManagerTest, ThreadSafetyTest, TestSize.Level0)
{
    EXPECT_CALL(*mockRemoteObject_, AddDeathRecipient(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockRemoteObject_, RemoveDeathRecipient(_)).WillRepeatedly(Return(true));

    auto thread1 = std::thread([&] {
        for (int i = 0; i < 50; ++i) {
            deathManager_->RegisterProcessDeath(i, mockRemoteObject_);
        }
    });

    auto thread2 = std::thread([&] {
        for (int i = 0; i < 50; ++i) {
            deathManager_->UnregisterProcessDeath(i);
        }
    });

    thread1.join();
    thread2.join();
}