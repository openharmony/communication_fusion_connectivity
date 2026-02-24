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
#define LOG_TAG "PartnerDevice"
#endif

#include "partner_device.h"
#include "log.h"
#include "log_util.h"
#include "datetime_ex.h"
#include "common_utils.h"
#include "ffrt_inner.h"
#include "device_agent_capability_ble_adv.h"
#include "device_agent_capability_br.h"

namespace OHOS {
namespace FusionConnectivity {
namespace {
enum BondState : int {
    BOND_STATE_NONE = 0,
    BOND_STATE_BONDING = 1,
    BOND_STATE_BONDED = 2
};
}  // namespace
using namespace OHOS::Bluetooth;

std::shared_ptr<PartnerDevice> PartnerDevice::CreateInstance(const DeviceInfo &deviceInfo, DependencyFuncs funcs)
{
    // The passkey pattern used here.
    auto deviceSptr = std::make_shared<PartnerDevice>(PassKey(), deviceInfo, funcs);
    deviceSptr->Init();
    return deviceSptr;
}

PartnerDevice::~PartnerDevice()
{}

void PartnerDevice::UpdatePartnerDeviceIsAllowStarted(const DeviceInfo &info)
{
    bool isPaired = IsPairedDevice(info.deviceAddress.GetAddress());
    if (!isPaired || !info.isUserEnabled) {
        HILOGI("The partner device %{public}s is closed due to (isPaired: %{public}d, isUserEnabled: %{public}d)",
            GET_ENCRYPT_ADDR(info.deviceAddress), isPaired, info.isUserEnabled);
        isAllowed_ = false;
    } else {
        HILOGI("The partner device %{public}s is started", GET_ENCRYPT_ADDR(info.deviceAddress));
        isAllowed_ = true;
    }
}

void PartnerDevice::SetUserEnableAbility(bool isEnabled)
{
    {
        std::lock_guard<std::mutex> lock(deviceInfoMutex_);
        deviceInfo_.isUserEnabled = isEnabled;
    }

    DeviceInfo info = GetDeviceInfo();
    if (isEnabled) {
        UpdatePartnerDeviceIsAllowStarted(info);
        InitDeviceAgentCapability(info.deviceAddress.GetAddress(), info.capability.isSupportBleAdvertiser);
    } else {
        UpdatePartnerDeviceIsAllowStarted(info);
        CloseDeviceAgentCapability(ABILITY_DESTROY_USER_CLOSED_ABILITY);
    }
}

void PartnerDevice::SubscribeCommonEvent(const std::vector<std::string> &eventVec,
    const std::vector<std::string> &permissionVec, std::shared_ptr<FcmCommonEventSubscriber> &eventSubscribe)
{
    EventFwk::MatchingSkills matchingSkills;
    for (const std::string &event : eventVec) {
        matchingSkills.AddEvent(event);
    }
    EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);
    for (const std::string &permission : permissionVec) {
        subscribeInfo.SetPermission(permission);
    }
    auto func = [ptr = weak_from_this()](const OHOS::EventFwk::CommonEventData &data) {
        ffrt::submit([ptr, data]() {
            auto partnerDeviceSptr = ptr.lock();
            if (partnerDeviceSptr) {
                partnerDeviceSptr->OnCommonEventReceived(data);
            }
        });
    };
    eventSubscribe =
        std::make_shared<FcmCommonEventSubscriber>(subscribeInfo, eventVec, func);
    if (!EventFwk::CommonEventManager::SubscribeCommonEvent(eventSubscribe)) {
        HILOGE("Subscribe common event failed");
        // no need return
    }
}

void PartnerDevice::BluetoothStateObserver::OnStateChanged(const int transport, const int status)
{
    auto ownerSptr = ownerWptr_.lock();
    if (!ownerSptr) {
        HILOGE("ownerSptr is nullptr");
        return;
    }

    if (transport == BTTransport::ADAPTER_BREDR && status == BTStateID::STATE_TURN_ON) {
        DeviceInfo info = ownerSptr->GetDeviceInfo();
        ownerSptr->UpdatePartnerDeviceIsAllowStarted(info);
        ownerSptr->InitDeviceAgentCapability(info.deviceAddress.GetAddress(), info.capability.isSupportBleAdvertiser);
    }
    if (transport == BTTransport::ADAPTER_BLE && status == BTStateID::STATE_TURN_OFF) {
        ownerSptr->CloseDeviceAgentCapability(ABILITY_DESTROY_BLUETOOTH_DISABLED);
    }
}

void PartnerDevice::Init()
{
    auto startExtension = [this]() {
        HILOGI("start extension");
        DeviceInfo info = GetDeviceInfo();
        dependencyFuncs_.discoverExtension(info.bundleName, info.abilityName, info.deviceAddress);
    };
    auto destroyExtension = [this](int destroyReason) {
        HILOGI("destroy extension");
        DeviceInfo info = GetDeviceInfo();
        dependencyFuncs_.destroyExtension(info.bundleName, info.abilityName, destroyReason);
    };
    IDeviceAgentCapability::DependencyFuncs funcs = {
        .startExtension = startExtension,
        .destroyExtension = destroyExtension,
    };
    deviceAgentCapabilityMap_[CAPABILITY_BLE_ADV_KEY] =
        std::make_shared<DeviceAgentCapabilityBleAdv>(funcs, weak_from_this());
    deviceAgentCapabilityMap_[CAPABILITY_BR_KEY] =
        std::make_shared<DeviceAgentCapabilityBr>(funcs, weak_from_this());

    DeviceInfo info = GetDeviceInfo();
    UpdatePartnerDeviceIsAllowStarted(info);
    InitDeviceAgentCapability(info.deviceAddress.GetAddress(), info.capability.isSupportBleAdvertiser);

    // 监听蓝牙开关状态
    bluetoothStateObserver_ = std::make_shared<BluetoothStateObserver>(weak_from_this());
    BluetoothHost::GetDefaultHost().RegisterObserver(bluetoothStateObserver_);
    // 监听亮灭屏公共事件
    std::vector<std::string> screenCommonEventVec = {
        EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_ON,
        EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_OFF,
    };
    SubscribeCommonEvent(screenCommonEventVec, {}, screenEventSubscribe_);
    // 监听蓝牙公共事件
    std::vector<std::string> commonEventVec = {
        COMMON_EVENT_BLUETOOTH_REMOTEDEVICE_ACL_CONNECTED_REALMAC,
        COMMON_EVENT_BLUETOOTH_REMOTEDEVICE_ACL_DISCONNECTED_REALMAC,
        COMMON_EVENT_BLUETOOTH_REMOTEDEVICE_PAIR_STATE_CHANGE_REALMAC,
    };
    std::vector<std::string> permissionVec = {
        "ohos.permission.ACCESS_BLUETOOTH",
        "ohos.permission.MANAGE_BLUETOOTH",
    };
    SubscribeCommonEvent(commonEventVec, permissionVec, bluetoothEventSubscribe_);
}

void PartnerDevice::InitDeviceAgentCapability(const std::string &addr, bool isSupportBleAdvertiser)
{
    if (isAllowed_.load()) {
        deviceAgentCapabilityMap_[CAPABILITY_BR_KEY]->Init(addr);
        if (isSupportBleAdvertiser) {
            deviceAgentCapabilityMap_[CAPABILITY_BLE_ADV_KEY]->Init(addr);
        }
    }
}

void PartnerDevice::CloseDeviceAgentCapability(int destroyReason)
{
    for (auto &[_, deviceAgentAbility] : deviceAgentCapabilityMap_) {
        deviceAgentAbility->Close();
    }
    DeviceInfo info = GetDeviceInfo();
    dependencyFuncs_.destroyExtension(info.bundleName, info.abilityName, destroyReason);
}

void PartnerDevice::Close()
{
    // 取消公共事件监听
    if (bluetoothEventSubscribe_) {
        EventFwk::CommonEventManager::UnSubscribeCommonEvent(bluetoothEventSubscribe_);
        bluetoothEventSubscribe_ = nullptr;
    }
    if (screenEventSubscribe_) {
        EventFwk::CommonEventManager::UnSubscribeCommonEvent(screenEventSubscribe_);
        screenEventSubscribe_ = nullptr;
    }
}

bool PartnerDevice::IsPairedDevice(const std::string &address)
{
    int pairState = PAIR_NONE;
    auto device = BluetoothHost::GetDefaultHost().GetRemoteDevice(address, Bluetooth::BTTransport::ADAPTER_BREDR);
    device.GetPairState(pairState);
    return pairState == PAIR_PAIRED;
}

void PartnerDevice::OnBluetoothDeviceAclStateChange(const OHOS::EventFwk::CommonEventData &data, bool isConnect)
{
    AAFwk::Want want = data.GetWant();
    std::string addr = want.GetStringParam("deviceAddr");
    if (!IsValidAddress(addr)) {
        HILOGE("Invalid address");
        return;
    }
    HILOGI("%{public}s acl state change, isConnect: %{public}d", GetEncryptAddr(addr).c_str(), isConnect);
    DeviceInfo deviceInfo = GetDeviceInfo();
    if (deviceInfo.deviceAddress.GetAddress() != addr) {
        return;
    }

    for (auto &[_, deviceAgentAbility] : deviceAgentCapabilityMap_) {
        if (isConnect) {
            deviceAgentAbility->OnBluetoothDeviceAclConnected();
        } else {
            deviceAgentAbility->OnBluetoothDeviceAclDisconnected();
        }
    }
}

void PartnerDevice::OnBluetoothDevicePairStateChange(const OHOS::EventFwk::CommonEventData &data)
{
    AAFwk::Want want = data.GetWant();
    std::string addr = want.GetStringParam("deviceAddr");
    if (!IsValidAddress(addr)) {
        HILOGE("Invalid address");
        return;
    }

    DeviceInfo deviceInfo = GetDeviceInfo();
    if (deviceInfo.deviceAddress.GetAddress() != addr) {
        return;
    }
    int state = want.GetIntParam("state", BOND_STATE_NONE);
    HILOGI("pair state change, %{public}s, state: %{public}d (0: BOND_NONE, 2: BONDED), isUserEnabled: %{public}d",
        GetEncryptAddr(addr).c_str(), state, deviceInfo.isUserEnabled);

    if (state == BOND_STATE_NONE) {
        CloseDeviceAgentCapability(ABILITY_DESTROY_DEVICE_UNPAIRED);
        isAllowed_ = false;

        UpdateLostTimestamp(GetDaysSince1970ToNow());
        if (dependencyFuncs_.updateConfig) {
            dependencyFuncs_.updateConfig();
        }
    } else if (state == BOND_STATE_BONDED) {
        if (deviceInfo.isUserEnabled) {
            isAllowed_ = true;
            InitDeviceAgentCapability(
                deviceInfo.deviceAddress.GetAddress(), deviceInfo.capability.isSupportBleAdvertiser);
        }

        UpdateLostTimestamp(0);
        if (dependencyFuncs_.updateConfig) {
            dependencyFuncs_.updateConfig();
        }
    }
}

void PartnerDevice::OnScreenStateChange(bool isScreenOn)
{
    for (auto &[_, deviceAgentAbility] : deviceAgentCapabilityMap_) {
        if (isScreenOn) {
            deviceAgentAbility->OnScreenOn();
        } else {
            deviceAgentAbility->OnScreenOff();
        }
    }
}

void PartnerDevice::OnCommonEventReceived(const OHOS::EventFwk::CommonEventData &data)
{
    auto want = data.GetWant();
    std::string action = want.GetAction();
    if (action == COMMON_EVENT_BLUETOOTH_REMOTEDEVICE_ACL_CONNECTED_REALMAC) {
        OnBluetoothDeviceAclStateChange(data, true);
    }
    if (action == COMMON_EVENT_BLUETOOTH_REMOTEDEVICE_ACL_DISCONNECTED_REALMAC) {
        OnBluetoothDeviceAclStateChange(data, false);
    }
    if (action == COMMON_EVENT_BLUETOOTH_REMOTEDEVICE_PAIR_STATE_CHANGE_REALMAC) {
        OnBluetoothDevicePairStateChange(data);
    }
    if (action == EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_ON) {
        OnScreenStateChange(true);
    }
    if (action == EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_OFF) {
        OnScreenStateChange(false);
    }
}

}  // namespace FusionConnectivity
}  // namespace OHOS
