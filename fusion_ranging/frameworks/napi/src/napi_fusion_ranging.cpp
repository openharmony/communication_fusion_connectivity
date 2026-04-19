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
#define LOG_TAG "FusionRangingNapi"
#endif

#include "napi_fusion_ranging.h"
#include "fusion_ranging_errorcode.h"
#include "napi_fusion_ranging_constant.h"
#include "fusion_ranging_manager.h"
#include "napi_async_work.h"
#include "napi_parser_utils.h"
#include "napi_native_object.h"
#include "napi_fusion_connectivity_error.h"
#include "napi_async_callback.h"
#include "log_util.h"
#include <memory>
#include <map>

namespace OHOS {
namespace FusionRanging {

namespace {
struct RangingCallbackInfo {
    napi_env env;
    napi_ref ref;
};

std::map<std::string, RangingCallbackInfo> g_rangingCallbacks;
std::map<napi_env, napi_ref> g_stateChangeCallbacks;
std::mutex g_rangingCallbacksMutex;
std::mutex g_stateChangeCallbacksMutex;

static napi_value GenerateRangingMeasurement(napi_env env, const RangingMeasurement &measurement);
static napi_value GenerateRangingResult(napi_env env, const RangingResult &result);
static napi_value GenerateRangingStateChangeInfo(napi_env env, const RangingStateChangeInfo &info);
static napi_ref RegisterRangingCallback(napi_env env, napi_value callback, const RangingParams &params);
static std::string FindDeviceIdByEnv(const napi_env env);
static void CleanupRangingCallback(napi_env env, const std::string &deviceId, napi_ref callbackRef);

static napi_status NapiParseRangingParams(napi_env env, napi_value object, RangingParams &outParams)
{
    bool hasProperty = false;
    NAPI_FCM_CALL_RETURN(napi_has_named_property(env, object, "deviceId", &hasProperty));
    if (!hasProperty) {
        HILOGE("deviceId property is needed");
        return napi_invalid_arg;
    }

    napi_value deviceIdValue;
    NAPI_FCM_CALL_RETURN(napi_get_named_property(env, object, "deviceId", &deviceIdValue));
    std::string deviceId;
    NAPI_FCM_RETURN_IF(!FusionConnectivity::ParseString(env, deviceId, deviceIdValue), "Invalid deviceId",
                       napi_invalid_arg);
    outParams.SetDeviceId(deviceId);

    NAPI_FCM_CALL_RETURN(napi_has_named_property(env, object, "role", &hasProperty));
    if (hasProperty) {
        napi_value roleValue;
        NAPI_FCM_CALL_RETURN(napi_get_named_property(env, object, "role", &roleValue));
        int32_t role;
        NAPI_FCM_RETURN_IF(!FusionConnectivity::ParseInt32(env, role, roleValue), "Invalid role", napi_invalid_arg);
        outParams.SetRole(static_cast<RangingRole>(role));
    }

    NAPI_FCM_CALL_RETURN(napi_has_named_property(env, object, "capabilityType", &hasProperty));
    if (hasProperty) {
        napi_value capabilityTypeValue;
        NAPI_FCM_CALL_RETURN(napi_get_named_property(env, object, "capabilityType", &capabilityTypeValue));
        int32_t capabilityType;
        NAPI_FCM_RETURN_IF(!FusionConnectivity::ParseInt32(env, capabilityType, capabilityTypeValue),
                           "Invalid capabilityType", napi_invalid_arg);
        outParams.SetCapabilityType(static_cast<RangingTypes>(capabilityType));
    }

    return napi_ok;
}

static napi_ref RegisterRangingCallback(napi_env env, napi_value callback, const RangingParams &params)
{
    napi_ref callbackRef = nullptr;
    napi_status refStatus = napi_create_reference(env, callback, 1, &callbackRef);
    if (refStatus != napi_ok || callbackRef == nullptr) {
        HILOGE("StartRanging: create callback reference failed");
        return nullptr;
    }

    auto callbackRefLocal = callbackRef;
    auto paramsLocal = params;
    auto completeFunc = [env, callbackRefLocal, paramsLocal]() {
        std::lock_guard<std::mutex> lock(g_rangingCallbacksMutex);
        auto it = g_rangingCallbacks.find(paramsLocal.GetDeviceId());
        if (it != g_rangingCallbacks.end()) {
            if (it->second.env == env) {
                HILOGI("StartRanging: same env, reuse callback ref for device:%{public}s",
                       GET_ENCRYPT_ADDR(paramsLocal.GetDeviceId()));
            } else {
                HILOGI("StartRanging: cleanup old callback ref for device:%{public}s",
                       GET_ENCRYPT_ADDR(paramsLocal.GetDeviceId()));
                napi_delete_reference(env, it->second.ref);
            }
        }
        g_rangingCallbacks[paramsLocal.GetDeviceId()] = {env, callbackRefLocal};
        HILOGI("StartRanging: registered callback for device:%{public}s", GET_ENCRYPT_ADDR(paramsLocal.GetDeviceId()));
    };

    FusionConnectivity::DoInJsMainThread(env, completeFunc);
    return callbackRef;
}

static std::string FindDeviceIdByEnv(const napi_env env)
{
    std::string deviceId;
    std::lock_guard<std::mutex> lock(g_rangingCallbacksMutex);
    for (auto it = g_rangingCallbacks.begin(); it != g_rangingCallbacks.end(); ++it) {
        if (it->second.env == env) {
            deviceId = it->first;
            break;
        }
    }
    HILOGI("FindDeviceIdByEnv: found deviceId=%{public}s for env", GET_ENCRYPT_ADDR(deviceId));
    return deviceId;
}

static void CleanupRangingCallback(napi_env env, const std::string &deviceId, napi_ref callbackRef)
{
    std::lock_guard<std::mutex> lock(g_rangingCallbacksMutex);
    if (deviceId.empty()) {
        for (auto it = g_rangingCallbacks.begin(); it != g_rangingCallbacks.end();) {
            if (it->second.env == env) {
                HILOGI("CleanupRangingCallback: cleanup all callback ref for env");
                napi_delete_reference(env, it->second.ref);
                it = g_rangingCallbacks.erase(it);
            } else {
                ++it;
            }
        }
    } else {
        auto it = g_rangingCallbacks.find(deviceId);
        if (it != g_rangingCallbacks.end() && it->second.env == env) {
            HILOGI("CleanupRangingCallback: cleanup callback ref for device:%{public}s", GET_ENCRYPT_ADDR(deviceId));
            napi_delete_reference(env, it->second.ref);
            g_rangingCallbacks.erase(it);
        }
    }
}

static napi_value GenerateRangingMeasurement(napi_env env, const RangingMeasurement &measurement)
{
    napi_value obj = nullptr;
    napi_status status = napi_create_object(env, &obj);
    if (status != napi_ok || obj == nullptr) {
        return obj;
    }

    napi_value isValid = nullptr;
    status = napi_get_boolean(env, measurement.GetIsValid(), &isValid);
    if (status == napi_ok && isValid != nullptr) {
        napi_set_named_property(env, obj, "isValid", isValid);
    }

    napi_value value = nullptr;
    status = napi_create_int32(env, measurement.GetValue(), &value);
    if (status == napi_ok && value != nullptr) {
        napi_set_named_property(env, obj, "value", value);
    }

    napi_value confidence = nullptr;
    status = napi_create_int32(env, static_cast<int32_t>(measurement.GetConfidence()), &confidence);
    if (status == napi_ok && confidence != nullptr) {
        napi_set_named_property(env, obj, "confidence", confidence);
    }

    return obj;
}

static napi_value GenerateRangingResult(napi_env env, const RangingResult &result)
{
    napi_value obj = nullptr;
    napi_status status = napi_create_object(env, &obj);
    if (status != napi_ok || obj == nullptr) {
        return obj;
    }

    napi_value deviceId = nullptr;
    status = napi_create_string_utf8(env, result.GetDeviceId().c_str(), NAPI_AUTO_LENGTH, &deviceId);
    if (status == napi_ok && deviceId != nullptr) {
        napi_set_named_property(env, obj, "deviceId", deviceId);
    }

    napi_value distance = GenerateRangingMeasurement(env, result.GetDistance());
    if (distance != nullptr) {
        napi_set_named_property(env, obj, "distance", distance);
    }

    napi_value angle = GenerateRangingMeasurement(env, result.GetAngle());
    if (angle != nullptr) {
        napi_set_named_property(env, obj, "angle", angle);
    }

    napi_value rssi = nullptr;
    status = napi_create_int32(env, result.GetRssi(), &rssi);
    if (status == napi_ok && rssi != nullptr) {
        napi_set_named_property(env, obj, "rssi", rssi);
    }

    return obj;
}

static napi_value GenerateRangingStateChangeInfo(napi_env env, const RangingStateChangeInfo &info)
{
    napi_value obj = nullptr;
    napi_status status = napi_create_object(env, &obj);
    if (status != napi_ok || obj == nullptr) {
        return obj;
    }

    napi_value state = nullptr;
    status = napi_create_int32(env, static_cast<int32_t>(info.GetState()), &state);
    if (status == napi_ok && state != nullptr) {
        napi_set_named_property(env, obj, "state", state);
    }

    napi_value cause = nullptr;
    status = napi_create_int32(env, static_cast<int32_t>(info.GetCause()), &cause);
    if (status == napi_ok && cause != nullptr) {
        napi_set_named_property(env, obj, "cause", cause);
    }

    return obj;
}

std::function<void(const RangingResult &)> g_rangingResultCallback;
bool g_rangingCallbackInited = false;

static std::function<void(const RangingResult &)> GetRangingResultCallback()
{
    if (!g_rangingCallbackInited) {
        g_rangingCallbackInited = true;
        g_rangingResultCallback = [](const RangingResult &result) {
            std::string deviceId = result.GetDeviceId();
            RangingCallbackInfo cbInfo;
            {
                std::lock_guard<std::mutex> lock(g_rangingCallbacksMutex);
                auto it = g_rangingCallbacks.find(deviceId);
                if (it == g_rangingCallbacks.end()) {
                    HILOGE("rangingCallback not found for device:%{public}s", GET_ENCRYPT_ADDR(deviceId));
                    return;
                }
                cbInfo = it->second;
            }

            auto resultPtr = std::make_shared<RangingResult>(result);
            auto jsCallback = [resultPtr, cbInfo]() {
                napi_handle_scope scope = nullptr;
                napi_open_handle_scope(cbInfo.env, &scope);
                if (scope == nullptr) {
                    return;
                }

                napi_value callbackFunc = nullptr;
                napi_status refStatus = napi_get_reference_value(cbInfo.env, cbInfo.ref, &callbackFunc);
                if (refStatus != napi_ok || callbackFunc == nullptr) {
                    napi_close_handle_scope(cbInfo.env, scope);
                    return;
                }

                napi_value resultObj = GenerateRangingResult(cbInfo.env, *resultPtr);
                napi_value callResult = nullptr;
                napi_call_function(cbInfo.env, FusionConnectivity::NapiGetUndefined(cbInfo.env), callbackFunc, 1,
                                   &resultObj, &callResult);

                napi_close_handle_scope(cbInfo.env, scope);
            };

            FusionConnectivity::DoInJsMainThread(cbInfo.env, jsCallback);
        };
    }
    return g_rangingResultCallback;
}
}  // anonymous namespace

void DefineRangingInterface(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("isRangingSupported", IsRangingSupported),
        DECLARE_NAPI_FUNCTION("getRangingCapability", GetRangingCapability),
        DECLARE_NAPI_FUNCTION("startRanging", StartRanging),
        DECLARE_NAPI_FUNCTION("stopRanging", StopRanging),
        DECLARE_NAPI_FUNCTION("onRangingStateChange", OnRangingStateChange),
        DECLARE_NAPI_FUNCTION("offRangingStateChange", OffRangingStateChange),
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
}

static napi_value GenerateRangingCapabilitySupported(napi_env env, const RangingCapabilitySupported &cap)
{
    napi_value obj = nullptr;
    napi_status status = napi_create_object(env, &obj);
    if (status != napi_ok || obj == nullptr) {
        return obj;
    }

    napi_value nearlinkHadm = nullptr;
    status = napi_get_boolean(env, cap.GetNearlinkHadm(), &nearlinkHadm);
    if (status == napi_ok && nearlinkHadm != nullptr) {
        napi_set_named_property(env, obj, "nearlinkHadm", nearlinkHadm);
    }

    return obj;
}

napi_value IsRangingSupported(napi_env env, napi_callback_info info)
{
    napi_get_cb_info(env, info, nullptr, nullptr, nullptr, nullptr);
    HILOGI("IsRangingSupported napi enter!");
    auto manager = FusionRangingManager::GetInstance();
    bool isSupported = manager ? manager->IsRangingSupported() : false;
    napi_value result = nullptr;
    napi_status status = napi_get_boolean(env, isSupported, &result);
    if (status != napi_ok || result == nullptr) {
        return FusionConnectivity::NapiGetUndefined(env);
    }
    return result;
}

class NapiNativeRangingCapabilityData : public FusionConnectivity::NapiNativeObject {
public:
    explicit NapiNativeRangingCapabilityData(RangingCapabilitySupported capData) : capData_(capData) {}
    ~NapiNativeRangingCapabilityData() override = default;

