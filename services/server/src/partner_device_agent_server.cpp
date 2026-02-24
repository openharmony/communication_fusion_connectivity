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
#define LOG_TAG "PartnerDeviceAgentServer"
#endif

#include "partner_device_agent_server.h"

#include <set>
#include "log.h"
#include "log_util.h"
#include "ipc_skeleton.h"
#include "fcm_thread_util.h"
#include "fusion_connectivity_errorcode.h"
#include "fusion_conn_load_utils.h"
#include "bluetooth_remote_device.h"
#include "bluetooth_def.h"
#include "bluetooth_host.h"
#include "parameter.h"
#include "parameters.h"
#include "common_utils.h"
#include "datetime_ex.h"
#include "ffrt_inner.h"
#include "bluetooth_host.h"
#include "partner_device_config.h"

namespace OHOS {
namespace FusionConnectivity {
using namespace OHOS::Bluetooth;

using CONNECT = int32_t (*)(const std::string, const std::string, const int32_t);
using ONDEVICEDISCOVERED = void (*)(const std::string&,
    const std::string&, const int32_t, const PartnerDeviceAddress&, const NotificationType&);
using ONDESTROYWITHREASON = void (*)(const std::string&, const std::string&, const int32_t, const int32_t);

namespace {
    const int32_t PARTNER_DEVICE_AGENT_SYS_ABILITY_ID = 8630;
    const size_t MAX_OBSERVERS_SIZE = 1000;
    constexpr const char* PARTNER_AGENT_EXTENSION_SERVICE_MODULE_NAME = "libpartner_agent_extension_service.z.so";
    constexpr const int64_t SHUTDOWN_DELAY_TIME = 1;    // 1s
    static constexpr const char *SYS_PARAM_ENABLE_PARTNER_AGENT =
        "persist.fusion_connectivity.enable_partner_agent";
    const char *SYS_PARAM_ENABLE_PARTNER_AGENT_DISABLED = "0";
    const char *SYS_PARAM_ENABLE_PARTNER_AGENT_ENABLED = "1";
}

const bool REGISTER_RESULT =
    SystemAbility::MakeAndRegisterAbility(PartnerDeviceAgentServer::GetInstance().GetRefPtr());

struct PartnerDeviceAgentServer::impl {
};

PartnerDeviceAgentServer::PartnerDeviceAgentServer() : SystemAbility(PARTNER_DEVICE_AGENT_SYS_ABILITY_ID, true)
{
    HILOGI("PartnerDeviceAgentServer enter");
    // 该注册仅仅为向sa_main进程注册（即需要向samgr注册请求的SA，onStart 调用Publish才真正完成SAMGR的注册）
    pimpl = std::make_unique<impl>();

    permissionsMap_ = {
        {
            static_cast<int>(IPartnerDeviceAgentIpcCode::COMMAND_BIND_DEVICE),
            PermissionItem(PUBLIC_API, PERMISSION_ACCESS_BLUETOOTH)
        },
        {
            static_cast<int>(IPartnerDeviceAgentIpcCode::COMMAND_UNBIND_DEVICE),
            PermissionItem(PUBLIC_API, PERMISSION_ACCESS_BLUETOOTH)
        },
        {
            static_cast<int>(IPartnerDeviceAgentIpcCode::COMMAND_IS_DEVICE_BOUND),
            PermissionItem(PUBLIC_API, PERMISSION_ACCESS_BLUETOOTH)
        },
        {
            static_cast<int>(IPartnerDeviceAgentIpcCode::COMMAND_GET_BOUND_DEVICES),
            PermissionItem(PUBLIC_API, PERMISSION_ACCESS_BLUETOOTH)
        },
        {
            static_cast<int>(IPartnerDeviceAgentIpcCode::COMMAND_ENABLE_DEVICE_CONTROL),
            PermissionItem(SYSTEM_API, PERMISSION_ACCESS_BLUETOOTH)
        },
        {
            static_cast<int>(IPartnerDeviceAgentIpcCode::COMMAND_DISABLE_DEVICE_CONTROL),
            PermissionItem(SYSTEM_API, PERMISSION_ACCESS_BLUETOOTH)
        },
        {
            static_cast<int>(IPartnerDeviceAgentIpcCode::COMMAND_IS_DEVICE_CONTROL_ENABLED),
            PermissionItem(PUBLIC_API, PERMISSION_ACCESS_BLUETOOTH)
        },
    };
}

std::mutex PartnerDeviceAgentServer::instanceMutex_;
sptr<PartnerDeviceAgentServer> PartnerDeviceAgentServer::instance_ = nullptr;

sptr<PartnerDeviceAgentServer> PartnerDeviceAgentServer::GetInstance()
{
    std::lock_guard<std::mutex> locker(instanceMutex_);
    if (instance_ == nullptr) {
        instance_ = new (std::nothrow) PartnerDeviceAgentServer();
        if (instance_ == nullptr) {
            HILOGE("Failed to create PartnerDeviceAgentServer instance.");
            return nullptr;
        }
    }
    return instance_;
}

int32_t PartnerDeviceAgentServer::CallbackEnter(uint32_t code)
{
    HILOGD("PartnerDeviceAgentServer CallbackEnter ipc code: %{public}u", code);
    auto it = permissionsMap_.find(code);
    if (it == permissionsMap_.end()) {
        HILOGE("Unknown ipc code: %{public}u", code);
        return FCM_ERR_API_NOT_SUPPORT;
    }

    return PermissionManager::VerifyPermissions(it->second);
}

int32_t PartnerDeviceAgentServer::CallbackExit([[maybe_unused]] uint32_t code, [[maybe_unused]] int32_t result)
{
    return FCM_NO_ERROR;
}

PartnerDeviceAgentServer::~PartnerDeviceAgentServer()
{
    HILOGI("~PartnerDeviceAgentServer enter");
}

std::shared_ptr<PartnerDevice> PartnerDeviceAgentServer::CreatePartnerDeviceInstance(
    PartnerDevice::DeviceInfo &deviceInfo)
{
    auto updateConfig = [this]() {
        // 切换线程环境
        ffrt::submit([this]() {
            UpdatePartnerDeviceConfig(partnerDeviceMap_);
        });
    };
    auto discoverExtension = [this](std::string bundleName,
        std::string abilityName, PartnerDeviceAddress deviceAddress) {
        OnDeviceDiscoveredExtensionService(bundleName, abilityName, deviceAddress);
    };
    auto destroyExtension = [this](std::string bundleName, std::string abilityName, int destroyReason) {
        OnDestroyWithReasonExtensionService(bundleName, abilityName, destroyReason);
    };

    PartnerDevice::DependencyFuncs funcs = {
        .updateConfig = updateConfig,
        .discoverExtension = discoverExtension,
        .destroyExtension = destroyExtension,
    };
    return PartnerDevice::CreateInstance(deviceInfo, funcs);
}

int PartnerDeviceAgentServer::ChangeDeviceControlState(
    const std::string &addr, bool isEnabled)
{
    int ret = FCM_ERR_DEVICE_NOT_FOUND;
    partnerDeviceMap_.Iterate(
        [&addr, &isEnabled, &ret](const PartnerDeviceMapKey &key, std::shared_ptr<PartnerDevice> &deviceSptr) {
        if (!deviceSptr) {
            return;
        }
        PartnerDevice::DeviceInfo deviceInfo = deviceSptr->GetDeviceInfo();
        if (deviceInfo.deviceAddress.GetAddress() == addr) {
            deviceSptr->SetUserEnableAbility(isEnabled);
            ret = FCM_NO_ERROR;
        }
    });
    UpdatePartnerDeviceConfig(partnerDeviceMap_);
    return ret;
}

void PartnerDeviceAgentServer::AttemptUnloadPartnerAgent()
{
    if (partnerDeviceMap_.IsEmpty()) {
        HILOGI("Attempt unload sa after 30s");
        // 变更系统参数值，触发一次系统参数事件变化卸载SA，SaMgr自动延迟30s卸载SA
        SetParameter(SYS_PARAM_ENABLE_PARTNER_AGENT, SYS_PARAM_ENABLE_PARTNER_AGENT_ENABLED);
        SetParameter(SYS_PARAM_ENABLE_PARTNER_AGENT, SYS_PARAM_ENABLE_PARTNER_AGENT_DISABLED);
    }
}

ErrCode PartnerDeviceAgentServer::BindDevice(
    const PartnerDeviceAddress &deviceAddress, const DeviceCapability &capability,
    const BusinessCapability &businessCapability, const std::string &partnerAgentExtensionAbilityName)
{
    if (BluetoothHost::GetDefaultHost().GetBluetoothState() != BluetoothState::STATE_ON) {
        HILOGE("bluetooth is not open");
        AttemptUnloadPartnerAgent();
        return FCM_ERR_BLUETOOTH_IS_OFF;
    }
    // 是否支持绑定虚拟地址，固化虚拟地址？
    if (deviceAddress.GetAddressType() == BluetoothAddressType::VIRTUAL) {
        HILOGE("not support this address");
        AttemptUnloadPartnerAgent();
        return FCM_ERR_API_NOT_SUPPORT;
    }
    // 检查设备是否配对
    if (!IsPairedDevice(deviceAddress)) {
        HILOGE("%{public}s device is not paired", GET_ENCRYPT_ADDR(deviceAddress));
        AttemptUnloadPartnerAgent();
        return FCM_ERR_DEVICE_NOT_PAIRED;
    }
    // 检查该设备是否已注册
    if (IsDeviceBoundByCallingApp(deviceAddress)) {
        HILOGE("%{public}s device is already bound", GET_ENCRYPT_ADDR(deviceAddress));
        AttemptUnloadPartnerAgent();
        return FCM_ERR_DEVICE_ALREADY_BOUNDED;
    }

    HILOGI("%{public}s bind device %{public}s",
        PermissionManager::GetCallingName().c_str(), GET_ENCRYPT_ADDR(deviceAddress));
    auto key = std::make_pair<uint32_t, std::string>(IPCSkeleton::GetCallingTokenID(), deviceAddress.GetAddress());
    PartnerDevice::DeviceInfo deviceInfo = {
        .bundleName = PermissionManager::GetCallingName(),
        .abilityName = partnerAgentExtensionAbilityName,
        .deviceAddress = deviceAddress,
        .tokenId = IPCSkeleton::GetCallingTokenID(),
        .registerTimestamp = GetSecondsSince1970ToNow(),
        .lostTimestamp = 0,
        .capability = capability,
        .businessCapability = businessCapability,
    };

    // 固化虚拟地址
    auto deviceSptr = CreatePartnerDeviceInstance(deviceInfo);
    partnerDeviceMap_.EnsureInsert(key, deviceSptr);
    UpdatePartnerDeviceConfig(partnerDeviceMap_);
    // 设置SA自启动标记
    SetParameter(SYS_PARAM_ENABLE_PARTNER_AGENT, SYS_PARAM_ENABLE_PARTNER_AGENT_ENABLED);
    return FCM_NO_ERROR;
}

bool PartnerDeviceAgentServer::IsPairedDevice(const PartnerDeviceAddress &deviceAddress)
{
    int pairState = PAIR_NONE;
    auto device = BluetoothHost::GetDefaultHost().GetRemoteDevice(
        deviceAddress.GetAddress(), Bluetooth::BTTransport::ADAPTER_BREDR);
    device.GetPairState(pairState);
    return pairState == PAIR_PAIRED;
}

ErrCode PartnerDeviceAgentServer::UnbindDevice(const PartnerDeviceAddress &deviceAddress)
{
    if (deviceAddress.GetAddressType() == BluetoothAddressType::VIRTUAL ||
        deviceAddress.GetRawAddressType() == BluetoothRawAddressType::RANDOM) {
        HILOGE("not support this address");
        AttemptUnloadPartnerAgent();
        return FCM_ERR_API_NOT_SUPPORT;
    }
    // 检查设备是否已注册过
    if (!IsDeviceBoundByCallingApp(deviceAddress)) {
        HILOGE("%{public}s device is not bound", GET_ENCRYPT_ADDR(deviceAddress));
        AttemptUnloadPartnerAgent();
        return FCM_ERR_DEVICE_NOT_FOUND;
    }

    HILOGI("%{public}s unbind device %{public}s",
        PermissionManager::GetCallingName().c_str(), GET_ENCRYPT_ADDR(deviceAddress));
    auto key = std::make_pair(IPCSkeleton::GetCallingTokenID(), deviceAddress.GetAddress());
    std::shared_ptr<PartnerDevice> deviceSptr = nullptr;
    partnerDeviceMap_.Find(key, deviceSptr);
    if (deviceSptr) {
        deviceSptr->Close();
    }
    // 清除虚拟MAC固化
    partnerDeviceMap_.Erase(key);
    UpdatePartnerDeviceConfig(partnerDeviceMap_);
    AttemptUnloadPartnerAgent();
    return FCM_NO_ERROR;
}

ErrCode PartnerDeviceAgentServer::IsDeviceBound(const PartnerDeviceAddress &deviceAddress, bool &isBound)
{
    if (deviceAddress.GetAddressType() == BluetoothAddressType::VIRTUAL ||
        deviceAddress.GetRawAddressType() == BluetoothRawAddressType::RANDOM) {
        HILOGE("not support this address");
        AttemptUnloadPartnerAgent();
        return FCM_ERR_API_NOT_SUPPORT;
    }

    isBound = PermissionManager::IsSystemCaller() ?
        IsDeviceBoundByAll(deviceAddress) : IsDeviceBoundByCallingApp(deviceAddress);

    AttemptUnloadPartnerAgent();
    return FCM_NO_ERROR;
}

ErrCode PartnerDeviceAgentServer::GetBoundDevices(std::vector<PartnerDeviceAddress> &deviceAddressVec)
{
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    partnerDeviceMap_.Iterate(
        [tokenId, &deviceAddressVec](const PartnerDeviceMapKey &key, std::shared_ptr<PartnerDevice> &deviceSptr) {
        if (key.first == tokenId && deviceSptr != nullptr) {
            PartnerDevice::DeviceInfo info = deviceSptr->GetDeviceInfo();
            deviceAddressVec.push_back(info.deviceAddress);
        }
    });
    return FCM_NO_ERROR;
}

bool PartnerDeviceAgentServer::IsDeviceBoundByCallingApp(const PartnerDeviceAddress &deviceAddress)
{
    auto key = std::make_pair(IPCSkeleton::GetCallingTokenID(), deviceAddress.GetAddress());
    std::shared_ptr<PartnerDevice> deviceSptr = nullptr;
    bool isBound = partnerDeviceMap_.Find(key, deviceSptr);
    return isBound;
}

bool PartnerDeviceAgentServer::IsDeviceBoundByAll(const PartnerDeviceAddress &deviceAddress)
{
    bool isBound = false;
    partnerDeviceMap_.Iterate(
        [&deviceAddress, &isBound](const PartnerDeviceMapKey &key, std::shared_ptr<PartnerDevice> &deviceSptr) {
        if (!deviceSptr) {
            return;
        }
        PartnerDevice::DeviceInfo deviceInfo = deviceSptr->GetDeviceInfo();
        if (deviceInfo.deviceAddress.GetAddress() == deviceAddress.GetAddress()) {
            isBound = true;
        }
    });
    return isBound;
}

ErrCode PartnerDeviceAgentServer::EnableDeviceControl(const PartnerDeviceAddress &deviceAddress)
{
    HILOGI("%{public}s enable device control %{public}s",
        PermissionManager::GetCallingName().c_str(), GET_ENCRYPT_ADDR(deviceAddress));
    return ChangeDeviceControlState(deviceAddress.GetAddress(), true);
}

ErrCode PartnerDeviceAgentServer::DisableDeviceControl(const PartnerDeviceAddress &deviceAddress)
{
    HILOGI("%{public}s disable device control %{public}s",
        PermissionManager::GetCallingName().c_str(), GET_ENCRYPT_ADDR(deviceAddress));
    return ChangeDeviceControlState(deviceAddress.GetAddress(), false);
}

ErrCode PartnerDeviceAgentServer::IsDeviceControlEnabled(const PartnerDeviceAddress &deviceAddress, bool &isEnabled)
{
    isEnabled = false;
    partnerDeviceMap_.Iterate(
        [&deviceAddress, &isEnabled](const PartnerDeviceMapKey &key, std::shared_ptr<PartnerDevice> &deviceSptr) {
        if (!deviceSptr) {
            return;
        }
        PartnerDevice::DeviceInfo deviceInfo = deviceSptr->GetDeviceInfo();
        if (deviceInfo.deviceAddress.GetAddress() == deviceAddress.GetAddress()) {
            isEnabled = deviceInfo.isUserEnabled;
        }
    });

    AttemptUnloadPartnerAgent();
    return FCM_NO_ERROR;
}

void PartnerDeviceAgentServer::Init()
{
    CreatePartnerDeviceFunc func = [this](PartnerDevice::DeviceInfo &deviceInfo) {
        return CreatePartnerDeviceInstance(deviceInfo);
    };
    LoadPartnerDeviceConfig(partnerDeviceMap_, func);
    if (partnerDeviceMap_.IsEmpty()) {
        AttemptUnloadPartnerAgent();
    }
}

void PartnerDeviceAgentServer::OnStart()
{
    HILOGI("PartnerDeviceAgentServer starting service.");
    Init();
    bool res = Publish(this);
    HILOGI("Publish result is %{public}d.", res);
    return;
}

void PartnerDeviceAgentServer::OnStop()
{
    HILOGI("stopping service.");

    return;
}

int32_t PartnerDeviceAgentServer::OnIdle(const SystemAbilityOnDemandReason& idleReason)
{
    const int SA_DISABLE_STATE = 0;
    int enablePartnerAgentParam = GetIntParameter(SYS_PARAM_ENABLE_PARTNER_AGENT, SA_DISABLE_STATE);
    HILOGI("Idle reason: %{public}s", idleReason.GetName().c_str());
    if (enablePartnerAgentParam != SA_DISABLE_STATE) {
        HILOGI("persist.fusion_connectivity.enable_partner_agent is %{public}d, not allow enter idle",
            enablePartnerAgentParam);
        return -1;
    }
    return 0;
}

int32_t PartnerDeviceAgentServer::ConnectExtensionService(const std::string &bundleName, const std::string &abilityName)
{
    std::string path_ = PARTNER_AGENT_EXTENSION_SERVICE_MODULE_NAME;
    if (partnerAgentExtensionHandler_ == nullptr) {
        partnerAgentExtensionHandler_ =
            std::make_shared<FusionConnectivityLoadUtils>(PARTNER_AGENT_EXTENSION_SERVICE_MODULE_NAME);
    }
    // 一个partnerAgentSA可以拉起多个extension
    CONNECT connect = reinterpret_cast<CONNECT>(partnerAgentExtensionHandler_->GetProxyFunc("Connect"));
    if (connect == nullptr) {
        HILOGW("GetProxyFunc Connect init failed.");
        return -1;
    }
    connect(bundleName, abilityName, GetCurrentActiveUserId());
    partnerAgentExtensionLoaded_.store(true);
    return 0;
}

int32_t PartnerDeviceAgentServer::OnDestroyWithReasonExtensionService(
    const std::string &bundleName, const std::string &abilityName, int destroyReason)
{
    std::string path_ = PARTNER_AGENT_EXTENSION_SERVICE_MODULE_NAME;
    partnerAgentExtensionHandler_ =
        std::make_shared<FusionConnectivityLoadUtils>(PARTNER_AGENT_EXTENSION_SERVICE_MODULE_NAME);
    if (partnerAgentExtensionHandler_ == nullptr) {
        HILOGW("notificationExtensionHandle init failed.");
        return -1;
    }
    // 一个partnerAgentSA可以拉起多个extension
    ONDESTROYWITHREASON OnDestroyWithReason =
        reinterpret_cast<ONDESTROYWITHREASON>(partnerAgentExtensionHandler_->GetProxyFunc("OnDestroyWithReason"));
    if (OnDestroyWithReason == nullptr) {
        HILOGW("GetProxyFunc OnDestroyWithReason init failed.");
        return -1;
    }
    OnDestroyWithReason(bundleName, abilityName, GetCurrentActiveUserId(), destroyReason);
    HILOGW("OnDestroyWithReason end.");
    partnerAgentExtensionLoaded_.store(true);
    return 0;
}

int32_t PartnerDeviceAgentServer::OnDeviceDiscoveredExtensionService(
    const std::string &bundleName, const std::string &abilityName, PartnerDeviceAddress deviceAddress)
{
    std::string path_ = PARTNER_AGENT_EXTENSION_SERVICE_MODULE_NAME;
    partnerAgentExtensionHandler_ =
        std::make_shared<FusionConnectivityLoadUtils>(PARTNER_AGENT_EXTENSION_SERVICE_MODULE_NAME);
    if (partnerAgentExtensionHandler_ == nullptr) {
        HILOGW("notificationExtensionHandle init failed.");
        return -1;
    }
    HILOGW("onDeviceDiscovered begin.");
    // 一个partnerAgentSA可以拉起多个extension
    ONDEVICEDISCOVERED onDeviceDiscovered =
        reinterpret_cast<ONDEVICEDISCOVERED>(partnerAgentExtensionHandler_->GetProxyFunc("OnDeviceDiscovered"));
    if (onDeviceDiscovered == nullptr) {
        HILOGW("GetProxyFunc onDeviceDiscovered init failed.");
        return -1;
    }
    onDeviceDiscovered(bundleName,
        abilityName, GetCurrentActiveUserId(), deviceAddress, NotificationType::TELEPHONY_CONTROL_ONLY);
    HILOGW("onDeviceDiscovered end.");
    partnerAgentExtensionLoaded_.store(true);
    return 0;
}
}  // namespace FusionConnectivity
}  // namespace OHOS