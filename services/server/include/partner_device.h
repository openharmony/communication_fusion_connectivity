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

#ifndef PARTNER_DEVICE_H
#define PARTNER_DEVICE_H

#include <memory>
#include <mutex>
#include "ifusion_connectivity_types.h"
#include "fcm_common_event_subscriber.h"
#include "partner_device_address.h"
#include "timer_manager.h"
#include "i_device_agent_capability.h"
#include "bluetooth_host.h"

namespace OHOS {
namespace FusionConnectivity {
class FcmCommonEventSubscriber;
enum PartnerAgentExtensionAbilityDestroyReason {
    ABILITY_DESTROY_UNKNOWN_REASON = 0,
    ABILITY_DESTROY_USER_CLOSED_ABILITY = 1,
    ABILITY_DESTROY_DEVICE_UNPAIRED = 2,
    ABILITY_DESTROY_DEVICE_LOST = 3,
    ABILITY_DESTROY_BLUETOOTH_DISABLED = 4,
};

class PartnerDevice : public std::enable_shared_from_this<PartnerDevice> {
public:
    static constexpr const char* COMMON_EVENT_BLUETOOTH_REMOTEDEVICE_ACL_CONNECTED_REALMAC =
        "usual.event.bluetooth.remotedevice.ACL_CONNECTED_REALMAC";
    static constexpr const char* COMMON_EVENT_BLUETOOTH_REMOTEDEVICE_ACL_DISCONNECTED_REALMAC =
        "usual.event.bluetooth.remotedevice.ACL_DISCONNECTED_REALMAC";
    static constexpr const char* COMMON_EVENT_BLUETOOTH_REMOTEDEVICE_PAIR_STATE_CHANGE_REALMAC =
        "usual.event.bluetooth.remotedevice.PAIR_STATE_CHANGE_REALMAC";

    struct DependencyFuncs {
        std::function<void(void)> updateConfig;
        std::function<void(std::string, std::string, PartnerDeviceAddress)> discoverExtension;
        std::function<void(std::string, std::string, int)> destroyExtension;
    };

    struct DeviceInfo {
        std::string bundleName = "";
        std::string abilityName = "";
        PartnerDeviceAddress deviceAddress;
        uint32_t tokenId = 0;
        int64_t registerTimestamp = 0;
        int64_t lostTimestamp = 0;
        bool isUserEnabled = true;
        DeviceCapability capability;
        BusinessCapability businessCapability;
    };

    static std::shared_ptr<PartnerDevice> CreateInstance(
        const DeviceInfo &deviceInfo, DependencyFuncs funcs);

    void Close();
    DeviceInfo GetDeviceInfo()
    {
        std::lock_guard<std::mutex> lock(deviceInfoMutex_);
        return deviceInfo_;
    }
    void SetUserEnableAbility(bool isEnabled);
    bool IsUserEnableAbility()
    {
        std::lock_guard<std::mutex> lock(deviceInfoMutex_);
        return deviceInfo_.isUserEnabled;
    }

private:
    class BluetoothStateObserver : public Bluetooth::BluetoothHostObserver {
    public:
        BluetoothStateObserver(std::weak_ptr<PartnerDevice> ownerWptr) : ownerWptr_(ownerWptr) {}
        ~BluetoothStateObserver() override = default;

        void OnStateChanged(const int transport, const int status) override;
        void OnDiscoveryStateChanged(int status) override {}
        void OnDiscoveryResult(const Bluetooth::BluetoothRemoteDevice &device,
            int rssi, const std::string deviceName, int deviceClass) override {}
        void OnPairRequested(const Bluetooth::BluetoothRemoteDevice &device) override {}
        void OnPairConfirmed(const Bluetooth::BluetoothRemoteDevice &device, int reqType, int number) override {}
        void OnScanModeChanged(int mode) override {}
        void OnDeviceNameChanged(const std::string &deviceName) override {}
        void OnDeviceAddrChanged(const std::string &address) override {}

    private:
        std::weak_ptr<PartnerDevice> ownerWptr_;
    };

    PartnerDevice(const DeviceInfo &deviceInfo, DependencyFuncs funcs)
        : deviceInfo_(deviceInfo), dependencyFuncs_(funcs) {}

    void Init();
    void UpdatePartnerDeviceIsAllowStarted(const DeviceInfo &info);
    void InitDeviceAgentCapability(const std::string &addr, bool isSupportBleAdvertiser);
    void CloseDeviceAgentCapability(int destroyReason);
    void OnCommonEventReceived(const OHOS::EventFwk::CommonEventData &data);
    void OnBluetoothDeviceAclStateChange(const OHOS::EventFwk::CommonEventData &data, bool isConnect);
    void OnBluetoothDevicePairStateChange(const OHOS::EventFwk::CommonEventData &data);
    void OnScreenStateChange(bool isScreenOn);
    void SubscribeCommonEvent(const std::vector<std::string> &eventVec,
    const std::vector<std::string> &permissionVec, std::shared_ptr<FcmCommonEventSubscriber> &eventSubscribe);
    bool IsPairedDevice(const std::string &address);

    void UpdateLostTimestamp(int64_t lostTimestamp)
    {
        std::lock_guard<std::mutex> lock(deviceInfoMutex_);
        deviceInfo_.lostTimestamp = lostTimestamp;
    }

    std::mutex deviceInfoMutex_;
    DeviceInfo deviceInfo_;

    std::shared_ptr<FcmCommonEventSubscriber> bluetoothEventSubscribe_ { nullptr };
    std::shared_ptr<FcmCommonEventSubscriber> screenEventSubscribe_ { nullptr };

    DependencyFuncs dependencyFuncs_;

    std::atomic_bool isAllowed_ { true };

    const std::string CAPABILITY_BLE_ADV_KEY = "CapabilityBleAdvKey";
    const std::string CAPABILITY_BR_KEY = "CapabilityBrKey";
    // 该变量仅初始化创建一次，后续都是读操作
    std::map<std::string, std::shared_ptr<IDeviceAgentCapability>> deviceAgentCapabilityMap_;

    std::shared_ptr<BluetoothStateObserver> bluetoothStateObserver_ { nullptr };

    // The passkey pattern of C++
    struct PassKey {
        PassKey() {};
    };
public:
    explicit PartnerDevice(PassKey, const DeviceInfo &deviceInfo, DependencyFuncs funcs)
        : PartnerDevice(deviceInfo, funcs) {};
    ~PartnerDevice();
};

}  // namespace FusionConnectivity
}  // namespace OHOS
#endif  // PARTNER_DEVICE_H