    napi_value ToNapiValue(napi_env env) const override
    {
        return GenerateRangingCapabilitySupported(env, capData_);
    }

    RangingCapabilitySupported GetCapabilityData() const
    {
        return capData_;
    }

private:
    RangingCapabilitySupported capData_;
};

napi_value GetRangingCapability(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    auto asyncCallback = FusionConnectivity::NapiParseAsyncCallback(env, info);
    if (!asyncCallback) {
        HILOGE("asyncCallback is nullptr!");
        return FusionConnectivity::NapiGetUndefined(env);
    }

    auto asyncWorkFunc = []() -> FusionConnectivity::NapiAsyncWorkRet {
        RangingCapabilitySupported cap;
        auto manager = FusionRangingManager::GetInstance();
        if (!manager) {
            HILOGE("manager is nullptr");
            auto emptyObj = std::make_shared<FusionConnectivity::NapiNativeEmpty>();
            return {FusionConnectivity::FCM_ERR_INTERNAL_ERROR, emptyObj};
        }

        int ret = manager->GetRangingCapability(cap);
        HILOGI("GetRangingCapability ret: %{public}d", ret);
        if (ret != FusionConnectivity::FCM_NO_ERROR) {
            HILOGE("GetRangingCapability failed, ret: %{public}d", ret);
            auto emptyObj = std::make_shared<FusionConnectivity::NapiNativeEmpty>();
            return {ret, emptyObj};
        }
        auto nativeObj = std::make_shared<NapiNativeRangingCapabilityData>(cap);
        return {FusionConnectivity::FCM_NO_ERROR, nativeObj};
    };

    auto asyncWork = FusionConnectivity::NapiAsyncWorkFactory::CreateAsyncWork(
        env, info, asyncWorkFunc, FusionConnectivity::ASYNC_WORK_NO_NEED_CALLBACK);
    if (!asyncWork) {
        HILOGE("asyncWork is nullptr!");
        return FusionConnectivity::NapiGetUndefined(env);
    }

    asyncWork->Run();
    return asyncWork->GetRet();
}

static napi_status ParseStartRangingParams(napi_env env, napi_value *argv, RangingParams &params)
{
    napi_valuetype valuetype;
    napi_typeof(env, argv[FusionConnectivity::PARAM0], &valuetype);
    if (valuetype != napi_object) {
        HILOGE("StartRanging: params is not an object");
        return napi_invalid_arg;
    }

    napi_status status = NapiParseRangingParams(env, argv[FusionConnectivity::PARAM0], params);
    if (status != napi_ok) {
        HILOGE("StartRanging: parse params failed");
        return status;
    }

    napi_typeof(env, argv[FusionConnectivity::PARAM1], &valuetype);
    if (valuetype != napi_function) {
        HILOGE("StartRanging: callback is not a function");
        return napi_invalid_arg;
    }

    return napi_ok;
}

static napi_status ValidateStartRangingParams(napi_env env, size_t argc, napi_value *argv)
{
    if (argc < FusionConnectivity::ARGS_SIZE_TWO) {
        HILOGE("StartRanging: wrong argument count");
        return napi_invalid_arg;
    }

    if (argv[FusionConnectivity::PARAM0] == nullptr || argv[FusionConnectivity::PARAM1] == nullptr) {
        HILOGE("StartRanging: argv[%{public}d] or argv[%{public}d] is nullptr", FusionConnectivity::PARAM0,
               FusionConnectivity::PARAM1);
        return napi_invalid_arg;
    }

    napi_valuetype valuetype;
    napi_typeof(env, argv[FusionConnectivity::PARAM0], &valuetype);
    if (valuetype != napi_object) {
        HILOGE("StartRanging: param is not an object");
        return napi_invalid_arg;
    }

    napi_typeof(env, argv[FusionConnectivity::PARAM1], &valuetype);
    if (valuetype != napi_function) {
        HILOGE("StartRanging: callback is not a function");
        return napi_invalid_arg;
    }

    return napi_ok;
}

static FusionConnectivity::NapiAsyncWorkRet ExecuteStartRangingWork(
    const RangingParams &params, const std::function<void(const RangingResult &)> &rangingCallback)
{
    auto manager = FusionRangingManager::GetInstance();
    if (!manager) {
        HILOGE("StartRanging: manager is nullptr");
        return {static_cast<int32_t>(RangingErrCode::RANGING_ERR_INTERNAL_ERROR), nullptr};
    }

    int ret = manager->StartRanging(params, rangingCallback);
    HILOGI("StartRanging: napi ret:%{public}d", ret);
    if (ret != 0) {
        HILOGE("StartRanging: manager StartRanging failed, ret:%{public}d", ret);
        return {ret, nullptr};
    }

    HILOGI("StartRanging: manager StartRanging success");
    return {FusionConnectivity::FCM_NO_ERROR, nullptr};
}

napi_value StartRanging(napi_env env, napi_callback_info info)
{
    HILOGI("StartRanging napi enter.");
    size_t argc = FusionConnectivity::ARGS_SIZE_TWO;
    napi_value argv[FusionConnectivity::ARGS_SIZE_TWO] = {nullptr};
    napi_status cbInfoStatus = napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    HILOGI("StartRanging: cbInfoStatus=%{public}d argc=%{public}zu", cbInfoStatus, argc);
    napi_status cbStatus = ValidateStartRangingParams(env, argc, argv);
    if (cbStatus != napi_ok) {
        return FusionConnectivity::NapiGetUndefined(env);
    }

    RangingParams params;
    napi_status parseStatus = ParseStartRangingParams(env, argv, params);
    if (parseStatus != napi_ok) {
        return FusionConnectivity::NapiGetUndefined(env);
    }

    auto rangingCallback = GetRangingResultCallback();
    auto asyncWorkFunc = [&params, rangingCallback]() -> FusionConnectivity::NapiAsyncWorkRet {
        return ExecuteStartRangingWork(params, rangingCallback);
    };
    auto asyncCallback = std::make_shared<FusionConnectivity::NapiAsyncCallback>();
    asyncCallback->env = env;
    asyncCallback->callback = std::make_shared<FusionConnectivity::NapiCallback>(env, argv[FusionConnectivity::PARAM1]);

    auto asyncWork = std::make_shared<FusionConnectivity::NapiAsyncWork>(env, asyncWorkFunc, asyncCallback, false);
    asyncWork->Run();

    if (RegisterRangingCallback(env, argv[FusionConnectivity::PARAM1], params) == nullptr) {
        HILOGE("StartRanging: register callback failed");
    }

    return asyncWork->GetRet();
}

static napi_status ValidateStopRangingParams(napi_env env, size_t argc, napi_value *argv)
{
    if (argc < FusionConnectivity::ARGS_SIZE_ONE) {
        HILOGE("StopRanging: wrong argument count");
        return napi_invalid_arg;
    }

    if (argv[FusionConnectivity::PARAM0] == nullptr) {
        HILOGE("StopRanging: argv[0] is nullptr");
        return napi_invalid_arg;
    }

    napi_valuetype valuetype;
    napi_typeof(env, argv[FusionConnectivity::PARAM0], &valuetype);
    if (valuetype != napi_function) {
        HILOGE("StopRanging: callback should be a function");
        return napi_invalid_arg;
    }

    return napi_ok;
}

static auto ExecuteStopRangingWork(const std::string &deviceId)
{
    return [deviceId]() -> FusionConnectivity::NapiAsyncWorkRet {
        auto manager = FusionRangingManager::GetInstance();
        if (!manager) {
            return {static_cast<int32_t>(RangingErrCode::RANGING_ERR_INTERNAL_ERROR), nullptr};
        }

        int ret = manager->StopRanging(deviceId);
        HILOGI("StopRanging: manager StopRanging ret=%{public}d", ret);
        return {ret, nullptr};
    };
}

napi_value StopRanging(napi_env env, napi_callback_info info)
{
    size_t argc = FusionConnectivity::ARGS_SIZE_ONE;
    napi_value argv[FusionConnectivity::ARGS_SIZE_ONE] = {nullptr};
    napi_status cbInfoStatus = napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    HILOGI("StopRanging: cbInfoStatus=%{public}d argc=%{public}zu", cbInfoStatus, argc);
    if (cbInfoStatus != napi_ok) {
        HILOGE("StopRanging: napi_get_cb_info failed status=%{public}d", cbInfoStatus);
        return FusionConnectivity::NapiGetUndefined(env);
    }

    napi_status cbStatus = ValidateStopRangingParams(env, argc, argv);
    if (cbStatus != napi_ok) {
        return FusionConnectivity::NapiGetUndefined(env);
    }

    std::string deviceId = FindDeviceIdByEnv(env);
    napi_ref callbackRef = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_rangingCallbacksMutex);
        auto it = g_rangingCallbacks.find(deviceId);
        if (it != g_rangingCallbacks.end() && it->second.env == env) {
            callbackRef = it->second.ref;
            HILOGI("StopRanging: get callback ref for device:%{public}s", GET_ENCRYPT_ADDR(deviceId));
        }
    }

