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
#define LOG_TAG "PatnerDeviceAgentNapi"
#endif

#include "hilog/log.h"
#include "napi_partner_device_agent.h"
#include "napi_fusion_connectivity_error.h"

#include <memory>

#include "partner_device_agent.h"
#include "napi_async_work.h"

namespace OHOS {
namespace FusionConnectivity {

void DefinePartnerDeviceAgentInterface(napi_env env, napi_value exports)
{
    DefinePartnerDeviceAgentProperty(env, exports);

    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("isPartnerAgentSupported", IsPartnerAgentSupported),
        DECLARE_NAPI_FUNCTION("bindDevice", BindDevice),
        DECLARE_NAPI_FUNCTION("unbindDevice", UnbindDevice),
        DECLARE_NAPI_FUNCTION("isDeviceBound", IsDeviceBound),
        DECLARE_NAPI_FUNCTION("getBoundDevices", GetBoundDevices),
        DECLARE_NAPI_FUNCTION("enableDeviceControl", EnableDeviceControl),
        DECLARE_NAPI_FUNCTION("disableDeviceControl", DisableDeviceControl),
        DECLARE_NAPI_FUNCTION("isDeviceControlEnabled", IsDeviceControlEnabled),
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
}

static napi_value DefinePartnerAgentExtensionAbilityDestroyReason(napi_env env)
{
    napi_value obj = nullptr;
    napi_create_object(env, &obj);
    SetNamedPropertyByInteger(env, obj,
        static_cast<int32_t>(PartnerAgentExtensionAbilityDestroyReason::UNKNOWN_REASON), "UNKNOWN_REASON");
    SetNamedPropertyByInteger(env, obj,
        static_cast<int32_t>(PartnerAgentExtensionAbilityDestroyReason::USER_CLOSED_ABILITY), "USER_CLOSED_ABILITY");
    SetNamedPropertyByInteger(env, obj,
        static_cast<int32_t>(PartnerAgentExtensionAbilityDestroyReason::DEVICE_UNPAIRED), "DEVICE_UNPAIRED");
    SetNamedPropertyByInteger(env, obj,
        static_cast<int32_t>(PartnerAgentExtensionAbilityDestroyReason::DEVICE_LOST), "DEVICE_LOST");
    SetNamedPropertyByInteger(env, obj,
        static_cast<int32_t>(PartnerAgentExtensionAbilityDestroyReason::BLUETOOTH_DISABLED), "BLUETOOTH_DISABLED");
    return obj;
}

void DefinePartnerDeviceAgentProperty(napi_env env, napi_value exports)
{
    napi_value partnerAgentExtensionDestroyReasonObj = DefinePartnerAgentExtensionAbilityDestroyReason(env);
    napi_property_descriptor exportProperties[] = {
        DECLARE_NAPI_PROPERTY("PartnerAgentExtensionAbilityDestroyReason", partnerAgentExtensionDestroyReasonObj),
    };
    napi_define_properties(env, exports, sizeof(exportProperties) / sizeof(*exportProperties), exportProperties);
}

static napi_status NapiParsePartnerDeviceAddress(napi_env env, napi_value object, PartnerDeviceAddress &outAddr)
{
    bool hasProperty = false;
    NAPI_FCM_CALL_RETURN(napi_has_named_property(env, object, "bluetoothAddress", &hasProperty));
    if (!hasProperty) {
        HILOGE("bluetoothAddress property is needed");
        return napi_invalid_arg;
    }
    napi_value bluetoothAddressValue;
    NAPI_FCM_CALL_RETURN(napi_get_named_property(env, object, "bluetoothAddress", &bluetoothAddressValue));
    napi_valuetype valuetype;
    NAPI_FCM_CALL_RETURN(napi_typeof(env, bluetoothAddressValue, &valuetype));
    NAPI_FCM_RETURN_IF(valuetype != napi_object, "Wrong argument type, object expected", napi_object_expected);

    std::string address;
    int32_t addressType;
    int32_t rawAddressType;
    NAPI_FCM_CALL_RETURN(NapiParseStringObject(env, bluetoothAddressValue, "address", address));
    NAPI_FCM_RETURN_IF(!IsValidAddress(address), "Invalid address", napi_invalid_arg);
    NAPI_FCM_CALL_RETURN(NapiParseInt32Object(env, bluetoothAddressValue, "addressType", addressType));

    bool isExist = false;
    NAPI_FCM_CALL_RETURN(
        NapiParseInt32ObjectOptional(env, bluetoothAddressValue, "rawAddressType", isExist, rawAddressType));
    if (isExist) {
        outAddr = PartnerDeviceAddress(address,
            static_cast<BluetoothAddressType>(addressType), static_cast<BluetoothRawAddressType>(rawAddressType));
    } else {
        outAddr = PartnerDeviceAddress(address, static_cast<BluetoothAddressType>(addressType));
    }
    return napi_ok;
}

static napi_status NapiParseDeviceCapability(napi_env env, napi_value object, DeviceCapability &capability)
{
    bool isSupportBR = false;
    bool isSupportBleAdvertiser = false;

    bool isExist = false;
    NAPI_FCM_CALL_RETURN(NapiParseBoolObjectOptional(env, object, "supportBR", isExist, isSupportBR));
    NAPI_FCM_CALL_RETURN(
        NapiParseBoolObjectOptional(env, object, "supportBleAdvertiser", isExist, isSupportBleAdvertiser));
    capability.isSupportBR = isSupportBR;
    capability.isSupportBleAdvertiser = isSupportBleAdvertiser;
    return napi_ok;
}

static napi_status NapiParseBusinessCapability(napi_env env, napi_value object, BusinessCapability &capability)
{
    bool isSupportMediaControl = false;
    bool isSupportTelephonyControl = false;

    bool isExist = false;
    NAPI_FCM_CALL_RETURN(
        NapiParseBoolObjectOptional(env, object, "supportMediaControl", isExist, isSupportMediaControl));
    NAPI_FCM_CALL_RETURN(
        NapiParseBoolObjectOptional(env, object, "supportTelephonyControl", isExist, isSupportTelephonyControl));
    capability.isSupportMediaControl = isSupportMediaControl;
    capability.isSupportTelephonyControl = isSupportTelephonyControl;
    return napi_ok;
}

static napi_status NapiCheckBindDevice(napi_env env, napi_callback_info info, PartnerDeviceAddress &outDeviceAddress,
    DeviceCapability &outCapability, std::string &outAbilityName)
{
    size_t argc = ARGS_SIZE_FOUR;
    napi_value argv[ARGS_SIZE_FOUR] = {nullptr};
    napi_value thisVar = nullptr;
    NAPI_FCM_CALL_RETURN(napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr));
    NAPI_FCM_RETURN_IF(argc != ARGS_SIZE_FOUR, "need 4 parameter", napi_invalid_arg);

