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
#define LOG_TAG "RangingAdapterFactoryTest"
#endif

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <thread>

#include "ranging_adapter_factory.h"
#include "base_ranging_adapter.h"
#include "fusion_ranging_types.h"
#include "log.h"

using namespace OHOS;
using namespace OHOS::FusionRanging;
using namespace testing;
using namespace testing::ext;

class MockRangingAdapter : public BaseRangingAdapter {
public:
    MOCK_METHOD(int, Init, (), (override));
    MOCK_METHOD(int, DeInit, (), (override));
    MOCK_METHOD(int, StartRanging, (const std::string &deviceId), (override));
    MOCK_METHOD(int, StopRanging, (const std::string &deviceId), (override));
    MOCK_METHOD(int, StartPassiveRanging, (int32_t &handle), (override));
    MOCK_METHOD(int, StopPassiveRanging, (int32_t handle), (override));
    MOCK_METHOD(int, PauseRanging, (const std::string &deviceId), (override));
    MOCK_METHOD(int, ResumeRanging, (const std::string &deviceId), (override));
    MOCK_METHOD(int, SetCallback, (const std::shared_ptr<BaseRangingAdapterCallback> &callback), (override));
};

class RangingAdapterFactoryTest : public testing::Test {
public:
    RangingAdapterFactoryTest() = default;
    ~RangingAdapterFactoryTest() override = default;

    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void RangingAdapterFactoryTest::SetUpTestCase(void) {}

void RangingAdapterFactoryTest::TearDownTestCase(void) {}

void RangingAdapterFactoryTest::SetUp() {}

void RangingAdapterFactoryTest::TearDown() {}

/*
 * Precondition: None
 * Test Steps: Call Instance() multiple times to get singleton
 * Expected Result: All calls return the same address
 */
HWTEST_F(RangingAdapterFactoryTest, InstanceShouldReturnSingleton, TestSize.Level0)
{
    // Given: No precondition needed

    // When: Get factory instance twice
    auto &instance1 = RangingAdapterFactory::Instance();
    auto &instance2 = RangingAdapterFactory::Instance();

    // Then: Verify both instances are the same singleton
    EXPECT_EQ(&instance1, &instance2);
}

/*
 * Precondition: None
 * Test Steps: Call CreateRangingAdapter with unregistered type
 * Expected Result: Return null pointer
 */
HWTEST_F(RangingAdapterFactoryTest, CreateRangingAdapterForUnregisteredTypeShouldReturnNull, TestSize.Level0)
{
    // Given: Factory instance is available
    auto &factory = RangingAdapterFactory::Instance();

    // When: Create adapter for unregistered type (999)
    auto adapter = factory.CreateRangingAdapter(static_cast<RangingTypes>(999));

    // Then: Verify return value is null
    EXPECT_EQ(adapter, nullptr);
}

/*
 * Precondition: None
 * Test Steps: Call IsRangingAdapterSupported with unregistered type
 * Expected Result: Returns false
 */
HWTEST_F(RangingAdapterFactoryTest, IsRangingAdapterSupportedForUnregisteredTypeShouldReturnFalse, TestSize.Level0)
{
    // Given: Factory instance is available
    auto &factory = RangingAdapterFactory::Instance();

    // When: Check support status for unregistered type (999)
    bool supported = factory.IsRangingAdapterSupported(static_cast<RangingTypes>(999));

    // Then: Verify unsupported
    EXPECT_FALSE(supported);
}

/*
 * Precondition: None
 * Test Steps: Register adapter and checker, then check support status
 * Expected Result: Registered type is supported
 */
HWTEST_F(RangingAdapterFactoryTest, RegisterRangingAdapterShouldWork, TestSize.Level0)
{
    // Given: Factory instance is available
    auto &factory = RangingAdapterFactory::Instance();

    // When: Register adapter and checker for NEARLINK_HADM type
    auto generator = []() -> std::shared_ptr<BaseRangingAdapter> {
        return std::make_shared<MockRangingAdapter>();
    };
    factory.RegisterRangingAdapter<MockRangingAdapter>(RangingTypes::NEARLINK_HADM, generator);
    factory.RegisterChecker(RangingTypes::NEARLINK_HADM, []() { return true; });

    // Then: Verify type is supported
    bool supported = factory.IsRangingAdapterSupported(RangingTypes::NEARLINK_HADM);
    EXPECT_TRUE(supported);
}

/*
 * Precondition: None
 * Test Steps: Register adapter then create adapter instance
 * Expected Result: CreateRangingAdapter returns non-null adapter
 */
HWTEST_F(RangingAdapterFactoryTest, CreateRangingAdapterAfterRegisterShouldReturnAdapter, TestSize.Level0)
{
    // Given: Factory instance is available
    auto &factory = RangingAdapterFactory::Instance();

    // When: Register adapter for NEARLINK_HADM type
    auto generator = []() -> std::shared_ptr<BaseRangingAdapter> {
        return std::make_shared<MockRangingAdapter>();
    };
    factory.RegisterRangingAdapter<MockRangingAdapter>(RangingTypes::NEARLINK_HADM, generator);

    // Then: Create adapter and verify non-null
    auto adapter = factory.CreateRangingAdapter(RangingTypes::NEARLINK_HADM);
    EXPECT_NE(adapter, nullptr);
}

/*
 * Precondition: None
 * Test Steps: Register checker with null, then check support status
 * Expected Result: Returns false
 */
HWTEST_F(RangingAdapterFactoryTest, RegisterCheckerWithNullShouldReturnFalse, TestSize.Level0)
{
    // Given: Factory instance is available
    auto &factory = RangingAdapterFactory::Instance();
    RangingTypes testType = static_cast<RangingTypes>(100);

    // When: Register null checker for test type
    factory.RegisterChecker(testType, nullptr);

    // Then: Verify type is not supported
    bool supported = factory.IsRangingAdapterSupported(testType);
    EXPECT_FALSE(supported);
}

/*
 * Precondition: None
 * Test Steps: Register checker with custom logic and verify support status changes
 * Expected Result: Support status reflects checker return value
 */
HWTEST_F(RangingAdapterFactoryTest, RegisterCheckerWithCustomCheckerShouldWork, TestSize.Level0)
{
    // Given: Factory instance is available
    auto &factory = RangingAdapterFactory::Instance();
    RangingTypes testType = static_cast<RangingTypes>(101);

    // When: Register checker that returns false, then change to true
    bool customResult = false;
    factory.RegisterChecker(testType, [&customResult]() { return customResult; });

    // Then: Verify checker result is reflected in support status
    customResult = false;
    EXPECT_FALSE(factory.IsRangingAdapterSupported(testType));

    customResult = true;
    EXPECT_TRUE(factory.IsRangingAdapterSupported(testType));
}

/*
 * Precondition: None
 * Test Steps: Create AutoRegisterRangingAdapter and verify auto-registration
 * Expected Result: Adapter is available after auto-registration
 */
HWTEST_F(RangingAdapterFactoryTest, AutoRegisterRangingAdapterShouldAutoRegister, TestSize.Level0)
{
    // Given: Factory instance is available
    auto &factory = RangingAdapterFactory::Instance();
    RangingTypes testType = static_cast<RangingTypes>(102);

    // When: Create AutoRegisterRangingAdapter for test type
    AutoRegisterRangingAdapter<MockRangingAdapter> autoRegister(testType);

    // Then: Verify adapter is registered and can be created
    EXPECT_FALSE(factory.IsRangingAdapterSupported(testType));
    auto adapter = factory.CreateRangingAdapter(testType);
    EXPECT_NE(adapter, nullptr);
}

/*
 * Precondition: None
 * Test Steps: Create AutoRegisterRangingAdapter with custom checker
 * Expected Result: Adapter is supported after auto-registration
 */
HWTEST_F(RangingAdapterFactoryTest, AutoRegisterRangingAdapterWithCheckerShouldWork, TestSize.Level0)
{
    // Given: Factory instance is available
    auto &factory = RangingAdapterFactory::Instance();
    RangingTypes testType = static_cast<RangingTypes>(103);
    auto checker = []() {
        return true;
    };

    // When: Create AutoRegisterRangingAdapter with checker
    AutoRegisterRangingAdapter<MockRangingAdapter> autoRegister(testType, nullptr, checker);

    // Then: Verify adapter is supported and can be created
    EXPECT_TRUE(factory.IsRangingAdapterSupported(testType));
    auto adapter = factory.CreateRangingAdapter(testType);
    EXPECT_NE(adapter, nullptr);
}

/*
 * Precondition: None
 * Test Steps: Multi-thread concurrent access to factory methods
 * Expected Result: No crash or deadlock, test completes normally
 */
HWTEST_F(RangingAdapterFactoryTest, ThreadSafetyTest, TestSize.Level0)
{
    // Given: Factory instance is available
    auto &factory = RangingAdapterFactory::Instance();

    // When: Create two threads to concurrently access factory
    auto thread1 = std::thread([&factory] {
        for (int i = 0; i < 100; ++i) {
            factory.IsRangingAdapterSupported(RangingTypes::NEARLINK_HADM);
        }
    });

    auto thread2 = std::thread([&factory] {
        for (int i = 0; i < 100; ++i) {
            factory.CreateRangingAdapter(RangingTypes::NEARLINK_HADM);
        }
    });

    // Then: Wait for threads to complete
    thread1.join();
    thread2.join();
    SUCCEED();
}