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
#define LOG_TAG "fc_taihe_error"
#endif

#include <map>
#include "taihe_fusion_connectivity_error.h"
#include "taihe/runtime.hpp"

namespace OHOS {
namespace FusionConnectivity {

static std::map<int32_t, std::string> taiheErrMsgMap{
    {FcmErrCode::FCM_ERR_PERMISSION_FAILED, "Permission check failed"},
    {FcmErrCode::FCM_ERR_SYSTEM_PERMISSION_FAILED, "System permission check failed"},
    {FcmErrCode::FCM_ERR_PROHIBITED_BY_EDM, "Operation is prohibited by edm"},
    {FcmErrCode::FCM_ERR_INVALID_PARAM, "Invalid parameters"},
    {FcmErrCode::FCM_ERR_API_NOT_SUPPORT, "Api is not supported"},
    {FcmErrCode::FCM_ERR_DEVICE_NOT_FOUND, "Device is not found"},
    {FcmErrCode::FCM_ERR_APPLICATION_NOT_SUPPORT, "The application is not support PartnerDeviceExtensionAbility"},
    {FcmErrCode::FCM_ERR_DEVICE_NOT_PAIRED, "The device is not paired"},
    {FcmErrCode::FCM_ERR_DEVICE_ALREADY_BOUNDED, "The device is already bound"},
    {FusionRanging::RANGING_ERR_DEVICE_NOT_INITIATED, "Device not initiated."},
    {FusionRanging::RANGING_ERR_DEVICE_ALREADY_INITIATED, "Device already initiated."},
    {FusionRanging::RANGING_ERR_RANGING_TYPE_NOT_SUPPORT, "Ranging type not support."},
    {FusionRanging::RANGING_ERR_RANGING_SERVICE_DISABLED, "Ranging service disabled"},
    {FusionRanging::RANGING_ERR_PARAM_NOT_MEET_SPECIFICATIONS, "Parameters not meet specifications."},
    {FcmErrCode::FCM_ERR_INTERNAL_ERROR, "Operation failed"},
};

std::string GetTaiheErrMsg(const int32_t errCode)
{
    auto iter = taiheErrMsgMap.find(errCode);
    if (iter != taiheErrMsgMap.end()) {
        std::string errMessage = "BusinessError ";
        errMessage.append(std::to_string(errCode)).append(": ").append(iter->second);
        return errMessage;
    }
    return "Inner error.";
}

void HandleSyncErr(int32_t errCode)
{
    if (errCode == FcmErrCode::FCM_NO_ERROR) {
        return;
    }
    std::string errMsg = GetTaiheErrMsg(errCode);
    if (errMsg != "") {
        taihe::set_business_error(errCode, errMsg.c_str());
    }
}

void HandleSyncErrWithValidCodes(int32_t errCode, const std::vector<int32_t> &validErrCodes)
{
    if (errCode == FcmErrCode::FCM_NO_ERROR) {
        return;
    }
    auto processResult = ProcessErrCode(errCode, validErrCodes);
    if (!processResult.errMsg.empty()) {
        taihe::set_business_error(processResult.errCode, processResult.errMsg.c_str());
    }
}

void HandleSyncErrAdapter(int32_t errCode, const std::vector<int32_t> &validErrCodes)
{
    if (validErrCodes.empty()) {
        HandleSyncErr(errCode);
    } else {
        HandleSyncErrWithValidCodes(errCode, validErrCodes);
    }
}

ErrInfo ProcessErrCode(int32_t originalCode, const std::vector<int32_t> &validErrCodes)
{
    ErrInfo result = {originalCode, ""};
    bool isValidCode = false;
    for (const auto &code : validErrCodes) {
        if (code == result.errCode) {
            isValidCode = true;
            break;
        }
    }
    if (!isValidCode) {
        result.errCode = FcmErrCode::FCM_ERR_INTERNAL_ERROR;
        result.errMsg = "Operation failed";
    } else {
        auto detailIter = taiheErrMsgMap.find(result.errCode);
        if (detailIter != taiheErrMsgMap.end()) {
            result.errMsg = detailIter->second;
        } else {
            result.errMsg = "Unknown error";
        }
    }
    return result;
}
}  // namespace FusionConnectivity
}  // namespace OHOS