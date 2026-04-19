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
#define LOG_TAG "StateObserverRegistryTest"
#endif

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <thread>
#include "state_observer_registry.h"
#include "ranging_state_observer_stub.h"
#include "ranging_state_change_info.h"
#include "log.h"

using namespace OHOS;
using namespace OHOS::FusionRanging;
using namespace testing;
using namespace testing::ext;

class MockRangingStateObserver : public IRangingStateObserver {
public:
    MOCK_METHOD(void, OnRangingStateChanged, (const RangingStateChangeInfo &info), (override));
    MOCK_METHOD(sptr<IRemoteObject>, AsObject, (), (override));
};

class StateObserverRegistryTest : public testing::Test {
public:
    StateObserverRegistryTest() = default;
    ~StateObserverRegistryTest() override = default;

    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    std::unique_ptr<StateObserverRegistry> registry_;
    sptr<MockRangingStateObserver> mockObserver_;
};

void StateObserverRegistryTest::SetUpTestCase(void)
{}

void StateObserverRegistryTest::TearDownTestCase(void)
{}

void StateObserverRegistryTest::SetUp()
{
    registry_ = std::make_unique<StateObserverRegistry>();
    mockObserver_ = new MockRangingStateObserver();
}

void StateObserverRegistryTest::TearDown()
{
    registry_->ClearAll();
    registry_.reset();
    mockObserver_ = nullptr;
}

/**
 * @tc.name: RegisterWithValidObserverShouldSuccess
 * @tc.desc: 测试用例1：验证注册观察者成功
 * @tc.type: FUNC
 */
HWTEST_F(StateObserverRegistryTest, RegisterWithValidObserverShouldSuccess, TestSize.Level0)
{
    bool result = registry_->Register(1000, mockObserver_);
    EXPECT_TRUE(result);
    EXPECT_EQ(registry_->GetObserverCount(), 1);
}

/**
 * @tc.name: RegisterWithNullObserverShouldFail
 * @tc.desc: 测试用例2：验证空观察者注册失败
 * @tc.type: FUNC
 */
HWTEST_F(StateObserverRegistryTest, RegisterWithNullObserverShouldFail, TestSize.Level0)
{
    bool result = registry_->Register(1000, nullptr);
    EXPECT_FALSE(result);
    EXPECT_EQ(registry_->GetObserverCount(), 0);
}

/**
 * @tc.name: RegisterDuplicateUidShouldFail
 * @tc.desc: 测试用例3：验证重复UID注册失败
 * @tc.type: FUNC
 */
HWTEST_F(StateObserverRegistryTest, RegisterDuplicateUidShouldFail, TestSize.Level0)
{
    registry_->Register(1000, mockObserver_);
    bool result = registry_->Register(1000, mockObserver_);
    EXPECT_FALSE(result);
    EXPECT_EQ(registry_->GetObserverCount(), 1);
}

/**
 * @tc.name: UnregisterShouldSuccess
 * @tc.desc: 测试用例4：验证注销观察者成功
 * @tc.type: FUNC
 */
HWTEST_F(StateObserverRegistryTest, UnregisterShouldSuccess, TestSize.Level0)
{
    registry_->Register(1000, mockObserver_);
    bool result = registry_->Unregister(1000);
    EXPECT_TRUE(result);
    EXPECT_EQ(registry_->GetObserverCount(), 0);
}

/**
 * @tc.name: UnregisterNonExistingUidShouldFail
 * @tc.desc: 测试用例5：验证注销不存在UID失败
 * @tc.type: FUNC
 */
HWTEST_F(StateObserverRegistryTest, UnregisterNonExistingUidShouldFail, TestSize.Level0)
{
    bool result = registry_->Unregister(1000);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: FindShouldReturnCorrectObserver
 * @tc.desc: 测试用例6：验证查找观察者
 * @tc.type: FUNC
 */
HWTEST_F(StateObserverRegistryTest, FindShouldReturnCorrectObserver, TestSize.Level0)
{
    registry_->Register(1000, mockObserver_);
    
    auto observer = registry_->Find(1000);
    EXPECT_NE(observer, nullptr);
    
    observer = registry_->Find(2000);
    EXPECT_EQ(observer, nullptr);
}

/**
 * @tc.name: GetAllUidsShouldReturnCorrectUids
 * @tc.desc: 测试用例7：验证获取所有UID
 * @tc.type: FUNC
 */
HWTEST_F(StateObserverRegistryTest, GetAllUidsShouldReturnCorrectUids, TestSize.Level0)
{
    registry_->Register(1000, mockObserver_);
    registry_->Register(2000, mockObserver_);
    
    auto uids = registry_->GetAllUids();
    EXPECT_EQ(uids.size(), 2);
}

/**
 * @tc.name: HasObserverShouldReturnCorrectResult
 * @tc.desc: 测试用例8：验证观察者存在检查
 * @tc.type: FUNC
 */
HWTEST_F(StateObserverRegistryTest, HasObserverShouldReturnCorrectResult, TestSize.Level0)
{
    EXPECT_FALSE(registry_->HasObserver(1000));
    
    registry_->Register(1000, mockObserver_);
    EXPECT_TRUE(registry_->HasObserver(1000));
}

/**
 * @tc.name: GetObserverCountShouldReturnCorrectCount
 * @tc.desc: 测试用例9：验证获取观察者数量
 * @tc.type: FUNC
 */
HWTEST_F(StateObserverRegistryTest, GetObserverCountShouldReturnCorrectCount, TestSize.Level0)
{
    EXPECT_EQ(registry_->GetObserverCount(), 0);
    
    registry_->Register(1000, mockObserver_);
    EXPECT_EQ(registry_->GetObserverCount(), 1);
    
    registry_->Register(2000, mockObserver_);
    EXPECT_EQ(registry_->GetObserverCount(), 2);
}

/**
 * @tc.name: ClearAllShouldRemoveAllObservers
 * @tc.desc: 测试用例10：验证清空所有观察者
 * @tc.type: FUNC
 */
HWTEST_F(StateObserverRegistryTest, ClearAllShouldRemoveAllObservers, TestSize.Level0)
{
    registry_->Register(1000, mockObserver_);
    registry_->Register(2000, mockObserver_);
    
    registry_->ClearAll();
    EXPECT_EQ(registry_->GetObserverCount(), 0);
}

/**
 * @tc.name: ThreadSafetyTest
 * @tc.desc: 测试用例11：线程安全测试
 * @tc.type: FUNC
 */
HWTEST_F(StateObserverRegistryTest, ThreadSafetyTest, TestSize.Level0)
{
    auto thread1 = std::thread([&] {
        for (int i = 0; i < 100; ++i) {
            registry_->Register(i, mockObserver_);
        }
    });

    auto thread2 = std::thread([&] {
        for (int i = 0; i < 100; ++i) {
            registry_->Unregister(i);
        }
    });

    thread1.join();
    thread2.join();
}