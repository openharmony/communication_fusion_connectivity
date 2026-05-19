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
    MockRemoteObject() : IRemoteObject(u"")
    {
        ON_CALL(*this, AddDeathRecipient(_)).WillByDefault(Return(true));
        ON_CALL(*this, RemoveDeathRecipient(_)).WillByDefault(Return(true));
    }

    MOCK_METHOD(int32_t, SendRequest, (uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option),
                (override));
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
};

static std::atomic<bool> mockSetup_{false};

void ProcessDeathManagerTest::SetUpTestCase(void)
{
    if (!mockSetup_) {
        mockSetup_ = true;
    }
}

void ProcessDeathManagerTest::TearDownTestCase(void) {}

void ProcessDeathManagerTest::SetUp()
{
    deathManager_ = std::make_unique<ProcessDeathManager>();
    mockRemoteObject_ = new MockRemoteObject();
    EXPECT_CALL(*mockRemoteObject_, RemoveDeathRecipient(_)).WillRepeatedly(Return(true));
}

void ProcessDeathManagerTest::TearDown()
{
    deathManager_->ClearAll();
    deathManager_.reset();
    mockRemoteObject_ = nullptr;
}

/*
 * Precondition: MockRemoteObject is available
 * Test Steps: Register process death with valid parameters
 * Expected Result: Registration succeeds and handler exists
 */
HWTEST_F(ProcessDeathManagerTest, RegisterProcessDeathWithValidParamsShouldSuccess, TestSize.Level0)
{
    // Given: Mock setup for AddDeathRecipient
    EXPECT_CALL(*mockRemoteObject_, AddDeathRecipient(_)).WillOnce(Return(true));

    // When: Register process death for UID 1000
    bool result = deathManager_->RegisterProcessDeath(1000, mockRemoteObject_);

    // Then: Verify registration succeeds and handler exists
    EXPECT_TRUE(result);
    EXPECT_TRUE(deathManager_->HasProcessDeathHandler(1000));
}

/*
 * Precondition: None
 * Test Steps: Register process death with null RemoteObject
 * Expected Result: Registration fails and no handler exists
 */
HWTEST_F(ProcessDeathManagerTest, RegisterProcessDeathWithNullRemoteObjectShouldFail, TestSize.Level0)
{
    // Given: No precondition needed

    // When: Register process death with null RemoteObject
    bool result = deathManager_->RegisterProcessDeath(1000, nullptr);

    // Then: Verify registration fails
    EXPECT_FALSE(result);
    EXPECT_FALSE(deathManager_->HasProcessDeathHandler(1000));
}

/*
 * Precondition: MockRemoteObject is available
 * Test Steps: Register when AddDeathRecipient returns false
 * Expected Result: Registration fails
 */
HWTEST_F(ProcessDeathManagerTest, RegisterProcessDeathWhenAddDeathRecipientFailShouldFail, TestSize.Level0)
{
    // Given: Mock setup for AddDeathRecipient to fail
    EXPECT_CALL(*mockRemoteObject_, AddDeathRecipient(_)).WillOnce(Return(false));

    // When: Register process death for UID 1000
    bool result = deathManager_->RegisterProcessDeath(1000, mockRemoteObject_);

    // Then: Verify registration fails
    EXPECT_FALSE(result);
}

/*
 * Precondition: UID 1000 has been registered
 * Test Steps: Register same UID again
 * Expected Result: Second registration succeeds (idempotent operation)
 */
HWTEST_F(ProcessDeathManagerTest, RegisterDuplicateUidShouldSucceed, TestSize.Level0)
{
    // Given: First registration for UID 1000 succeeds
    EXPECT_CALL(*mockRemoteObject_, AddDeathRecipient(_)).WillOnce(Return(true));
    deathManager_->RegisterProcessDeath(1000, mockRemoteObject_);

    // When: Register same UID again
    bool result = deathManager_->RegisterProcessDeath(1000, mockRemoteObject_);

    // Then: Verify second registration succeeds (idempotent behavior)
    EXPECT_TRUE(result);
}

/*
 * Precondition: UID 1000 has been registered
 * Test Steps: Deregister the registered UID
 * Expected Result: Deregistration succeeds and handler is removed
 */