    PartnerDeviceAddress deviceAddress;
    NAPI_FCM_CALL_RETURN(NapiParsePartnerDeviceAddress(env, argv[PARAM0], deviceAddress));
    DeviceCapability capability;
    NAPI_FCM_CALL_RETURN(NapiParseDeviceCapability(env, argv[PARAM1], capability));
    BusinessCapability businessCapability;
    NAPI_FCM_CALL_RETURN(NapiParseBusinessCapability(env, argv[PARAM2], businessCapability));
    std::string abilityName;
    NAPI_FCM_CALL_RETURN(NapiParseString(env, argv[PARAM3], abilityName));

    outDeviceAddress = deviceAddress;
    outCapability = capability;
    outAbilityName = abilityName;
    return napi_ok;
}

napi_value BindDevice(napi_env env, napi_callback_info info)
{
    PartnerDeviceAddress deviceAddress;
    DeviceCapability capability;
    BusinessCapability businessCapability;
    std::string abilityName;
    NAPI_FCM_ASSERT_RETURN_UNDEF(
        env, NapiCheckBindDevice(env, info, deviceAddress, capability, abilityName) == napi_ok, FCM_ERR_INVALID_PARAM);

    auto func = [deviceAddress, capability, businessCapability, abilityName]() {
        int ret = PartnerDeviceAgent::GetInstance()->BindDevice(
            deviceAddress, capability, businessCapability, abilityName);
        return NapiAsyncWorkRet(ret);
    };
    auto asyncWork = NapiAsyncWorkFactory::CreateAsyncWork(env, info, func, ASYNC_WORK_NO_NEED_CALLBACK);
    NAPI_FCM_ASSERT_RETURN_UNDEF(env, asyncWork, FCM_ERR_INTERNAL_ERROR);
    asyncWork->Run();
    return asyncWork->GetRet();
}

static napi_status NapiCheckPartnerDeviceAddress(
    napi_env env, napi_callback_info info, PartnerDeviceAddress &deviceAddress)
{
    size_t argc = ARGS_SIZE_ONE;
    napi_value argv[ARGS_SIZE_ONE] = {nullptr};
    napi_value thisVar = nullptr;
    NAPI_FCM_CALL_RETURN(napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr));
    NAPI_FCM_RETURN_IF(argc != ARGS_SIZE_ONE, "need 1 parameter", napi_invalid_arg);
    NAPI_FCM_CALL_RETURN(NapiParsePartnerDeviceAddress(env, argv[PARAM0], deviceAddress));
    return napi_ok;
}

