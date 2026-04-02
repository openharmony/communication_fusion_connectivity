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

#ifndef NAPI_PARSER_UTILS_H
#define NAPI_PARSER_UTILS_H

#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "napi_fusion_connectivity_error.h"

namespace OHOS {
namespace FusionConnectivity {

struct PartnerAgentDeviceCapability {
    bool supportBR;
    bool supportBleAdvertiser;
};

enum class PartnerAgentExtensionAbilityDestroyReason {
    UNKNOWN_REASON = 0,   // Unknown reason.
    USER_CLOSED_ABILITY = 1,    // The user actively disables this ability feature.
    DEVICE_UNPAIRED = 2,  // The device is unpaired.
    DEVICE_LOST = 3,  // The device is lost.
    BLUETOOTH_DISABLED = 4,  // Bluetooth is disabled.
};

enum AddressType {
    UNSET_ADDRESS = 0,
    VIRTUAL_ADDRESS,
    REAL_ADDRESS,
};

constexpr size_t ARGS_SIZE_ZERO = 0;
constexpr size_t ARGS_SIZE_ONE = 1;
constexpr size_t ARGS_SIZE_TWO = 2;
constexpr size_t ARGS_SIZE_THREE = 3;
constexpr size_t ARGS_SIZE_FOUR = 4;
constexpr int32_t PARAM0 = 0;
constexpr int32_t PARAM1 = 1;
constexpr int32_t PARAM2 = 2;
constexpr int32_t PARAM3 = 3;
constexpr int32_t PARAM4 = 4;

constexpr size_t ADDRESS_LENGTH = 17;
constexpr size_t ADDRESS_COLON_INDEX = 2;
constexpr size_t ADDRESS_SEPARATOR_UNIT = 3;

napi_status NapiIsObject(napi_env env, napi_value value);
napi_status NapiIsString(napi_env env, napi_value value);
napi_status NapiIsFunction(napi_env env, napi_value value);

napi_status NapiParseString(napi_env env, napi_value value, std::string &outStr);
napi_status NapiParseBdAddr(napi_env env, napi_value value, std::string &outAddr);

napi_value NapiGetNull(napi_env env);
napi_value NapiGetUndefined(napi_env env);
napi_value NapiGetInt32(napi_env env, int32_t res);
napi_value NapiGetBooleanRet(napi_env env, bool ret);

napi_status NapiParseInt32Object(napi_env env, napi_value object, const char *name, int32_t &outParam);
napi_status NapiParseStringObject(napi_env env, napi_value object, const char *name, std::string &outParam);

napi_status NapiParseInt32ObjectOptional(
    napi_env env, napi_value object, const char *name, bool &outExist, int32_t &outParam);
napi_status NapiParseStringObjectOptional(
    napi_env env, napi_value object, const char *name, bool &outExist, std::string &outParam);
napi_status NapiParseBoolObjectOptional(
    napi_env env, napi_value object, const char *name, bool &outExist, bool &outParam);

bool ParseString(napi_env env, std::string &param, napi_value args);
bool ParseInt32(napi_env env, int32_t &param, napi_value args);
bool IsValidAddress(const std::string &addr);

void SetNamedPropertyByInteger(napi_env env, napi_value dstObj, int32_t objName, const char *propName);

napi_status CheckEmptyParams(napi_env env, napi_callback_info info);

struct NapiAsyncCallback;
std::shared_ptr<NapiAsyncCallback> NapiParseAsyncCallback(napi_env env, napi_callback_info info);

int DoInJsMainThread(napi_env env, std::function<void(void)> func);
}  // namespace FusionConnectivity
}  // namespace OHOS
#endif  // NAPI_PARSER_UTILS_H