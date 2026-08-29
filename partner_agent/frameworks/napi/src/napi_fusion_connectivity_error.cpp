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
#define LOG_TAG "FusionConnectivityNapiError"
#endif

#include "napi_fusion_connectivity_error.h"

#include <map>
#include "napi_ha_event_utils.h"
#include "fusion_ranging_errorcode.h"

namespace OHOS {
namespace FusionConnectivity {
static std::map<int32_t, std::string> napiErrMsgMap = {
    { FCM_ERR_PERMISSION_FAILED, "Permission check failed" },
    { FCM_ERR_SYSTEM_PERMISSION_FAILED, "System permission check failed" },
    { FCM_ERR_PROHIBITED_BY_EDM, "Operation is prohibited by edm" },
    { FCM_ERR_INVALID_PARAM, "Invalid parameters" },
    { FCM_ERR_API_NOT_SUPPORT, "Api is not supported" },
    { FCM_ERR_DEVICE_NOT_FOUND, "Device is not found"},
    { FCM_ERR_APPLICATION_NOT_SUPPORT, "The application is not support PartnerDeviceExtensionAbility"},
    { FCM_ERR_DEVICE_NOT_PAIRED, "The device is not paired"},
    { FCM_ERR_DEVICE_ALREADY_BOUNDED, "The device is already bound"},
    { FCM_ERR_BLUETOOTH_IS_OFF, "Bluetooth is off" },
    { FusionRanging::RANGING_ERR_DEVICE_NOT_INITIATED, "Device not initiated."},
    { FusionRanging::RANGING_ERR_DEVICE_ALREADY_INITIATED, "Device already initiated."},
    { FusionRanging::RANGING_ERR_RANGING_TYPE_NOT_SUPPORT, "Ranging type not support."},
    { FusionRanging::RANGING_ERR_RANGING_SERVICE_DISABLED, "Ranging service disabled"},
    { FusionRanging::RANGING_ERR_PARAM_NOT_MEET_SPECIFICATIONS, "Parameters not meet specifications."},
    { FCM_ERR_INTERNAL_ERROR, "Operation failed" },
};

std::string GetNapiErrMsg(napi_env env, int32_t errCode)
{
    auto iter = napiErrMsgMap.find(errCode);
    if (iter != napiErrMsgMap.end()) {
        std::string errMessage = "BusinessError ";
        errMessage.append(std::to_string(errCode)).append(": ").append(iter->second);
        return errMessage;
    }
    return "Inner error.";
}

static napi_value GenerateBusinessError(napi_env env, int32_t errCode, const std::string &errMsg)
{
    napi_value businessError = nullptr;
    napi_value code = nullptr;
    napi_create_int32(env, errCode, &code);

    napi_value message = nullptr;
    napi_create_string_utf8(env, errMsg.c_str(), NAPI_AUTO_LENGTH, &message);

    napi_create_error(env, nullptr, message, &businessError);
    napi_set_named_property(env, businessError, "code", code);
    return businessError;
}

void HandleSyncErr(napi_env env, int32_t errCode)
{
    if (errCode == FcmErrCode::FCM_NO_ERROR) {
        return;
    }
    NapiHaEventUtils::WriteErrCode(env, errCode);
    int ret = -1;
    std::string errMsg = GetNapiErrMsg(env, errCode);
    ret = napi_throw(env, GenerateBusinessError(env, errCode, errMsg));
    FCM_CHECK_RETURN(ret == napi_ok, "napi_throw failed, ret: %{public}d", ret);
}

static std::map<int32_t, int32_t> innerToBusinessErrCodeMap {
    // inner error code ->
    // business error code (ARKTS API, file: fusion_connectivity_errorcode.h)
    // One inner error code maps to one business error code, business error code can have multiple inner error codes.
    { FCM_ERR_CONFIG_INVALID_ADDRESS_INFO, FCM_ERR_INTERNAL_ERROR },
    { FCM_ERR_CONFIG_INVALID_ADDRESS, FCM_ERR_INTERNAL_ERROR },
    { FCM_ERR_CONFIG_INVALID_ADDRESS_TYPE, FCM_ERR_INTERNAL_ERROR },
    { FCM_ERR_CONFIG_INVALID_RAW_ADDRESS_TYPE, FCM_ERR_INTERNAL_ERROR },
    { FCM_ERR_CONFIG_INVALID_CAPABILITY, FCM_ERR_INTERNAL_ERROR },
    { FCM_ERR_CONFIG_INVALID_BUSINESS_CAPABILITY, FCM_ERR_INTERNAL_ERROR },
    { FCM_ERR_CONFIG_DEVICE_EXPIRED, FCM_ERR_INTERNAL_ERROR },
    { FCM_ERR_CONFIG_INVALID_JSON, FCM_ERR_INTERNAL_ERROR },
    { FCM_ERR_CONFIG_INVALID_TOKEN_ID, FCM_ERR_INTERNAL_ERROR },
    { FCM_ERR_EXTENSION_TYPE_NOT_SUPPORT, FCM_ERR_INTERNAL_ERROR },
    { FCM_ERR_PROXY_IS_NULL, FCM_ERR_INTERNAL_ERROR },
};

static std::map<int32_t, std::string> innerErrMsgMap {
    { FCM_ERR_CONFIG_INVALID_ADDRESS_INFO, "Operation failed. Invalid addressInfo format." },
    { FCM_ERR_CONFIG_INVALID_ADDRESS, "Operation failed. Invalid address." },
    { FCM_ERR_CONFIG_INVALID_ADDRESS_TYPE, "Operation failed. Invalid address type." },
    { FCM_ERR_CONFIG_INVALID_RAW_ADDRESS_TYPE, "Operation failed. Invalid raw address type." },
    { FCM_ERR_CONFIG_INVALID_CAPABILITY, "Operation failed. Invalid capability format." },
    { FCM_ERR_CONFIG_INVALID_BUSINESS_CAPABILITY, "Operation failed. Invalid business capability format." },
    { FCM_ERR_CONFIG_DEVICE_EXPIRED,
        "Operation failed. The device has been unpaired for more than 30 days." },
    { FCM_ERR_CONFIG_INVALID_JSON, "Operation failed. Invalid partner device config JSON format." },
    { FCM_ERR_CONFIG_INVALID_TOKEN_ID, "Operation failed. Invalid tokenId, the app is uninstalled." },
    { FCM_ERR_EXTENSION_TYPE_NOT_SUPPORT, "Operation failed. The extension is not partnerAgent type." },
    { FCM_ERR_PROXY_IS_NULL, "Operation failed. SA proxy is nullptr." },
};

bool IsInnerErrorCode(int32_t errCode)
{
    return innerToBusinessErrCodeMap.find(errCode) != innerToBusinessErrCodeMap.end();
}

void ConvertInnerToBusinessErrCode(int32_t innerCode, ErrInfo &info)
{
    info.errCode = innerCode;
    info.errMsg = "Unknown inner error.";
    // find business errCode
    auto mapIter = innerToBusinessErrCodeMap.find(innerCode);
    if (mapIter != innerToBusinessErrCodeMap.end()) {
        info.errCode = mapIter->second;
    }
    // find inner errMsg
    auto innerMsgIter = innerErrMsgMap.find(innerCode);
    if (innerMsgIter != innerErrMsgMap.end()) {
        info.errMsg = innerMsgIter->second;
    }
    HILOGI("innerCode: %{public}d -> errCode: %{public}d, msg: %{public}s",
        innerCode, info.errCode, info.errMsg.c_str());
}

ErrInfo ProcessErrCode(int32_t originalCode, const std::vector<int32_t> &validErrCodes)
{
    ErrInfo result = { originalCode, "" };
    // inner code: originalCode -> business errCode + specific errMsg
    if (IsInnerErrorCode(originalCode)) {
        ConvertInnerToBusinessErrCode(originalCode, result);
        return result;
    }
    bool isValidCode = false;
    for (const auto &code: validErrCodes) {
        if (code == result.errCode) {
            isValidCode = true;
            break;
        }
    }
    if (!isValidCode) {
        // invalid code: FCM_ERR_INTERNAL_ERROR + specific errMsg
        result.errCode = FCM_ERR_INTERNAL_ERROR;
        result.errMsg = "Operation failed";
    } else {
        auto detailIter = napiErrMsgMap.find(result.errCode);
        if (detailIter != napiErrMsgMap.end()) {
            result.errMsg = detailIter->second;
        }
    }
    return result;
}

void HandleSyncErrNum(const napi_env &env, int32_t errCode)
{
    if (errCode == FcmErrCode::FCM_NO_ERROR) {
        return;
    }

    int ret = -1;
    std::string errMsg = GetNapiErrMsg(env, errCode);
    ret = napi_throw(env, GenerateBusinessError(env, errCode, errMsg));
    FCM_CHECK_RETURN(ret == napi_ok, "napi_throw failed, ret: %{public}d", ret);
}

void HandleSyncErrWithValidCodes(const napi_env &env, int32_t errCode, const std::vector<int32_t> &validErrCodes)
{
    if (errCode == FcmErrCode::FCM_NO_ERROR) {
        return;
    }
    auto processResult = ProcessErrCode(errCode, validErrCodes);
    NapiHaEventUtils::WriteErrCode(env, processResult.errCode);
    if (!processResult.errMsg.empty()) {
        napi_throw_error(env, std::to_string(processResult.errCode).c_str(), processResult.errMsg.c_str());
    }
}

void HandleSyncErrNumWithValidCodes(const napi_env &env, int32_t errCode, const std::vector<int32_t> &validErrCodes)
{
    if (errCode == FcmErrCode::FCM_NO_ERROR) {
        return;
    }
    auto processResult = ProcessErrCode(errCode, validErrCodes);
    NapiHaEventUtils::WriteErrCode(env, processResult.errCode);
    if (!processResult.errMsg.empty()) {
        int ret = -1;
        ret = napi_throw(env, GenerateBusinessError(env, processResult.errCode, processResult.errMsg.c_str()));
        FCM_CHECK_RETURN(ret == napi_ok, "napi_throw failed, ret: %{public}d", ret);
    }
}

void HandleSyncErrAdapter(const napi_env &env, int32_t errCode, std::vector<int32_t> &validErrCodes)
{
    if (validErrCodes.empty()) {
        HandleSyncErr(env, errCode);
    } else {
        HandleSyncErrWithValidCodes(env, errCode, validErrCodes);
    }
}

void HandleSyncErrNumAdapter(const napi_env &env, int32_t errCode, std::vector<int32_t> &validErrCodes)
{
    if (validErrCodes.empty()) {
        HandleSyncErrNum(env, errCode);
    } else {
        HandleSyncErrNumWithValidCodes(env, errCode, validErrCodes);
    }
}
}  // namespace FusionConnectivity
}  // namespace OHOS