napi_value UnbindDevice(napi_env env, napi_callback_info info)
{
    PartnerDeviceAddress deviceAddress;
    NAPI_FCM_ASSERT_RETURN_UNDEF(
        env, NapiCheckPartnerDeviceAddress(env, info, deviceAddress) == napi_ok, FCM_ERR_INVALID_PARAM);

    auto func = [deviceAddress]() {
        int ret = PartnerDeviceAgent::GetInstance()->UnbindDevice(deviceAddress);
        return NapiAsyncWorkRet(ret);
    };
    auto asyncWork = NapiAsyncWorkFactory::CreateAsyncWork(env, info, func, ASYNC_WORK_NO_NEED_CALLBACK);
    NAPI_FCM_ASSERT_RETURN_UNDEF(env, asyncWork, FCM_ERR_INTERNAL_ERROR);
    asyncWork->Run();
    return asyncWork->GetRet();
}

napi_value IsDeviceBound(napi_env env, napi_callback_info info)
{
    PartnerDeviceAddress deviceAddress;
    NAPI_FCM_ASSERT_RETURN_UNDEF(
        env, NapiCheckPartnerDeviceAddress(env, info, deviceAddress) == napi_ok, FCM_ERR_INVALID_PARAM);

    bool isBound = false;
    int ret = PartnerDeviceAgent::GetInstance()->IsDeviceBound(deviceAddress, isBound);
    NAPI_FCM_ASSERT_RETURN_UNDEF(env, ret == FCM_NO_ERROR, ret);
    return NapiGetBooleanRet(env, isBound);
}

static napi_status CreatePartnerDeviceAddress(
    napi_env env, napi_value napiVec, const std::vector<PartnerDeviceAddress> &vec)
{
    size_t idx = 0;

    for (const auto &deviceAddress : vec) {
        napi_value napiPartnerDeviceAddress = nullptr;
        napi_create_object(env, &napiPartnerDeviceAddress);

        napi_value napiBluetoothAddress = nullptr;
        napi_create_object(env, &napiBluetoothAddress);

        napi_value nDeviceAddress = nullptr;
        napi_create_string_utf8(env, deviceAddress.GetAddress().c_str(), NAPI_AUTO_LENGTH, &nDeviceAddress);
        napi_set_named_property(env, napiBluetoothAddress, "address", nDeviceAddress);

        napi_value nDeviceAddressType = nullptr;
        napi_create_int32(env, static_cast<int32_t>(deviceAddress.GetAddressType()), &nDeviceAddressType);
        napi_set_named_property(env, napiBluetoothAddress, "addressType", nDeviceAddressType);

        if (deviceAddress.HasRawAddressType()) {
            napi_value nRawAddressType = nullptr;
            napi_create_int32(env, static_cast<int32_t>(deviceAddress.GetRawAddressType()), &nRawAddressType);
            napi_set_named_property(env, napiBluetoothAddress, "rawAddressType", nRawAddressType);
        }

        napi_set_named_property(env, napiPartnerDeviceAddress, "bluetoothAddress", napiBluetoothAddress);
        napi_set_element(env, napiVec, idx, napiPartnerDeviceAddress);
        idx++;
    }

    return napi_ok;
}