    auto asyncWorkFunc = ExecuteStopRangingWork(deviceId);
    auto completeFunc = [env, deviceId, callbackRef]() {
        CleanupRangingCallback(env, deviceId, callbackRef);
    };
    FusionConnectivity::DoInJsMainThread(env, completeFunc);

    auto asyncCallback = std::make_shared<FusionConnectivity::NapiAsyncCallback>();
    auto asyncWork = std::make_shared<FusionConnectivity::NapiAsyncWork>(env, asyncWorkFunc, asyncCallback, false);
    asyncWork->Run();

    return asyncWork->GetRet();
}

static std::function<void(const RangingStateChangeInfo &)> CreateStateChangeCallback(napi_env env)
{
    auto callback = [env](const RangingStateChangeInfo &info) {
        RangingCallbackInfo cbInfo;
        {
            std::lock_guard<std::mutex> lock(g_stateChangeCallbacksMutex);
            auto it = g_stateChangeCallbacks.find(env);
            if (it == g_stateChangeCallbacks.end()) {
                HILOGE("stateChangeCallback not found");
                return;
            }
            cbInfo = {it->first, it->second};
        }

        auto infoPtr = std::make_shared<RangingStateChangeInfo>(info);
        auto jsCallback = [infoPtr, cbInfo]() {
            napi_handle_scope scope = nullptr;
            napi_open_handle_scope(cbInfo.env, &scope);
            if (scope == nullptr) {
                return;
            }

            napi_value callbackFunc = nullptr;
            napi_get_reference_value(cbInfo.env, cbInfo.ref, &callbackFunc);
            if (callbackFunc == nullptr) {
                napi_close_handle_scope(cbInfo.env, scope);
                return;
            }

            napi_value result = GenerateRangingStateChangeInfo(cbInfo.env, *infoPtr);
            napi_call_function(cbInfo.env, FusionConnectivity::NapiGetUndefined(cbInfo.env), callbackFunc, 1, &result,
                               nullptr);

            napi_close_handle_scope(cbInfo.env, scope);
        };

        FusionConnectivity::DoInJsMainThread(env, jsCallback);
    };
    return callback;
}

