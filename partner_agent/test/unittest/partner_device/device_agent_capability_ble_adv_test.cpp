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
#define LOG_TAG "DeviceAgentCapBleAdvTest"
#endif

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <thread>
#include "device_agent_capability_ble_adv.h"
#include "partner_device.h"
#include "log.h"
#include "bluetooth_ble_central_manager.h"

using namespace OHOS;
using namespace OHOS::FusionConnectivity;
using namespace testing;
using namespace testing::ext;

// Mock依赖函数接口
class MockDependencyFuncs {
public:
    MOCK_METHOD(void, startExtension, (), ());
    MOCK_METHOD(void, destroyExtension, (int), ());
};

class DeviceAgentCapabilityBleAdvTest : public testing::Test {
public:
    DeviceAgentCapabilityBleAdvTest() = default;
    ~DeviceAgentCapabilityBleAdvTest() override = default;

    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    std::shared_ptr<MockDependencyFuncs> funcs_;
    std::shared_ptr<DeviceAgentCapabilityBleAdv> deviceAgent_;
};

void DeviceAgentCapabilityBleAdvTest::SetUpTestCase(void)
{}
void DeviceAgentCapabilityBleAdvTest::TearDownTestCase(void)
{}
void DeviceAgentCapabilityBleAdvTest::SetUp()
{
    funcs_ = std::make_shared<NiceMock<MockDependencyFuncs>>();

    auto startExtension = [this]() { funcs_->startExtension(); };
    auto destroyExtension = [this](int reason) { funcs_->destroyExtension(reason); };
    IDeviceAgentCapability::DependencyFuncs realFuncs = {
        .startExtension = startExtension,
        .destroyExtension = destroyExtension,
    };
    deviceAgent_ = std::make_shared<DeviceAgentCapabilityBleAdv>(realFuncs, std::weak_ptr<PartnerDevice>());
}

void DeviceAgentCapabilityBleAdvTest::TearDown()
{
    deviceAgent_->Close();
    deviceAgent_ = nullptr;
}

// 测试用例1：初始化测试
/**
 * @tc.name: InitShouldStartScan
 * @tc.desc: 验证初始化时正确启动蓝牙扫描
 * @tc.type: FUNC
 */
HWTEST_F(DeviceAgentCapabilityBleAdvTest, InitShouldStartScan, TestSize.Level0)
{
    EXPECT_CALL(*funcs_, startExtension()).Times(0);
    deviceAgent_->Init("00:11:22:33:44:55");
    EXPECT_EQ(deviceAgent_->address_, "00:11:22:33:44:55");
    EXPECT_NE(deviceAgent_->bleCentralManager_, nullptr);
    EXPECT_TRUE(deviceAgent_->isScanStarted_.load());
}

// 测试用例2：关闭测试
/**
 * @tc.name: CloseShouldStopScan
 * @tc.desc: 验证关闭时正确停止蓝牙扫描
 * @tc.type: FUNC
 */
HWTEST_F(DeviceAgentCapabilityBleAdvTest, CloseShouldStopScan, TestSize.Level0) {
    deviceAgent_->Init("00:11:22:33:44:55");
    deviceAgent_->Close();
    EXPECT_EQ(deviceAgent_->bleCentralManager_, nullptr);
    EXPECT_FALSE(deviceAgent_->isScanStarted_.load());
}

// 测试用例3：扫描回调测试
/**
 * @tc.name: ScanCallbackTest
 * @tc.desc: 验证蓝牙扫描回调处理逻辑
 * @tc.type: FUNC
 */
