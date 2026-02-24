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

    int ret = -1;
    std::string errMsg = GetNapiErrMsg(env, errCode);
    ret = napi_throw(env, GenerateBusinessError(env, errCode, errMsg));
    FCM_CHECK_RETURN(ret == napi_ok, "napi_throw failed, ret: %{public}d", ret);
}
}  // namespace FusionConnectivity
}  // namespace OHOS