napi_value GetBoundDevices(napi_env env, napi_callback_info info)
{
    NAPI_FCM_ASSERT_RETURN_UNDEF(env, CheckEmptyParams(env, info) == napi_ok, FCM_ERR_INVALID_PARAM);
    std::vector<PartnerDeviceAddress> vec {};
    int ret = PartnerDeviceAgent::GetInstance()->GetBoundDevices(vec);
    NAPI_FCM_ASSERT_RETURN_UNDEF(env, ret == FCM_NO_ERROR, ret);

    napi_value napiObject;
    napi_create_array(env, &napiObject);
    CreatePartnerDeviceAddress(env, napiObject, vec);
    return napiObject;
}

napi_value IsPartnerAgentSupported(napi_env env, napi_callback_info info)
{
    NAPI_FCM_ASSERT_RETURN_UNDEF(env, CheckEmptyParams(env, info) == napi_ok, FCM_ERR_INVALID_PARAM);
    bool ret = PartnerDeviceAgent::GetInstance()->IsPartnerAgentSupported();
    HILOGI("IsPartnerAgentSupported enter result is %{public}d", ret);
    return NapiGetBooleanRet(env, ret);
}

napi_value EnableDeviceControl(napi_env env, napi_callback_info info)
{
    PartnerDeviceAddress deviceAddress;
    NAPI_FCM_ASSERT_RETURN_UNDEF(
        env, NapiCheckPartnerDeviceAddress(env, info, deviceAddress) == napi_ok, FCM_ERR_INVALID_PARAM);

    auto func = [deviceAddress]() {
        int ret = PartnerDeviceAgent::GetInstance()->EnableDeviceControl(deviceAddress);
        return NapiAsyncWorkRet(ret);
    };
    auto asyncWork = NapiAsyncWorkFactory::CreateAsyncWork(env, info, func, ASYNC_WORK_NO_NEED_CALLBACK);
    NAPI_FCM_ASSERT_RETURN_UNDEF(env, asyncWork, FCM_ERR_INTERNAL_ERROR);
    asyncWork->Run();
    return asyncWork->GetRet();
}

napi_value DisableDeviceControl(napi_env env, napi_callback_info info)
{
    PartnerDeviceAddress deviceAddress;
    NAPI_FCM_ASSERT_RETURN_UNDEF(
        env, NapiCheckPartnerDeviceAddress(env, info, deviceAddress) == napi_ok, FCM_ERR_INVALID_PARAM);

    auto func = [deviceAddress]() {
        int ret = PartnerDeviceAgent::GetInstance()->DisableDeviceControl(deviceAddress);
        return NapiAsyncWorkRet(ret);
    };
    auto asyncWork = NapiAsyncWorkFactory::CreateAsyncWork(env, info, func, ASYNC_WORK_NO_NEED_CALLBACK);
    NAPI_FCM_ASSERT_RETURN_UNDEF(env, asyncWork, FCM_ERR_INTERNAL_ERROR);
    asyncWork->Run();
    return asyncWork->GetRet();
}

napi_value IsDeviceControlEnabled(napi_env env, napi_callback_info info)
{
    PartnerDeviceAddress deviceAddress;
    NAPI_FCM_ASSERT_RETURN_UNDEF(
        env, NapiCheckPartnerDeviceAddress(env, info, deviceAddress) == napi_ok, FCM_ERR_INVALID_PARAM);

    bool isEnabled = true;
    int ret = PartnerDeviceAgent::GetInstance()->IsDeviceControlEnabled(deviceAddress, isEnabled);
    NAPI_FCM_ASSERT_RETURN_UNDEF(env, ret == FCM_NO_ERROR, ret);
    return NapiGetBooleanRet(env, isEnabled);
}

}  // namespace FusionConnectivity
}  // namespace OHOS