HWTEST_F(DeviceAgentCapabilityBleAdvTest, ScanCallbackTest, TestSize.Level0) {
    deviceAgent_->Init("00:11:22:33:44:55");
    deviceAgent_->extensionKeepAliveTimeout_ = 1;
    Bluetooth::BleScanResult scanResult;

    EXPECT_CALL(*funcs_, startExtension()).Times(1);
    EXPECT_CALL(*funcs_, destroyExtension(ABILITY_DESTROY_DEVICE_LOST)).Times(1);

    deviceAgent_->bluetoothScanCallback_->OnScanCallback(scanResult);

    EXPECT_NE(deviceAgent_->scanTimer_, nullptr);
    EXPECT_TRUE(deviceAgent_->scanTimer_->IsStarted());
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

// 测试用例4：ACL连接测试
/**
 * @tc.name: AclConnectedTest
 * @tc.desc: 验证ACL连接时停止扫描
 * @tc.type: FUNC
 */
HWTEST_F(DeviceAgentCapabilityBleAdvTest, AclConnectedTest, TestSize.Level0) {
    deviceAgent_->Init("00:11:22:33:44:55");
    deviceAgent_->OnBluetoothDeviceAclConnected();
    EXPECT_FALSE(deviceAgent_->isScanStarted_.load());
    EXPECT_EQ(deviceAgent_->scanTimer_, nullptr);
}

// 测试用例5：屏幕状态测试
/**
 * @tc.name: ScreenStateChangeTest
 * @tc.desc: 验证屏幕状态变化时调整扫描模式
 * @tc.type: FUNC
 */
HWTEST_F(DeviceAgentCapabilityBleAdvTest, ScreenStateChangeTest, TestSize.Level0) {
    deviceAgent_->Init("00:11:22:33:44:55");

    deviceAgent_->OnScreenOn();
    EXPECT_EQ(deviceAgent_->curScanMode_.load(), Bluetooth::SCAN_MODE_OP_P10_60_600);

    deviceAgent_->OnScreenOff();
    EXPECT_EQ(deviceAgent_->curScanMode_.load(), Bluetooth::SCAN_MODE_OP_P2_60_3000);
}

// 测试用例6：线程安全测试
/**
 * @tc.name: ThreadSafetyTest
 * @tc.desc: 验证多线程环境下的扫描操作安全性
 * @tc.type: FUNC
 */
HWTEST_F(DeviceAgentCapabilityBleAdvTest, ThreadSafetyTest, TestSize.Level1) {
    deviceAgent_->Init("00:11:22:33:44:55");

    auto thread1 = std::thread([&] {
        for (int i = 0; i < 100; ++i) {
            deviceAgent_->OnScreenOn();
            deviceAgent_->OnScreenOff();
        }
    });

    auto thread2 = std::thread([&] {
        for (int i = 0; i < 100; ++i) {
            deviceAgent_->OnBluetoothDeviceAclConnected();
        }
    });

    thread1.join();
    thread2.join();

    EXPECT_FALSE(deviceAgent_->isScanStarted_.load());
}

// 测试用例7：scanTimer超时流程
/**
 * @tc.name: ScanTimeoutShouldStartPowerInhibitTimer
 * @tc.desc: 验证scanTimer超时后正确启动功耗抑制定时器
 * @tc.type: FUNC
 */
HWTEST_F(DeviceAgentCapabilityBleAdvTest, ScanTimeoutShouldStartPowerInhibitTimer, TestSize.Level1) {
    HILOGI("ScanTimeoutShouldStartPowerInhibitTimer enter");
    deviceAgent_->Init("00:11:22:33:44:55");
    deviceAgent_->extensionKeepAliveTimeout_ = 1;
    Bluetooth::BleScanResult scanResult;

    EXPECT_CALL(*funcs_, startExtension()).Times(1);
    EXPECT_CALL(*funcs_, destroyExtension(_)).Times(1);

    deviceAgent_->bluetoothScanCallback_->OnScanCallback(scanResult);

    EXPECT_NE(deviceAgent_->scanTimer_, nullptr);
    EXPECT_TRUE(deviceAgent_->scanTimer_->IsStarted());
    EXPECT_EQ(deviceAgent_->timeoutCount_.load(), 0);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    EXPECT_EQ(deviceAgent_->timeoutCount_.load(), 1);
    EXPECT_NE(deviceAgent_->powerInhibitTimer_, nullptr);
    EXPECT_TRUE(deviceAgent_->powerInhibitTimer_->IsStarted());
    HILOGI("ScanTimeoutShouldStartPowerInhibitTimer end");
}

// 测试用例8：连续5次超时解注册
/**
 * @tc.name: FiveConsecutiveTimeoutsShouldStopScan
 * @tc.desc: 验证连续5次超时后停止扫描
 * @tc.type: FUNC
 */
HWTEST_F(DeviceAgentCapabilityBleAdvTest, FiveConsecutiveTimeoutsShouldStopScan, TestSize.Level1) {
    deviceAgent_->Init("00:11:22:33:44:55");
    deviceAgent_->extensionKeepAliveTimeout_ = 1;
    Bluetooth::BleScanResult scanResult;

    EXPECT_CALL(*funcs_, startExtension()).Times(5);
    EXPECT_CALL(*funcs_, destroyExtension(_)).Times(5);

    for (int i = 0; i < 5; i++) {
        deviceAgent_->bluetoothScanCallback_->OnScanCallback(scanResult);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_EQ(deviceAgent_->timeoutCount_.load(), 5);
    EXPECT_EQ(deviceAgent_->powerInhibitTimer_, nullptr);
}

// 测试用例9：ACL连接重置计数器
/**
 * @tc.name: AclConnectedShouldResetCounter
 * @tc.desc: 验证ACL连接后重置计数器并取消功耗抑制定时器
 * @tc.type: FUNC
 */
HWTEST_F(DeviceAgentCapabilityBleAdvTest, AclConnectedShouldResetCounter, TestSize.Level1) {
    deviceAgent_->Init("00:11:22:33:44:55");
    deviceAgent_->extensionKeepAliveTimeout_ = 1;
    Bluetooth::BleScanResult scanResult;

    deviceAgent_->bluetoothScanCallback_->OnScanCallback(scanResult);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    EXPECT_EQ(deviceAgent_->timeoutCount_.load(), 1);
    EXPECT_NE(deviceAgent_->powerInhibitTimer_, nullptr);

    deviceAgent_->OnBluetoothDeviceAclConnected();

    EXPECT_EQ(deviceAgent_->timeoutCount_.load(), 0);
    EXPECT_EQ(deviceAgent_->powerInhibitTimer_, nullptr);
}

// 测试用例10：功耗抑制定时器超时重启扫描
/**
 * @tc.name: PowerInhibitTimerShouldRestartScan
 * @tc.desc: 验证功耗抑制定时器超时后重启扫描
 * @tc.type: FUNC
 */
HWTEST_F(DeviceAgentCapabilityBleAdvTest, PowerInhibitTimerShouldRestartScan, TestSize.Level1) {
    deviceAgent_->Init("00:11:22:33:44:55");
    deviceAgent_->extensionKeepAliveTimeout_ = 1;
    deviceAgent_->powerInhibitTimeout_ = 10;
    Bluetooth::BleScanResult scanResult;

    deviceAgent_->bluetoothScanCallback_->OnScanCallback(scanResult);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    EXPECT_EQ(deviceAgent_->timeoutCount_.load(), 1);
    EXPECT_NE(deviceAgent_->powerInhibitTimer_, nullptr);

    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    EXPECT_TRUE(deviceAgent_->isScanStarted_.load());
}

// 测试用例11：并发定时器操作线程安全
/**
 * @tc.name: ConcurrentTimerOperationsShouldBeThreadSafe
 * @tc.desc: 验证并发定时器操作的线程安全性
 * @tc.type: FUNC
 */
HWTEST_F(DeviceAgentCapabilityBleAdvTest, ConcurrentTimerOperationsShouldBeThreadSafe, TestSize.Level2) {
    deviceAgent_->Init("00:11:22:33:44:55");
    deviceAgent_->extensionKeepAliveTimeout_ = 10;
    Bluetooth::BleScanResult scanResult;

    auto thread1 = std::thread([&] {
        for (int i = 0; i < 100; ++i) {
            deviceAgent_->bluetoothScanCallback_->OnScanCallback(scanResult);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    auto thread2 = std::thread([&] {
        for (int i = 0; i < 100; ++i) {
            deviceAgent_->OnBluetoothDeviceAclConnected();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    thread1.join();
    thread2.join();

    EXPECT_GE(deviceAgent_->timeoutCount_.load(), 0);
    EXPECT_LE(deviceAgent_->timeoutCount_.load(), 5);
}

// 测试用例12：设备关闭时清理定时器
/**
 * @tc.name: CloseShouldCleanUpAllTimers
 * @tc.desc: 验证Close方法正确清理所有定时器
 * @tc.type: FUNC
 */
HWTEST_F(DeviceAgentCapabilityBleAdvTest, CloseShouldCleanUpAllTimers, TestSize.Level0) {
    deviceAgent_->Init("00:11:22:33:44:55");
    deviceAgent_->extensionKeepAliveTimeout_ = 1000;
    Bluetooth::BleScanResult scanResult;

    deviceAgent_->bluetoothScanCallback_->OnScanCallback(scanResult);

    EXPECT_NE(deviceAgent_->scanTimer_, nullptr);

    deviceAgent_->Close();

    EXPECT_EQ(deviceAgent_->scanTimer_, nullptr);
    EXPECT_EQ(deviceAgent_->powerInhibitTimer_, nullptr);
    EXPECT_EQ(deviceAgent_->timeoutCount_.load(), 0);
    EXPECT_FALSE(deviceAgent_->isScanStarted_.load());
}
