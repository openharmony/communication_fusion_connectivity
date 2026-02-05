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
#define LOG_TAG "DeviceAgentCapBrTest"
#endif

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <thread>
#include "device_agent_capability_br.h"
#include "partner_device.h"
#include "log.h"

using namespace OHOS::FusionConnectivity;
using namespace testing;
using namespace testing::ext;

// Mock依赖函数接口
class MockDependencyFuncs {
public:
    MOCK_METHOD(void, startExtension, (), ());
    MOCK_METHOD(void, destroyExtension, (int), ());
};

class DeviceAgentCapabilityBrTest : public testing::Test {
public:
    DeviceAgentCapabilityBrTest() = default;
    ~DeviceAgentCapabilityBrTest() override = default;

    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    std::shared_ptr<MockDependencyFuncs> funcs_;
    std::shared_ptr<DeviceAgentCapabilityBr> deviceAgent_;
};

void DeviceAgentCapabilityBrTest::SetUpTestCase(void)
{}
void DeviceAgentCapabilityBrTest::TearDownTestCase(void)
{}
void DeviceAgentCapabilityBrTest::SetUp()
{
    funcs_ = std::make_shared<NiceMock<MockDependencyFuncs>>();

    auto startExtension = [this]() { funcs_->startExtension(); };
    auto destroyExtension = [this](int reason) { funcs_->destroyExtension(reason); };
    IDeviceAgentCapability::DependencyFuncs realFuncs = {
        .startExtension = startExtension,
        .destroyExtension = destroyExtension,
    };
    deviceAgent_ = std::make_shared<DeviceAgentCapabilityBr>(realFuncs, std::weak_ptr<PartnerDevice>());
}

void DeviceAgentCapabilityBrTest::TearDown()
{}

/**
 * @tc.name: InitShouldSetIsInitTrue
 * @tc.desc: 测试用例1：验证初始化状态
 * @tc.type: FUNC
 */
HWTEST_F(DeviceAgentCapabilityBrTest, InitShouldSetIsInitTrue, TestSize.Level0)
{
    EXPECT_CALL(*funcs_, startExtension()).Times(0);
    deviceAgent_->Init("00:11:22:33:44:55");
    EXPECT_TRUE(deviceAgent_->isInit_.load());
}

/**
 * @tc.name: CloseShouldSetIsInitFalse
 * @tc.desc: 测试用例2：验证关闭状态
 * @tc.type: FUNC
 */
HWTEST_F(DeviceAgentCapabilityBrTest, CloseShouldSetIsInitFalse, TestSize.Level0)
{
    deviceAgent_->Init("00:11:22:33:44:55");
    deviceAgent_->Close();
    EXPECT_FALSE(deviceAgent_->isInit_.load());
}

/**
 * @tc.name: AclConnectedWhenNotInitShouldDoNothing
 * @tc.desc: 测试用例3：未初始化时ACL连接回调
 * @tc.type: FUNC
 */
HWTEST_F(DeviceAgentCapabilityBrTest, AclConnectedWhenNotInitShouldDoNothing, TestSize.Level0)
{
    EXPECT_CALL(*funcs_, startExtension()).Times(0);
    deviceAgent_->OnBluetoothDeviceAclConnected();
}

/**
 * @tc.name: AclConnectedShouldResetTimer
 * @tc.desc: 测试用例4：ACL连接成功时重置定时器
 * @tc.type: FUNC
 */
HWTEST_F(DeviceAgentCapabilityBrTest, AclConnectedShouldResetTimer, TestSize.Level0)
{
    deviceAgent_->Init("00:11:22:33:44:55");
    deviceAgent_->aclDisconnectTimer_ = std::make_unique<Timer>([]() {});

    EXPECT_CALL(*funcs_, startExtension()).Times(1);
    deviceAgent_->OnBluetoothDeviceAclConnected();

    EXPECT_EQ(deviceAgent_->aclDisconnectTimer_, nullptr);
}

/**
 * @tc.name: AclDisconnectedShouldStartTimer
 * @tc.desc: 测试用例5：ACL断开启动定时器
 * @tc.type: FUNC
 */
HWTEST_F(DeviceAgentCapabilityBrTest, AclDisconnectedShouldStartTimer, TestSize.Level0)
{
    deviceAgent_->Init("00:11:22:33:44:55");
    deviceAgent_->extensionKeepAliveTimeout_ = 1;

    EXPECT_CALL(*funcs_, destroyExtension(ABILITY_DESTROY_DEVICE_LOST)).Times(1);
    deviceAgent_->OnBluetoothDeviceAclDisconnected();

    EXPECT_NE(deviceAgent_->aclDisconnectTimer_, nullptr);
    EXPECT_TRUE(deviceAgent_->aclDisconnectTimer_->IsStarted());
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

/**
 * @tc.name: TimerThreadSafetyTest
 * @tc.desc: 测试用例6：定时器线程安全测试
 * @tc.type: FUNC
 */
HWTEST_F(DeviceAgentCapabilityBrTest, TimerThreadSafetyTest, TestSize.Level0)
{
    deviceAgent_->Init("00:11:22:33:44:55");

    auto thread1 = std::thread([&] {
        for (int i = 0; i < 100; ++i) {
            deviceAgent_->OnBluetoothDeviceAclDisconnected();
        }
    });

    auto thread2 = std::thread([&] {
        for (int i = 0; i < 100; ++i) {
            deviceAgent_->OnBluetoothDeviceAclConnected();
        }
    });

    thread1.join();
    thread2.join();

    // 验证定时器状态
    EXPECT_TRUE(deviceAgent_->aclDisconnectTimer_ == nullptr ||
               deviceAgent_->aclDisconnectTimer_->IsStarted());
}

/**
 * @tc.name: ScreenOnOffEventsShouldDoNothing
 * @tc.desc: 测试用例7：屏幕开关事件空实现
 * @tc.type: FUNC
 */
HWTEST_F(DeviceAgentCapabilityBrTest, ScreenOnOffEventsShouldDoNothing, TestSize.Level0)
{
    EXPECT_CALL(*funcs_, startExtension()).Times(0);
    EXPECT_CALL(*funcs_, destroyExtension(_)).Times(0);
    deviceAgent_->OnScreenOn();
    deviceAgent_->OnScreenOff();
}

/**
 * @tc.name: ExtensionDestroyEventShouldDoNothing
 * @tc.desc: 测试用例8：扩展销毁事件空实现
 * @tc.type: FUNC
 */
HWTEST_F(DeviceAgentCapabilityBrTest, ExtensionDestroyEventShouldDoNothing, TestSize.Level0)
{
    EXPECT_CALL(*funcs_, startExtension()).Times(0);
    EXPECT_CALL(*funcs_, destroyExtension(_)).Times(0);
    deviceAgent_->OnExtensionDestroy();
}
