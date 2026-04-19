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
    MOCK_METHOD(int, StartRanging, (const std::string &deviceId, RangingRole role), (override));
    MOCK_METHOD(int, StopRanging, (const std::string &deviceId), (override));
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

void RangingAdapterFactoryTest::SetUpTestCase(void)
{}

void RangingAdapterFactoryTest::TearDownTestCase(void)
{}

void RangingAdapterFactoryTest::SetUp()
{}

void RangingAdapterFactoryTest::TearDown()
{}

/**
 * @tc.name: InstanceShouldReturnSingleton
 * @tc.desc: 测试用例1：验证Instance返回单例
 * @tc.type: FUNC
 */
HWTEST_F(RangingAdapterFactoryTest, InstanceShouldReturnSingleton, TestSize.Level0)
{
    auto& instance1 = RangingAdapterFactory::Instance();
    auto& instance2 = RangingAdapterFactory::Instance();
    EXPECT_EQ(&instance1, &instance2);
}

/**
 * @tc.name: CreateRangingAdapterForUnregisteredTypeShouldReturnNull
 * @tc.desc: 测试用例2：验证未注册类型返回空指针
 * @tc.type: FUNC
 */
HWTEST_F(RangingAdapterFactoryTest, CreateRangingAdapterForUnregisteredTypeShouldReturnNull, TestSize.Level0)
{
    auto& factory = RangingAdapterFactory::Instance();
    auto adapter = factory.CreateRangingAdapter(static_cast<RangingTypes>(999));
    EXPECT_EQ(adapter, nullptr);
}

/**
 * @tc.name: IsRangingAdapterSupportedForUnregisteredTypeShouldReturnFalse
 * @tc.desc: 测试用例3：验证未注册类型不支持
 * @tc.type: FUNC
 */
HWTEST_F(RangingAdapterFactoryTest, IsRangingAdapterSupportedForUnregisteredTypeShouldReturnFalse, TestSize.Level0)
{
    auto& factory = RangingAdapterFactory::Instance();
    bool supported = factory.IsRangingAdapterSupported(static_cast<RangingTypes>(999));
    EXPECT_FALSE(supported);
}

/**
 * @tc.name: RegisterRangingAdapterShouldWork
 * @tc.desc: 测试用例4：验证注册适配器功能
 * @tc.type: FUNC
 */
HWTEST_F(RangingAdapterFactoryTest, RegisterRangingAdapterShouldWork, TestSize.Level0)
{
    auto& factory = RangingAdapterFactory::Instance();
    
    auto generator = []() -> std::shared_ptr<BaseRangingAdapter> {
        return std::make_shared<MockRangingAdapter>();
    };
    
    factory.RegisterRangingAdapter<MockRangingAdapter>(RangingTypes::NEARLINK_HADM, generator);
    factory.RegisterChecker(RangingTypes::NEARLINK_HADM, []() { return true; });
    
    bool supported = factory.IsRangingAdapterSupported(RangingTypes::NEARLINK_HADM);
    EXPECT_TRUE(supported);
}

/**
 * @tc.name: CreateRangingAdapterAfterRegisterShouldReturnAdapter
 * @tc.desc: 测试用例5：验证注册后创建适配器
 * @tc.type: FUNC
 */
HWTEST_F(RangingAdapterFactoryTest, CreateRangingAdapterAfterRegisterShouldReturnAdapter, TestSize.Level0)
{
    auto& factory = RangingAdapterFactory::Instance();
    
    auto generator = []() -> std::shared_ptr<BaseRangingAdapter> {
        return std::make_shared<MockRangingAdapter>();
    };
    
    factory.RegisterRangingAdapter<MockRangingAdapter>(RangingTypes::NEARLINK_HADM, generator);
    
    auto adapter = factory.CreateRangingAdapter(RangingTypes::NEARLINK_HADM);
    EXPECT_NE(adapter, nullptr);
}

/**
 * @tc.name: RegisterCheckerWithNullShouldReturnTrue
 * @tc.desc: 测试用例6：验证空checker注册返回true
 * @tc.type: FUNC
 */
HWTEST_F(RangingAdapterFactoryTest, RegisterCheckerWithNullShouldReturnTrue, TestSize.Level0)
{
    auto& factory = RangingAdapterFactory::Instance();
    
    RangingTypes testType = static_cast<RangingTypes>(100);
    factory.RegisterChecker(testType, nullptr);
    
    bool supported = factory.IsRangingAdapterSupported(testType);
    EXPECT_TRUE(supported);
}

/**
 * @tc.name: RegisterCheckerWithCustomCheckerShouldWork
 * @tc.desc: 测试用例7：验证自定义checker注册
 * @tc.type: FUNC
 */
HWTEST_F(RangingAdapterFactoryTest, RegisterCheckerWithCustomCheckerShouldWork, TestSize.Level0)
{
    auto& factory = RangingAdapterFactory::Instance();
    
    RangingTypes testType = static_cast<RangingTypes>(101);
    bool customResult = false;
    
    factory.RegisterChecker(testType, [&customResult]() { return customResult; });
    
    customResult = false;
    EXPECT_FALSE(factory.IsRangingAdapterSupported(testType));
    
    customResult = true;
    EXPECT_TRUE(factory.IsRangingAdapterSupported(testType));
}

/**
 * @tc.name: AutoRegisterRangingAdapterShouldAutoRegister
 * @tc.desc: 测试用例8：验证自动注册类
 * @tc.type: FUNC
 */
HWTEST_F(RangingAdapterFactoryTest, AutoRegisterRangingAdapterShouldAutoRegister, TestSize.Level0)
{
    auto& factory = RangingAdapterFactory::Instance();
    
    RangingTypes testType = static_cast<RangingTypes>(102);
    AutoRegisterRangingAdapter<MockRangingAdapter> autoRegister(testType);
    
    EXPECT_TRUE(factory.IsRangingAdapterSupported(testType));
    auto adapter = factory.CreateRangingAdapter(testType);
    EXPECT_NE(adapter, nullptr);
}

/**
 * @tc.name: ThreadSafetyTest
 * @tc.desc: 测试用例9：线程安全测试
 * @tc.type: FUNC
 */
HWTEST_F(RangingAdapterFactoryTest, ThreadSafetyTest, TestSize.Level0)
{
    auto& factory = RangingAdapterFactory::Instance();
    
    auto thread1 = std::thread([&] {
        for (int i = 0; i < 100; ++i) {
            factory.IsRangingAdapterSupported(RangingTypes::NEARLINK_HADM);
        }
    });

    auto thread2 = std::thread([&] {
        for (int i = 0; i < 100; ++i) {
            factory.CreateRangingAdapter(RangingTypes::NEARLINK_HADM);
        }
    });

    thread1.join();
    thread2.join();
}