HWTEST_F(ProcessDeathManagerTest, UnregisterProcessDeathShouldSuccess, TestSize.Level0)
{
    // Given: Registration for UID 1000
    EXPECT_CALL(*mockRemoteObject_, AddDeathRecipient(_)).WillOnce(Return(true));
    EXPECT_CALL(*mockRemoteObject_, RemoveDeathRecipient(_)).WillOnce(Return(true));
    deathManager_->RegisterProcessDeath(1000, mockRemoteObject_);

    // When: Deregister process death for UID 1000
    bool result = deathManager_->DeregisterProcessDeath(1000);

    // Then: Verify deregistration succeeds and handler is removed
    EXPECT_TRUE(result);
    EXPECT_FALSE(deathManager_->HasProcessDeathHandler(1000));
}

/*
 * Precondition: None (UID 1000 not registered)
 * Test Steps: Deregister non-existing UID
 * Expected Result: Deregistration fails
 */
HWTEST_F(ProcessDeathManagerTest, UnregisterNonExistingUidShouldFail, TestSize.Level0)
{
    // Given: UID 1000 is not registered

    // When: Deregister non-existing UID 1000
    bool result = deathManager_->DeregisterProcessDeath(1000);

    // Then: Verify deregistration fails
    EXPECT_FALSE(result);
}

/*
 * Precondition: None or UID registered
 * Test Steps: Check handler existence before and after registration
 * Expected Result: Handler check returns correct status
 */
HWTEST_F(ProcessDeathManagerTest, HasProcessDeathHandlerShouldReturnCorrectResult, TestSize.Level0)
{
    // Given: UID 1000 is not registered

    // When: Check handler existence before registration
    // Then: Verify no handler exists
    EXPECT_FALSE(deathManager_->HasProcessDeathHandler(1000));

    // When: Register UID 1000
    EXPECT_CALL(*mockRemoteObject_, AddDeathRecipient(_)).WillOnce(Return(true));
    deathManager_->RegisterProcessDeath(1000, mockRemoteObject_);

    // Then: Verify handler exists after registration
    EXPECT_TRUE(deathManager_->HasProcessDeathHandler(1000));
}

/*
 * Precondition: Multiple UIDs have been registered
 * Test Steps: Call ClearAll to remove all handlers
 * Expected Result: All handlers are removed
 */
HWTEST_F(ProcessDeathManagerTest, ClearAllShouldRemoveAllHandlers, TestSize.Level0)
{
    // Given: Register multiple UIDs (1000 and 2000)
    EXPECT_CALL(*mockRemoteObject_, AddDeathRecipient(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockRemoteObject_, RemoveDeathRecipient(_)).WillRepeatedly(Return(true));
    deathManager_->RegisterProcessDeath(1000, mockRemoteObject_);
    deathManager_->RegisterProcessDeath(2000, mockRemoteObject_);

    // When: Clear all handlers
    deathManager_->ClearAll();

    // Then: Verify all handlers are removed
    EXPECT_FALSE(deathManager_->HasProcessDeathHandler(1000));
    EXPECT_FALSE(deathManager_->HasProcessDeathHandler(2000));
}

/*
 * Precondition: None
 * Test Steps: Multi-thread concurrent registration and deregistration
 * Expected Result: No crash or deadlock, test completes normally
 */
HWTEST_F(ProcessDeathManagerTest, ThreadSafetyTest, TestSize.Level0)
{
    // Given: Mock setup for AddDeathRecipient and RemoveDeathRecipient
    EXPECT_CALL(*mockRemoteObject_, AddDeathRecipient(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockRemoteObject_, RemoveDeathRecipient(_)).WillRepeatedly(Return(true));

    // When: Create two threads to concurrently register and deregister
    auto thread1 = std::thread([this] {
        for (int i = 0; i < 50; ++i) {
            deathManager_->RegisterProcessDeath(i, mockRemoteObject_);
        }
    });

    auto thread2 = std::thread([this] {
        for (int i = 0; i < 50; ++i) {
            deathManager_->DeregisterProcessDeath(i);
        }
    });

    // Then: Wait for threads to complete
    thread1.join();
    thread2.join();
    SUCCEED();
}