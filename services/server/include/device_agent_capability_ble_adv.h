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

#ifndef DEVICE_AGENT_CAPABILITY_BLE_ADV_H
#define DEVICE_AGENT_CAPABILITY_BLE_ADV_H

#include "i_device_agent_capability.h"
#include <atomic>
#include <mutex>
#include "timer_manager.h"
#include "bluetooth_ble_central_manager.h"

namespace OHOS {
namespace FusionConnectivity {
class DeviceAgentCapabilityBleAdv : public IDeviceAgentCapability,
                                    public std::enable_shared_from_this<DeviceAgentCapabilityBleAdv> {
public:
    DeviceAgentCapabilityBleAdv(DependencyFuncs funcs, std::weak_ptr<PartnerDevice> ownerWptr)
        : IDeviceAgentCapability(funcs, ownerWptr) {}
    ~DeviceAgentCapabilityBleAdv() override = default;

    void Init(const std::string &addr) override;
    void Close() override;
    void OnBluetoothDeviceAclConnected() override;
    void OnBluetoothDeviceAclDisconnected() override;
    void OnScreenOn() override;
    void OnScreenOff() override;
    void OnExtensionDestroy() override;

private:
    class BluetoothScanCallback : public Bluetooth::BleCentralManagerCallback {
    public:
        BluetoothScanCallback(std::weak_ptr<DeviceAgentCapabilityBleAdv> ownerWptr) : ownerWptr_(ownerWptr) {}
        ~BluetoothScanCallback() override = default;
        void OnScanCallback(const Bluetooth::BleScanResult &result) override;
        void OnFoundOrLostCallback(const Bluetooth::BleScanResult &result, uint8_t callbackType) override {}
        void OnBleBatchScanResultsEvent(const std::vector<Bluetooth::BleScanResult> &results) override {}
        void OnStartOrStopScanEvent(int resultCode, bool isStartScan) override {}

        std::weak_ptr<DeviceAgentCapabilityBleAdv> ownerWptr_;
    };

    void StartBleScan();
    void StopBleScan();

    std::atomic_bool isScanStarted_ = false;
    std::string address_ = "";
    std::atomic_int curScanMode_ = Bluetooth::SCAN_MODE_OP_P10_60_600;
    std::mutex bleScanMutex_;
    std::shared_ptr<BluetoothScanCallback> bluetoothScanCallback_ { nullptr };
    std::unique_ptr<Bluetooth::BleCentralManager> bleCentralManager_ { nullptr };

    std::mutex timerMutex_;
    int extensionKeepAliveTimeout_ = 3 * 60 * 1000;  // 3min
    std::unique_ptr<Timer> scanTimer_ { nullptr };  // BLE扫描到设备，但未触发ACL连接的超时定时器
};
}  // namespace FusionConnectivity
}  // namespace OHOS
#endif  // DEVICE_AGENT_CAPABILITY_BLE_ADV_H
