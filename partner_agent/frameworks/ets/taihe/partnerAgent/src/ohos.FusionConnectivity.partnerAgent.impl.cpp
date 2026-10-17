/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of, use or distribute this file except in compliance with
 * the License. You may obtain a copy of the License at
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
#define LOG_TAG "TaihePartnerAgentImpl"
#endif

#include "ohos.FusionConnectivity.partnerAgent.impl.hpp"
#include "ohos.FusionConnectivity.partnerAgent.proj.hpp"
#include "taihe/runtime.hpp"
#include "stdexcept"
#include "taihe_fusion_connectivity_utils.h"
#include "taihe_fusion_connectivity_error.h"
#include "partner_device_agent.h"
#include "hilog/log.h"

namespace OHOS {
namespace FusionConnectivity {

bool IsPartnerAgentSupported()
{
    auto ret = PartnerDeviceAgent::GetInstance()->IsPartnerAgentSupported();
    HILOGI("IsPartnerAgentSupported enter result is %{public}d", ret);
    return ret;
}

void BindDevice(::ohos::FusionConnectivity::partnerAgent::PartnerDeviceAddress const &deviceAddress,
    ::ohos::FusionConnectivity::partnerAgent::DeviceCapability const &deviceCapability,
    ::ohos::FusionConnectivity::partnerAgent::BusinessCapability const &businessCapability,
    ::taihe::string_view partnerAgentExtensionAbilityName)
{
    std::vector<int32_t> validErrCodes = { FCM_ERR_PERMISSION_FAILED, FCM_ERR_INVALID_PARAM,
        FCM_ERR_API_NOT_SUPPORT, FCM_ERR_DEVICE_NOT_PAIRED, FCM_ERR_DEVICE_ALREADY_BOUNDED,
        FCM_ERR_BLUETOOTH_IS_OFF, FCM_ERR_INTERNAL_ERROR };
    TAIHE_FC_CONTEXT_WITHOUT_HA(validErrCodes);
    PartnerDeviceAddress address;
    TAIHE_FC_ASSERT_RETURN_VOID(PartnerDeviceAddressToNative(deviceAddress, address), FCM_ERR_INVALID_PARAM);
    TAIHE_FC_ASSERT_RETURN_VOID(IsValidAddr(address.GetAddress()), FCM_ERR_INVALID_PARAM);

    auto cap = DeviceCapabilityToNative(deviceCapability);
    auto businessCap = BusinessCapabilityToNative(businessCapability);
    std::string abilityName(partnerAgentExtensionAbilityName.c_str());

    auto ret = PartnerDeviceAgent::GetInstance()->BindDevice(address, cap, businessCap, abilityName);
    TAIHE_FC_ASSERT_RETURN_VOID_VERIFY(ret == FCM_NO_ERROR, ret);
}

void UnbindDevice(::ohos::FusionConnectivity::partnerAgent::PartnerDeviceAddress const &deviceAddress)
{
    std::vector<int32_t> validErrCodes = { FCM_ERR_PERMISSION_FAILED, FCM_ERR_INVALID_PARAM,
        FCM_ERR_API_NOT_SUPPORT, FCM_ERR_DEVICE_NOT_FOUND, FCM_ERR_INTERNAL_ERROR };
    TAIHE_FC_CONTEXT_WITHOUT_HA(validErrCodes);
    PartnerDeviceAddress address;
    TAIHE_FC_ASSERT_RETURN_VOID(PartnerDeviceAddressToNative(deviceAddress, address), FCM_ERR_INVALID_PARAM);
    TAIHE_FC_ASSERT_RETURN_VOID(IsValidAddr(address.GetAddress()), FCM_ERR_INVALID_PARAM);

    auto ret = PartnerDeviceAgent::GetInstance()->UnbindDevice(address);
    TAIHE_FC_ASSERT_RETURN_VOID_VERIFY(ret == FCM_NO_ERROR, ret);
}

bool IsDeviceBound(::ohos::FusionConnectivity::partnerAgent::PartnerDeviceAddress const &deviceAddress)
{
    std::vector<int32_t> validErrCodes = { FCM_ERR_PERMISSION_FAILED, FCM_ERR_INVALID_PARAM,
        FCM_ERR_API_NOT_SUPPORT, FCM_ERR_INTERNAL_ERROR };
    TAIHE_FC_CONTEXT_WITHOUT_HA(validErrCodes);
    PartnerDeviceAddress address;
    TAIHE_FC_ASSERT_RETURN(PartnerDeviceAddressToNative(deviceAddress, address), FCM_ERR_INVALID_PARAM, false);
    TAIHE_FC_ASSERT_RETURN(IsValidAddr(address.GetAddress()), FCM_ERR_INVALID_PARAM, false);

    bool isBound = false;
    auto ret = PartnerDeviceAgent::GetInstance()->IsDeviceBound(address, isBound);
    TAIHE_FC_ASSERT_RETURN_VERIFY(ret == FCM_NO_ERROR, ret, false);
    return isBound;
}

::taihe::array<::ohos::FusionConnectivity::partnerAgent::PartnerDeviceAddress> GetBoundDevices()
{
    std::vector<int32_t> validErrCodes = { FCM_ERR_PERMISSION_FAILED,
        FCM_ERR_API_NOT_SUPPORT, FCM_ERR_INTERNAL_ERROR };
    TAIHE_FC_CONTEXT_WITHOUT_HA(validErrCodes);
    std::vector<PartnerDeviceAddress> vec;
    auto ret = PartnerDeviceAgent::GetInstance()->GetBoundDevices(vec);
    TAIHE_FC_ASSERT_RETURN_VERIFY(ret == FCM_NO_ERROR, ret,
        ::taihe::array<::ohos::FusionConnectivity::partnerAgent::PartnerDeviceAddress>(0));

    std::vector<::ohos::FusionConnectivity::partnerAgent::PartnerDeviceAddress> result;
    for (const auto &device : vec) {
        result.push_back(PartnerDeviceAddressToTaihe(device));
    }
    return ::taihe::array<::ohos::FusionConnectivity::partnerAgent::PartnerDeviceAddress>(
        result.data(), result.size());
}

void EnableDeviceControl(::ohos::FusionConnectivity::partnerAgent::PartnerDeviceAddress const &deviceAddress)
{
    std::vector<int32_t> validErrCodes = { FCM_ERR_PERMISSION_FAILED, FCM_ERR_SYSTEM_PERMISSION_FAILED,
        FCM_ERR_INVALID_PARAM, FCM_ERR_API_NOT_SUPPORT, FCM_ERR_DEVICE_NOT_FOUND, FCM_ERR_INTERNAL_ERROR };
    TAIHE_FC_CONTEXT_WITHOUT_HA(validErrCodes);
    PartnerDeviceAddress address;
    TAIHE_FC_ASSERT_RETURN_VOID(PartnerDeviceAddressToNative(deviceAddress, address), FCM_ERR_INVALID_PARAM);
    TAIHE_FC_ASSERT_RETURN_VOID(IsValidAddr(address.GetAddress()), FCM_ERR_INVALID_PARAM);

    auto ret = PartnerDeviceAgent::GetInstance()->EnableDeviceControl(address);
    TAIHE_FC_ASSERT_RETURN_VOID_VERIFY(ret == FCM_NO_ERROR, ret);
}

void DisableDeviceControl(::ohos::FusionConnectivity::partnerAgent::PartnerDeviceAddress const &deviceAddress)
{
    std::vector<int32_t> validErrCodes = { FCM_ERR_PERMISSION_FAILED, FCM_ERR_SYSTEM_PERMISSION_FAILED,
        FCM_ERR_INVALID_PARAM, FCM_ERR_API_NOT_SUPPORT, FCM_ERR_DEVICE_NOT_FOUND, FCM_ERR_INTERNAL_ERROR };
    TAIHE_FC_CONTEXT_WITHOUT_HA(validErrCodes);
    PartnerDeviceAddress address;
    TAIHE_FC_ASSERT_RETURN_VOID(PartnerDeviceAddressToNative(deviceAddress, address), FCM_ERR_INVALID_PARAM);
    TAIHE_FC_ASSERT_RETURN_VOID(IsValidAddr(address.GetAddress()), FCM_ERR_INVALID_PARAM);

    auto ret = PartnerDeviceAgent::GetInstance()->DisableDeviceControl(address);
    TAIHE_FC_ASSERT_RETURN_VOID_VERIFY(ret == FCM_NO_ERROR, ret);
}

bool IsDeviceControlEnabled(::ohos::FusionConnectivity::partnerAgent::PartnerDeviceAddress const &deviceAddress)
{
    std::vector<int32_t> validErrCodes = { FCM_ERR_PERMISSION_FAILED, FCM_ERR_INVALID_PARAM,
        FCM_ERR_API_NOT_SUPPORT, FCM_ERR_INTERNAL_ERROR };
    TAIHE_FC_CONTEXT_WITHOUT_HA(validErrCodes);
    PartnerDeviceAddress address;
    TAIHE_FC_ASSERT_RETURN(PartnerDeviceAddressToNative(deviceAddress, address), FCM_ERR_INVALID_PARAM, false);
    TAIHE_FC_ASSERT_RETURN(IsValidAddr(address.GetAddress()), FCM_ERR_INVALID_PARAM, false);

    bool isEnabled = false;
    auto ret = PartnerDeviceAgent::GetInstance()->IsDeviceControlEnabled(address, isEnabled);
    TAIHE_FC_ASSERT_RETURN_VERIFY(ret == FCM_NO_ERROR, ret, false);
    return isEnabled;
}
}  // namespace FusionConnectivity
}  // namespace OHOS

// NOLINTBEGIN
TH_EXPORT_CPP_API_IsPartnerAgentSupported(OHOS::FusionConnectivity::IsPartnerAgentSupported);
TH_EXPORT_CPP_API_BindDevice(OHOS::FusionConnectivity::BindDevice);
TH_EXPORT_CPP_API_UnbindDevice(OHOS::FusionConnectivity::UnbindDevice);
TH_EXPORT_CPP_API_IsDeviceBound(OHOS::FusionConnectivity::IsDeviceBound);
TH_EXPORT_CPP_API_GetBoundDevices(OHOS::FusionConnectivity::GetBoundDevices);
TH_EXPORT_CPP_API_EnableDeviceControl(OHOS::FusionConnectivity::EnableDeviceControl);
TH_EXPORT_CPP_API_DisableDeviceControl(OHOS::FusionConnectivity::DisableDeviceControl);
TH_EXPORT_CPP_API_IsDeviceControlEnabled(OHOS::FusionConnectivity::IsDeviceControlEnabled);
// NOLINTEND
