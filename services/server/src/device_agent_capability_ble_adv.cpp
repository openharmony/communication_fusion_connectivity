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
#define LOG_TAG "DeviceAgentCapBleAdv"
#endif

#include "device_agent_capability_ble_adv.h"
#include "log_util.h"
#include "partner_device.h"

namespace OHOS {
namespace FusionConnectivity {
using namespace OHOS::Bluetooth;

void DeviceAgentCapabilityBleAdv::BluetoothScanCallback::OnScanCallback(const BleScanResult &result)
{
    auto ownerSptr = ownerWptr_.lock();
    if (!ownerSptr) {
        HILOGW("partnerDeviceSptr is nullptr");
        return;
    }
    HILOGI("find device: %{public}s", GetEncryptAddr(result.GetPeripheralDevice().GetDeviceAddr()).c_str());

    ownerSptr->funcs_.startExtension();
    ownerSptr->StopBleScan();

    // 起3分钟定时器，若无ACL连接，断开extension
    auto func = [ptr = ownerWptr_]() {
        auto partnerDeviceSptr = ptr.lock();
        if (partnerDeviceSptr) {
            partnerDeviceSptr->funcs_.destroyExtension(ABILITY_DESTROY_DEVICE_LOST);
            // 起10分钟定时器，功耗抑制方案
        };
    };
    std::lock_guard<std::mutex> lock(ownerSptr->timerMutex_);
    ownerSptr->scanTimer_ = std::make_unique<Timer>(func);
    ownerSptr->scanTimer_->Start(ownerSptr->extensionKeepAliveTimeout_);
}

void DeviceAgentCapabilityBleAdv::Init(const std::string &addr)
{
    HILOGI("Init ble capability");
    {
        std::lock_guard<std::mutex> lock(bleScanMutex_);
        address_ = addr;
        bluetoothScanCallback_ = std::make_shared<BluetoothScanCallback>(weak_from_this());
        bleCentralManager_ = std::make_unique<BleCentralManager>(bluetoothScanCallback_);
    }
    StartBleScan();
}

void DeviceAgentCapabilityBleAdv::Close()
{
    HILOGI("Close ble capability");
    std::lock_guard<std::mutex> lock(bleScanMutex_);
    if (bleCentralManager_) {
        bleCentralManager_->StopScan();
        isScanStarted_ = false;
    }
    bluetoothScanCallback_ = nullptr;
    bleCentralManager_ = nullptr;
}

void DeviceAgentCapabilityBleAdv::StartBleScan()
{
    std::lock_guard<std::mutex> lock(bleScanMutex_);
    if (!bleCentralManager_) {
        return;
    }
    bleCentralManager_->StopScan();

    BleScanSettings settings;
    settings.SetScanMode(curScanMode_.load());
    BleScanFilter filter;
    filter.SetDeviceId(address_);
    bleCentralManager_->StartScan(settings, std::vector<BleScanFilter>{filter});
    isScanStarted_ = true;
}

void DeviceAgentCapabilityBleAdv::StopBleScan()
{
    std::lock_guard<std::mutex> lock(bleScanMutex_);
    if (bleCentralManager_) {
        bleCentralManager_->StopScan();
    }
    isScanStarted_ = false;
}

void DeviceAgentCapabilityBleAdv::OnBluetoothDeviceAclConnected()
{
    StopBleScan();

    std::lock_guard<std::mutex> lock(timerMutex_);
    scanTimer_ = nullptr;
}

void DeviceAgentCapabilityBleAdv::OnBluetoothDeviceAclDisconnected()
{}

void DeviceAgentCapabilityBleAdv::OnScreenOn()
{
    curScanMode_ = SCAN_MODE_OP_P10_60_600;
    if (isScanStarted_.load()) {
        // 更改参数，重启BLE扫描
        StartBleScan();
    }
}

void DeviceAgentCapabilityBleAdv::OnScreenOff()
{
    curScanMode_ = SCAN_MODE_OP_P2_60_3000;
    if (isScanStarted_.load()) {
        // 更改参数，重启BLE扫描
        StartBleScan();
    }
}

void DeviceAgentCapabilityBleAdv::OnExtensionDestroy()
{
    StartBleScan();
}
}  // namespace FusionConnectivity
}  // namespace OHOS
