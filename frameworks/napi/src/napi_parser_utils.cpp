/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributd under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef LOG_TAG
#define LOG_TAG "FcmNapiParserUtils"
#endif

#include "napi_fusion_connectivity_error.h"
#include "napi_parser_utils.h"
#include "log.h"
#include "napi_async_callback.h"

namespace OHOS {
namespace FusionConnectivity {

napi_status NapiIsObject(napi_env env, napi_value value)
{
    napi_valuetype valuetype = napi_undefined;
    NAPI_FCM_CALL_RETURN(napi_typeof(env, value, &valuetype));
    NAPI_FCM_RETURN_IF(valuetype != napi_object, "Wrong argument type. Object expected.", napi_object_expected);
    return napi_ok;
}

napi_status NapiIsString(napi_env env, napi_value value)
{
    napi_valuetype valuetype = napi_undefined;
    NAPI_FCM_CALL_RETURN(napi_typeof(env, value, &valuetype));
    NAPI_FCM_RETURN_IF(valuetype != napi_string, "Wrong argument type. String expected.", napi_string_expected);
    return napi_ok;
}

napi_status NapiIsFunction(napi_env env, napi_value value)
{
    napi_valuetype valuetype = napi_undefined;
    NAPI_FCM_CALL_RETURN(napi_typeof(env, value, &valuetype));
    if (valuetype != napi_function) {
        HILOGD("Wrong argument type. Function expected.");
        return napi_function_expected;
    }
    NAPI_FCM_RETURN_IF(valuetype != napi_function, "Wrong argument type. Function expected.", napi_function_expected);
    return napi_ok;
}

napi_status NapiParseString(napi_env env, napi_value value, std::string &outStr)
{
    std::string str {};
    NAPI_FCM_CALL_RETURN(NapiIsString(env, value));
    NAPI_FCM_RETURN_IF(!ParseString(env, str, value), "parse string failed", napi_invalid_arg);
    outStr = std::move(str);
    return napi_ok;
}

napi_status NapiParseBdAddr(napi_env env, napi_value value, std::string &outAddr)
{
    std::string bdaddr {};
    NAPI_FCM_CALL_RETURN(NapiParseString(env, value, bdaddr));
    NAPI_FCM_RETURN_IF(!IsValidAddress(bdaddr), "Invalid bdaddr", napi_invalid_arg);
    outAddr = std::move(bdaddr);
    return napi_ok;
}

napi_status NapiParseInt32Object(napi_env env, napi_value object, const char *name, int32_t &outParam)
{
    bool exist = false;
    NAPI_FCM_CALL_RETURN(NapiParseInt32ObjectOptional(env, object, name, exist, outParam));
    if (!exist) {
        HILOGE("%{public}s is needed", name);
        return napi_invalid_arg;
    }
    return napi_ok;
}

napi_status NapiParseStringObject(napi_env env, napi_value object, const char *name, std::string &outParam)
{
    bool exist = false;
    NAPI_FCM_CALL_RETURN(NapiParseStringObjectOptional(env, object, name, exist, outParam));
    if (!exist) {
        HILOGE("%{public}s is needed", name);
        return napi_invalid_arg;
    }
    return napi_ok;
}

static napi_status ParseNumberParams(napi_env env, napi_value object, const char *name, bool &outExist,
    napi_value &outParam)
{
    bool hasProperty = false;
    NAPI_FCM_CALL_RETURN(napi_has_named_property(env, object, name, &hasProperty));
    if (hasProperty) {
        napi_value property;
        NAPI_FCM_CALL_RETURN(napi_get_named_property(env, object, name, &property));
        napi_valuetype valuetype;
        NAPI_FCM_CALL_RETURN(napi_typeof(env, property, &valuetype));
        NAPI_FCM_RETURN_IF(valuetype != napi_number, "Wrong argument type, number expected", napi_number_expected);
        outParam = property;
    }
    outExist = hasProperty;
    return napi_ok;
}

napi_status NapiParseInt32ObjectOptional(
    napi_env env, napi_value object, const char *name, bool &outExist, int32_t &outParam)
{
    bool exist = false;
    napi_value param;
    NAPI_FCM_CALL_RETURN(ParseNumberParams(env, object, name, exist, param));
    if (exist) {
        int32_t num = 0;
        NAPI_FCM_CALL_RETURN(napi_get_value_int32(env, param, &num));
        outParam = num;
    }
    outExist = exist;
    return napi_ok;
}

napi_status NapiParseStringObjectOptional(
    napi_env env, napi_value object, const char *name, bool &outExist, std::string &outParam)
{
    bool hasProperty = false;
    NAPI_FCM_CALL_RETURN(napi_has_named_property(env, object, name, &hasProperty));
    if (hasProperty) {
        napi_value property;
        NAPI_FCM_CALL_RETURN(napi_get_named_property(env, object, name, &property));
        napi_valuetype valuetype;
        NAPI_FCM_CALL_RETURN(napi_typeof(env, property, &valuetype));
        NAPI_FCM_RETURN_IF(valuetype != napi_string, "Wrong argument type, string expected", napi_string_expected);

        std::string param {};
        bool isSuccess = ParseString(env, param, property);
        if (!isSuccess) {
            return napi_invalid_arg;
        }
        outParam = std::move(param);
    }
    outExist = hasProperty;
    return napi_ok;
}

napi_status NapiParseBoolObjectOptional(
    napi_env env, napi_value object, const char *name, bool &outExist, bool &outParam)
{
    bool hasProperty = false;
    NAPI_FCM_CALL_RETURN(napi_has_named_property(env, object, name, &hasProperty));
    if (hasProperty) {
        napi_value property;
        NAPI_FCM_CALL_RETURN(napi_get_named_property(env, object, name, &property));
        napi_valuetype valuetype;
        NAPI_FCM_CALL_RETURN(napi_typeof(env, property, &valuetype));
        NAPI_FCM_RETURN_IF(valuetype != napi_boolean, "Wrong argument type, boolean expected", napi_boolean_expected);

        bool param = false;
        NAPI_FCM_CALL_RETURN(napi_get_value_bool(env, property, &param));
        outParam = param;
    }
    outExist = hasProperty;
    return napi_ok;
}

napi_value NapiGetNull(napi_env env)
{
    napi_value result = nullptr;
    napi_get_null(env, &result);
    return result;
}

napi_value NapiGetUndefined(napi_env env)
{
    napi_value ret = nullptr;
    napi_get_undefined(env, &ret);
    return ret;
}

napi_value NapiGetInt32(napi_env env, int32_t res)
{
    napi_value ret = nullptr;
    napi_create_int32(env, res, &ret);
    return ret;
}

napi_value NapiGetBooleanRet(napi_env env, bool ret)
{
    napi_value result = nullptr;
    napi_get_boolean(env, ret, &result);
    if (result != nullptr) {
        HILOGI("result != nullptr");
    }
    return result;
}

void SetNamedPropertyByInteger(napi_env env, napi_value dstObj, int32_t objName, const char *propName)
{
    napi_value prop = nullptr;
    if (napi_create_int32(env, objName, &prop) == napi_ok) {
        napi_set_named_property(env, dstObj, propName, prop);
    }
}

napi_status CheckEmptyParams(napi_env env, napi_callback_info info)
{
    size_t argc = ARGS_SIZE_ZERO;
    NAPI_FCM_CALL_RETURN(napi_get_cb_info(env, info, &argc, nullptr, nullptr, nullptr));
    NAPI_FCM_RETURN_IF(argc != ARGS_SIZE_ZERO, "Requires 0 argument.", napi_invalid_arg);
    return napi_ok;
}

bool ParseString(napi_env env, std::string &param, napi_value args)
{
    napi_valuetype valuetype;
    napi_typeof(env, args, &valuetype);

    if (valuetype != napi_string) {
        HILOGE("Wrong argument type(%{public}d). String expected.", valuetype);
        return false;
    }
    size_t size = 0;

    if (napi_get_value_string_utf8(env, args, nullptr, 0, &size) != napi_ok) {
        HILOGE("can not get string size.");
        param = "";
        return false;
    }
    param.reserve(size + 1);
    param.resize(size);
    if (napi_get_value_string_utf8(env, args, param.data(), (size + 1), &size) != napi_ok) {
        HILOGE("can not get string value.");
        param = "";
        return false;
    }
    return true;
}

bool IsValidAddress(const std::string &addr)
{
    if (addr.empty() || addr.length() != ADDRESS_LENGTH) {
        return false;
    }
    for (size_t i = 0; i < ADDRESS_LENGTH; i++) {
        char c = addr[i];
        switch (i % ADDRESS_SEPARATOR_UNIT) {
            case 0:
            case 1:
                if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
                    break;
                }
                return false;
            case ADDRESS_COLON_INDEX:
            default:
                if (c == ':') {
                    break;
                }
                return false;
        }
    }
    return true;
}

bool ParseInt32(napi_env env, int32_t &param, napi_value args)
{
    napi_valuetype valuetype;
    napi_typeof(env, args, &valuetype);
    if (valuetype != napi_number) {
        HILOGE("Wrong argument type(%{public}d). Int32 expected.", valuetype);
        return false;
    }
    napi_get_value_int32(env, args, &param);
    return true;
}

std::shared_ptr<NapiAsyncCallback> NapiParseAsyncCallback(napi_env env, napi_callback_info info)
{
    size_t argc = ARGS_SIZE_FOUR;
    napi_value argv[ARGS_SIZE_FOUR] = {nullptr};
    auto status = napi_get_cb_info(env, info, &argc, argv, nullptr, NULL);
    if (status != napi_ok) {
        HILOGE("napi_get_cb_info failed");
        return nullptr;
    }
    if (argc > ARGS_SIZE_FOUR) {
        HILOGE("size of parameters is larger than ARGS_SIZE_FOUR");
        return nullptr;
    }

    // "argc - 1" is AsyncCallback parameter's index
    auto asyncCallback = std::make_shared<NapiAsyncCallback>();
    asyncCallback->env = env;
    if (argc > 0 && NapiIsFunction(env, argv[argc - 1]) == napi_ok) {
        HILOGD("callback mode");
        asyncCallback->callback = std::make_shared<NapiCallback>(env, argv[argc - 1]);
    } else {
        HILOGD("promise mode");
        asyncCallback->promise = std::make_shared<NapiPromise>(env);
    }
    return asyncCallback;
}

int DoInJsMainThread(napi_env env, std::function<void(void)> func)
{
    if (napi_send_event(env, func, napi_eprio_high) != napi_ok) {
        HILOGE("Failed to SendEvent");
        return -1;
    }
    return 0;
}
}  // namespace FusionConnectivity
}  // namespace OHOS