static napi_status ValidateOnRangingStateChangeParams(napi_env env, size_t argc, napi_value *argv)
{
    if (argc < FusionConnectivity::ARGS_SIZE_ONE) {
        HILOGE("OnRangingStateChange: wrong argument count");
        return napi_invalid_arg;
    }

    napi_valuetype valuetype;
    napi_typeof(env, argv[FusionConnectivity::PARAM0], &valuetype);
    if (valuetype != napi_function) {
        HILOGE("OnRangingStateChange: callback is not a function");
        return napi_invalid_arg;
    }

    return napi_ok;
}

static napi_status RegisterStateChangeCallback(napi_env env, napi_value callback, napi_ref callbackRef)
{
    std::lock_guard<std::mutex> lock(g_stateChangeCallbacksMutex);
    auto it = g_stateChangeCallbacks.find(env);
    if (it != g_stateChangeCallbacks.end()) {
        HILOGI("OnRangingStateChange: cleanup old callback ref");
        napi_delete_reference(env, it->second);
    }
    g_stateChangeCallbacks[env] = callbackRef;
    return napi_ok;
}

napi_value OnRangingStateChange(napi_env env, napi_callback_info info)
{
    size_t argc = 0;
    napi_value argv[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

    napi_status cbStatus = ValidateOnRangingStateChangeParams(env, argc, argv);
    if (cbStatus != napi_ok) {
        return FusionConnectivity::NapiGetUndefined(env);
    }

    napi_ref callbackRef = nullptr;
    napi_status refStatus = napi_create_reference(env, argv[FusionConnectivity::PARAM0], 1, &callbackRef);
    if (refStatus != napi_ok || callbackRef == nullptr) {
        HILOGE("OnRangingStateChange: create callback reference failed");
        return FusionConnectivity::NapiGetUndefined(env);
    }

    auto callback = CreateStateChangeCallback(env);
    auto manager = FusionRangingManager::GetInstance();
    if (!manager) {
        napi_delete_reference(env, callbackRef);
        HILOGE("OnRangingStateChange: manager is nullptr");
        return FusionConnectivity::NapiGetUndefined(env);
    }

    int ret = manager->OnRangingStateChange(callback);
    if (ret != 0) {
        napi_delete_reference(env, callbackRef);
        HILOGE("OnRangingStateChange failed, ret:%{public}d", ret);
        return FusionConnectivity::NapiGetUndefined(env);
    }

    RegisterStateChangeCallback(env, argv[FusionConnectivity::PARAM0], callbackRef);
    HILOGI("OnRangingStateChange success");
    return FusionConnectivity::NapiGetUndefined(env);
}

napi_value OffRangingStateChange(napi_env env, napi_callback_info info)
{
    auto manager = FusionRangingManager::GetInstance();
    if (manager) {
        manager->OffRangingStateChange();
    }

    {
        std::lock_guard<std::mutex> lock(g_stateChangeCallbacksMutex);
        auto it = g_stateChangeCallbacks.find(env);
        if (it != g_stateChangeCallbacks.end()) {
            HILOGI("OffRangingStateChange: cleanup callback ref");
            napi_delete_reference(env, it->second);
            g_stateChangeCallbacks.erase(it);
        }
    }

    return FusionConnectivity::NapiGetUndefined(env);
}

EXTERN_C_START
/*
 * Module initialization function
 */
static napi_value Init(napi_env env, napi_value exports)
{
    HILOGI("napi fusion_ranging init start");
    napi_property_descriptor desc[] = {};
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);

    FusionRanging::NapiFusionRangingConstant::DefineJSConstant(env, exports);

    DefineRangingInterface(env, exports);

    HILOGI("napi fusion_ranging init end");
    return exports;
}
EXTERN_C_END

/*
 * Module define
 */
static napi_module g_fusionRangingModule = {.nm_version = 1,
                                            .nm_flags = 0,
                                            .nm_filename = nullptr,
                                            .nm_register_func = Init,
                                            .nm_modname = "ranging",
                                            .nm_priv = ((void *)0),
                                            .reserved = {0}};
/*
 * Module register function
 */
extern "C" __attribute__((constructor)) void RegisterModule(void)
{
    HILOGI("Register fusionRanging nm_modname:%{public}s", g_fusionRangingModule.nm_modname);
    napi_module_register(&g_fusionRangingModule);
}
}  // namespace FusionRanging
}  // namespace OHOS