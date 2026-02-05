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
#define LOG_TAG "PartnerDeviceAgentFwk"
#endif

#include "partner_device_agent.h"
#include "partner_device_agent_proxy.h"
#include "iservice_registry.h"
#include "system_ability_status_change_stub.h"
#include "parameter.h"
#include "parameters.h"
#include "log.h"

namespace OHOS {
namespace FusionConnectivity {

namespace {
    const int32_t PARTNER_DEVICE_AGENT_SYS_ABILITY_ID = 8630;
    constexpr int32_t LOADSA_TIMEOUT_4S = 4;
}

static sptr<IPartnerDeviceAgent> GetRemoteProxy();

struct PartnerDeviceAgent::impl {
    class FcmSystemAbility : public SystemAbilityStatusChangeStub {
    public:
        void OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override;
        void OnRemoveSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override;

    private:
        std::mutex isSaRemovedMutex_ {};
        bool isSaRemoved_ = false;
    };

    bool IsAnyDeviceBound();

    impl();
    ~impl();
    sptr<FcmSystemAbility> fcmSystemAbility_;
};

void PartnerDeviceAgent::impl::FcmSystemAbility::OnAddSystemAbility(int32_t systemAbilityId,
    const std::string &deviceId)
{
    HILOGI("systemAbilityId:%{public}d", systemAbilityId);
    if (systemAbilityId == PARTNER_DEVICE_AGENT_SYS_ABILITY_ID) {
        auto proxy = GetRemoteProxy();
        if (!proxy) {
            HILOGE("Get proxy failed");
            return;
        }
        // 首次创建对象，监听SA状态时，会自动进入OnAddSystemAbility函数，此次需要避免重复调用。
        std::lock_guard<std::mutex> lock(isSaRemovedMutex_);
        if (isSaRemoved_) {
            isSaRemoved_ = false;
        }
    }
}

void PartnerDeviceAgent::impl::FcmSystemAbility::OnRemoveSystemAbility(
    int32_t systemAbilityId, const std::string &deviceId)
{
    HILOGI("systemAbilityId:%{public}d", systemAbilityId);
    std::lock_guard<std::mutex> lock(isSaRemovedMutex_);
    isSaRemoved_ = true;
}

PartnerDeviceAgent::impl::impl()
{
    HILOGI("PartnerDeviceAgent impl()");

    fcmSystemAbility_ = sptr<FcmSystemAbility>::MakeSptr();
    sptr<ISystemAbilityManager> samgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    int ret = samgrProxy->SubscribeSystemAbility(PARTNER_DEVICE_AGENT_SYS_ABILITY_ID, fcmSystemAbility_);
    if (ret != FCM_NO_ERROR) {
        HILOGE("SubscribeSystemAbility failed");
    }
}

PartnerDeviceAgent::impl::~impl()
{
    HILOGI("PartnerDeviceAgent ~impl()");
}

// 缓存机制，避免无效拉起SA
bool PartnerDeviceAgent::impl::IsAnyDeviceBound()
{
    int size = GetIntParameter("persist.fusion_connectivity.partner_agent_devices", 0);
    return size > 0;
}

PartnerDeviceAgent::PartnerDeviceAgent()
{
    HILOGI("PartnerDeviceAgent constructed.");
    pimpl = std::make_unique<impl>();
}

PartnerDeviceAgent::~PartnerDeviceAgent()
{
    HILOGI("~PartnerDeviceAgent destroyed.");
}

PartnerDeviceAgent *PartnerDeviceAgent::GetInstance()
{
    static PartnerDeviceAgent instance;
    return &instance;
}

static sptr<IPartnerDeviceAgent> GetRemoteProxy()
{
    auto samgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (!samgrProxy) {
        HILOGE("samgrProxy is nullptr.");
        return nullptr;
    }

    sptr<IRemoteObject> remote = nullptr;
    remote = samgrProxy->CheckSystemAbility(PARTNER_DEVICE_AGENT_SYS_ABILITY_ID);
    if (remote == nullptr) {
        HILOGE("failed is nullptr.");
        return nullptr;
    }

    sptr<IPartnerDeviceAgent> proxy = iface_cast<IPartnerDeviceAgent>(remote);
    if (proxy == nullptr) {
        HILOGE("failed: no proxy");
        return nullptr;
    }
    return proxy;
}

static sptr<IPartnerDeviceAgent> LoadRemoteSa()
{
    auto samgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (!samgrProxy) {
        HILOGE("samgrProxy is nullptr.");
        return nullptr;
    }
    sptr<IRemoteObject> remote = nullptr;
    remote = samgrProxy->LoadSystemAbility(PARTNER_DEVICE_AGENT_SYS_ABILITY_ID, LOADSA_TIMEOUT_4S);
    if (remote == nullptr) {
        HILOGE("failed is nullptr.");
        return nullptr;
    }
    sptr<IPartnerDeviceAgent> proxy = iface_cast<IPartnerDeviceAgent>(remote);
    if (proxy == nullptr) {
        HILOGE("failed: no proxy");
        return nullptr;
    }
    return proxy;
}

bool PartnerDeviceAgent::IsPartnerAgentSupported(void)
{
#ifdef ENABLE_PARTNER_AGENT_FEATURE
    return true;
#else
    return false;
#endif
}

int PartnerDeviceAgent::BindDevice(const PartnerDeviceAddress &deviceAddress, const DeviceCapability &capability,
    const BusinessCapability &businessCapability, const std::string &abilityName)
{
    if (!IsPartnerAgentSupported()) {
        return FCM_ERR_API_NOT_SUPPORT;
    }

    auto proxy = LoadRemoteSa();
    FCM_CHECK_RETURN_RET(proxy != nullptr, FCM_ERR_INTERNAL_ERROR, "proxy is nullptr");
    return proxy->BindDevice(deviceAddress, capability, businessCapability, abilityName);
}

int PartnerDeviceAgent::UnbindDevice(const PartnerDeviceAddress &deviceAddress)
{
    if (!IsPartnerAgentSupported()) {
        return FCM_ERR_API_NOT_SUPPORT;
    }

    auto proxy = LoadRemoteSa();
    FCM_CHECK_RETURN_RET(proxy != nullptr, FCM_ERR_INTERNAL_ERROR, "proxy is nullptr");
    return proxy->UnbindDevice(deviceAddress);
}

int PartnerDeviceAgent::IsDeviceBound(const PartnerDeviceAddress &deviceAddress, bool &isBound)
{
    if (!IsPartnerAgentSupported()) {
        return FCM_ERR_API_NOT_SUPPORT;
    }
    if (!pimpl->IsAnyDeviceBound()) {
        isBound = false;
        return FCM_NO_ERROR;
    }

    auto proxy = LoadRemoteSa();
    FCM_CHECK_RETURN_RET(proxy != nullptr, FCM_ERR_INTERNAL_ERROR, "proxy is nullptr");
    return proxy->IsDeviceBound(deviceAddress, isBound);
}

int PartnerDeviceAgent::EnableDeviceControl(const PartnerDeviceAddress &deviceAddress)
{
    if (!IsPartnerAgentSupported()) {
        return FCM_ERR_API_NOT_SUPPORT;
    }

    auto proxy = LoadRemoteSa();
    FCM_CHECK_RETURN_RET(proxy != nullptr, FCM_ERR_INTERNAL_ERROR, "proxy is nullptr");
    return proxy->EnableDeviceControl(deviceAddress);
}

int PartnerDeviceAgent::DisableDeviceControl(const PartnerDeviceAddress &deviceAddress)
{
    if (!IsPartnerAgentSupported()) {
        return FCM_ERR_API_NOT_SUPPORT;
    }

    auto proxy = LoadRemoteSa();
    FCM_CHECK_RETURN_RET(proxy != nullptr, FCM_ERR_INTERNAL_ERROR, "proxy is nullptr");
    return proxy->DisableDeviceControl(deviceAddress);
}

int PartnerDeviceAgent::IsDeviceControlEnabled(const PartnerDeviceAddress &deviceAddress, bool &isEnabled)
{
    if (!IsPartnerAgentSupported()) {
        return FCM_ERR_API_NOT_SUPPORT;
    }
    if (!pimpl->IsAnyDeviceBound()) {
        isEnabled = false;
        return FCM_NO_ERROR;
    }

    auto proxy = LoadRemoteSa();
    FCM_CHECK_RETURN_RET(proxy != nullptr, FCM_ERR_INTERNAL_ERROR, "proxy is nullptr");
    return proxy->IsDeviceControlEnabled(deviceAddress, isEnabled);
}

int PartnerDeviceAgent::GetBoundDevices(std::vector<PartnerDeviceAddress> &deviceAddressVec)
{
    if (!IsPartnerAgentSupported()) {
        return FCM_ERR_API_NOT_SUPPORT;
    }

    auto proxy = LoadRemoteSa();
    FCM_CHECK_RETURN_RET(proxy != nullptr, FCM_ERR_INTERNAL_ERROR, "proxy is nullptr");
    return proxy->GetBoundDevices(deviceAddressVec);
}

}  // namespace FusionConnectivity
}  // namespace OHOS
