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
#include "log_utils.h"
#include <memory>
#include <map>

namespace OHOS {
namespace FusionRanging {

namespace {
std::map<std::string, std::shared_ptr<FusionConnectivity::NapiCallback>> g_rangingCallbacks;
std::map<napi_env, std::shared_ptr<FusionConnectivity::NapiCallback>> g_stateChangeCallbacks;
std::mutex g_rangingCallbacksMutex;
std::mutex g_stateChangeCallbacksMutex;

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
    std::string deviceId = "";
    NAPI_FCM_RETURN_IF(!FusionConnectivity::ParseString(env, deviceId, deviceIdValue), "Invalid deviceId",
                       napi_invalid_arg);
    NAPI_FCM_RETURN_IF(!FusionConnectivity::IsValidAddress(deviceId), "Invalid deviceId", napi_invalid_arg);
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

static std::string FindDeviceIdByEnvCallback(const napi_env env, napi_value callback)
{
    std::string deviceId = "";
    std::lock_guard<std::mutex> lock(g_rangingCallbacksMutex);
    for (auto it = g_rangingCallbacks.begin(); it != g_rangingCallbacks.end(); ++it) {
        if (it->second != nullptr && it->second->Equal(env, callback)) {
            deviceId = it->first;
            break;
        }
    }
    HILOGI("FindDeviceIdByEnvCallback: found deviceId=%{public}s for env", GET_ENCRYPT_ADDR(deviceId));
    return deviceId;
}

static napi_value GenerateRangingMeasurement(napi_env env, const RangingMeasurement &measurement)
{
    napi_value obj = nullptr;
    napi_status status = napi_create_object(env, &obj);
    NAPI_FCM_RETURN_IF((status != napi_ok || obj == nullptr), "Generate Measurement Invalid status or obj", obj);
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
    NAPI_FCM_RETURN_IF((status != napi_ok || obj == nullptr), "Generate result Invalid status or obj", obj);
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
    NAPI_FCM_RETURN_IF((status != napi_ok || obj == nullptr), "Generate statechange Invalid status or obj", obj);
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

static napi_value GenerateRangingCapabilitySupported(napi_env env, const RangingCapabilitySupported &cap)
{
    napi_value obj = nullptr;
    napi_status status = napi_create_object(env, &obj);
    NAPI_FCM_RETURN_IF(((status != napi_ok) || (obj == nullptr)), "Generate Capability Invalid status or obj", obj);
    napi_value nearlinkHadm = nullptr;
    status = napi_get_boolean(env, cap.GetNearlinkHadm(), &nearlinkHadm);
    if (status == napi_ok && nearlinkHadm != nullptr) {
        napi_set_named_property(env, obj, "nearlinkHadm", nearlinkHadm);
    }
    return obj;
}
}  // anonymous namespace

class NapiNativeRangingCapabilityData : public FusionConnectivity::NapiNativeObject {
public:
    explicit NapiNativeRangingCapabilityData(RangingCapabilitySupported capData) : capData_(capData) {}
    ~NapiNativeRangingCapabilityData() override = default;

    napi_value ToNapiValue(napi_env env) const override
    {
        return GenerateRangingCapabilitySupported(env, capData_);
    }

private:
    RangingCapabilitySupported capData_;
};

class NapiNativeRangingResultData : public FusionConnectivity::NapiNativeObject {
public:
    explicit NapiNativeRangingResultData(const RangingResult &resultData) : resultData_(resultData) {}
    ~NapiNativeRangingResultData() override = default;

    napi_value ToNapiValue(napi_env env) const override
    {
        return GenerateRangingResult(env, resultData_);
    }

private:
    RangingResult resultData_;
};

class NapiNativeStateChnageData : public FusionConnectivity::NapiNativeObject {
public:
    explicit NapiNativeStateChnageData(const RangingStateChangeInfo &state) : stateInfo_(state) {}
    ~NapiNativeStateChnageData() override = default;

    napi_value ToNapiValue(napi_env env) const override
    {
        return GenerateRangingStateChangeInfo(env, stateInfo_);
    }

private:
    RangingStateChangeInfo stateInfo_;
};

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

napi_value IsRangingSupported(napi_env env, napi_callback_info info)
{
    NAPI_FCM_ASSERT_RETURN_UNDEF(env, FusionConnectivity::CheckEmptyParams(env, info) == napi_ok,
                                 FusionConnectivity::FCM_ERR_INVALID_PARAM);
    auto manager = FusionRangingManager::GetInstance();
    bool isSupported = (manager != nullptr) ? manager->IsRangingSupported() : false;
    HILOGI("IsRangingSupported result is %{public}d", isSupported);
    return FusionConnectivity::NapiGetBooleanRet(env, isSupported);
}

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
        int ret = FusionRangingManager::GetInstance()->GetRangingCapability(cap);
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
    NAPI_FCM_RETURN_IF((valuetype != napi_object), "params not object", napi_invalid_arg);
    napi_status status = NapiParseRangingParams(env, argv[FusionConnectivity::PARAM0], params);
    NAPI_FCM_RETURN_IF((status != napi_ok), "parse params not ok", status);
    napi_typeof(env, argv[FusionConnectivity::PARAM1], &valuetype);
    NAPI_FCM_RETURN_IF((valuetype != napi_function), "callback not function", napi_invalid_arg);
    return napi_ok;
}

static napi_status ValidateStartRangingParams(napi_env env, size_t argc, napi_value *argv)
{
    NAPI_FCM_RETURN_IF((argc < FusionConnectivity::ARGS_SIZE_TWO), "wrong argument count", napi_invalid_arg);
    if (argv[FusionConnectivity::PARAM0] == nullptr || argv[FusionConnectivity::PARAM1] == nullptr) {
        HILOGE("argv is nullptr");
        return napi_invalid_arg;
    }

    napi_valuetype valuetype;
    napi_typeof(env, argv[FusionConnectivity::PARAM0], &valuetype);
    NAPI_FCM_RETURN_IF((valuetype != napi_object), "param not object", napi_invalid_arg);
    napi_typeof(env, argv[FusionConnectivity::PARAM1], &valuetype);
    NAPI_FCM_RETURN_IF((valuetype != napi_function), "callback not function", napi_invalid_arg);
    return napi_ok;
}

static FusionConnectivity::NapiAsyncWorkRet ExecuteStartRangingWork(
    const RangingParams &params, const std::function<void(const RangingResult &)> &rangingCallback)
{
    int ret = FusionRangingManager::GetInstance()->StartRanging(params, rangingCallback);
    if (ret != 0) {
        HILOGE("manager StartRanging failed, ret:%{public}d", ret);
        return {ret, nullptr};
    }
    HILOGI("StartRanging: manager StartRanging success");
    return {FusionConnectivity::FCM_NO_ERROR, nullptr};
}

static FusionConnectivity::NapiAsyncWorkRet ExecuteRegisterStateChangeWork(
    const std::function<void(const RangingStateChangeInfo &)> &rangingCallback)
{
    int ret = FusionRangingManager::GetInstance()->OnRangingStateChange(rangingCallback);
    if (ret != 0) {
        HILOGE("manager StartRanging failed, ret:%{public}d", ret);
        return {ret, nullptr};
    }
    HILOGI("StartRanging: manager StartRanging success");
    return {FusionConnectivity::FCM_NO_ERROR, nullptr};
}

bool IsNapiCallbackExist(napi_env env, napi_value object)
{
    std::lock_guard<std::mutex> lock(g_rangingCallbacksMutex);
    for (auto it = g_rangingCallbacks.begin(); it != g_rangingCallbacks.end(); ++it) {
        if (it->second != nullptr && it->second->Equal(env, object)) {
            return true;
        }
    }
    return false;
}

napi_value StartRanging(napi_env env, napi_callback_info info)
{
    HILOGI("StartRanging napi enter.");
    size_t argc = FusionConnectivity::ARGS_SIZE_TWO;
    napi_value argv[FusionConnectivity::ARGS_SIZE_TWO] = {nullptr};
    napi_status cbInfoStatus = napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    napi_status cbStatus = ValidateStartRangingParams(env, argc, argv);
    if (cbStatus != napi_ok) {
        HILOGE("StartRanging: cbInfoStatus=%{public}d argc=%{public}zu", cbInfoStatus, argc);
        return FusionConnectivity::NapiGetUndefined(env);
    }

    RangingParams params;
    napi_status parseStatus = ParseStartRangingParams(env, argv, params);
    if (parseStatus != napi_ok) {
        return FusionConnectivity::NapiGetUndefined(env);
    }

    if (IsNapiCallbackExist(env, argv[FusionConnectivity::PARAM1])) {
        return FusionConnectivity::NapiGetInt32(env, FusionConnectivity::FCM_ERR_DEVICE_ALREADY_BOUNDED);
    }
    auto napiCallback = std::make_shared<FusionConnectivity::NapiCallback>(env, argv[FusionConnectivity::PARAM1]);
    auto resultCallback = [env, napiCallback](const RangingResult &result) {
        HILOGI("Napi ResultCallback");
        FusionConnectivity::NapiHandleScope scope(env);
        auto ret = std::make_shared<NapiNativeRangingResultData>(result);
        auto callback = [ret, napiCallback]() {
            napiCallback->CallFunction(ret);
        };
        FusionConnectivity::DoInJsMainThread(env, callback);
    };
    auto asynWorkFunc = [&params, resultCallback]() {
        return ExecuteStartRangingWork(params, resultCallback);
    };
    auto asyncWork = FusionConnectivity::NapiAsyncWorkFactory::CreateAsyncWork(
        env, info, asynWorkFunc, FusionConnectivity::ASYNC_WORK_NO_NEED_CALLBACK);
    asyncWork->Run();
    NAPI_FCM_ASSERT_RETURN_UNDEF(env, asyncWork, FusionConnectivity::FCM_ERR_INTERNAL_ERROR);
    {
        std::lock_guard<std::mutex> lock(g_rangingCallbacksMutex);
        g_rangingCallbacks[params.GetDeviceId()] = napiCallback;
    }
    return asyncWork->GetRet();
}

static napi_status CheckStopRangingParams(napi_env env, size_t argc, napi_value *argv)
{
    NAPI_FCM_RETURN_IF((argc < FusionConnectivity::ARGS_SIZE_ONE), "wrong argument count", napi_invalid_arg);
    NAPI_FCM_RETURN_IF((argv[FusionConnectivity::PARAM0] == nullptr), "argv nullptr", napi_invalid_arg);
    napi_valuetype valuetype;
    napi_typeof(env, argv[FusionConnectivity::PARAM0], &valuetype);
    NAPI_FCM_RETURN_IF((valuetype != napi_function), "callback not function", napi_invalid_arg);
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

static void CleanupRangingCallback(const std::string &deviceId)
{
    std::lock_guard<std::mutex> lock(g_rangingCallbacksMutex);
    auto it = g_rangingCallbacks.find(deviceId);
    if (it != g_rangingCallbacks.end()) {
        HILOGI("CleanupRangingCallback: cleanup callback ref for device:%{public}s", GET_ENCRYPT_ADDR(deviceId));
        g_rangingCallbacks.erase(it);
    }
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

    HILOGE("StopRanging: callback=%{public}p", argv[FusionConnectivity::PARAM0]);
    napi_status ret = CheckStopRangingParams(env, argc, argv);
    if (ret != napi_ok) {
        return FusionConnectivity::NapiGetUndefined(env);
    }
    HILOGE("StopRanging: callback=%{public}p", argv[FusionConnectivity::PARAM0]);
    std::string deviceId = FindDeviceIdByEnvCallback(env, argv[FusionConnectivity::PARAM0]);
    if (deviceId.empty()) {
        return FusionConnectivity::NapiGetUndefined(env);
    }
    auto asyncWorkFunc = ExecuteStopRangingWork(deviceId);
    auto completeFunc = [deviceId]() {
        CleanupRangingCallback(deviceId);
    };
    FusionConnectivity::DoInJsMainThread(env, completeFunc);
    auto asyncWork = FusionConnectivity::NapiAsyncWorkFactory::CreateAsyncWork(
        env, info, asyncWorkFunc, FusionConnectivity::ASYNC_WORK_NO_NEED_CALLBACK);
    asyncWork->Run();
    NAPI_FCM_ASSERT_RETURN_UNDEF(env, asyncWork, FusionConnectivity::FCM_ERR_INTERNAL_ERROR);
    return asyncWork->GetRet();
}

static napi_status ValidateOnRangingStateChangeParams(napi_env env, size_t argc, napi_value *argv)
{
    NAPI_FCM_RETURN_IF((argc < FusionConnectivity::ARGS_SIZE_ONE), "wrong argument count", napi_invalid_arg);
    napi_valuetype valuetype;
    napi_typeof(env, argv[FusionConnectivity::PARAM0], &valuetype);
    NAPI_FCM_RETURN_IF((valuetype != napi_function), "callback not function", napi_invalid_arg);
    return napi_ok;
}

bool IsStateChangeNapiCallbackExist(napi_env env)
{
    std::lock_guard<std::mutex> lock(g_stateChangeCallbacksMutex);
    for (auto it = g_stateChangeCallbacks.begin(); it != g_stateChangeCallbacks.end(); ++it) {
        if (it->first == env && it->second != nullptr) {
            return true;
        }
    }
    return false;
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
    if (IsStateChangeNapiCallbackExist(env)) {
        HILOGI("Napi ResultCallback");
        return FusionConnectivity::NapiGetInt32(env, FusionConnectivity::FCM_ERR_DEVICE_ALREADY_BOUNDED);
    }
    auto napiStateCb = std::make_shared<FusionConnectivity::NapiCallback>(env, argv[FusionConnectivity::PARAM0]);
    auto stateCallback = [env, napiStateCb](const RangingStateChangeInfo &state) {
        FusionConnectivity::NapiHandleScope scope(env);
        auto ret = std::make_shared<NapiNativeStateChnageData>(state);
        auto callback = [ret, napiStateCb]() {
            napiStateCb->CallFunction(ret);
        };
        FusionConnectivity::DoInJsMainThread(env, callback);
    };
    auto asynWorkFunc = [stateCallback]() {
        return ExecuteRegisterStateChangeWork(stateCallback);
    };
    auto asyncWork = FusionConnectivity::NapiAsyncWorkFactory::CreateAsyncWork(
        env, info, asynWorkFunc, FusionConnectivity::ASYNC_WORK_NO_NEED_CALLBACK);
    asyncWork->Run();
    NAPI_FCM_ASSERT_RETURN_UNDEF(env, asyncWork, FusionConnectivity::FCM_ERR_INTERNAL_ERROR);
    {
        std::lock_guard<std::mutex> lock(g_stateChangeCallbacksMutex);
        g_stateChangeCallbacks[env] = napiStateCb;
    }
    HILOGI("OnRangingStateChange success");
    return asyncWork->GetRet();
}

napi_value OffRangingStateChange(napi_env env, napi_callback_info info)
{
    auto asyncWorkFunc = []() -> FusionConnectivity::NapiAsyncWorkRet {
        auto ret = FusionRangingManager::GetInstance()->OffRangingStateChange();
        HILOGI("OffRangingStateChange ret:%{public}d", ret);
        return {ret, nullptr};
    };
    auto asyncWork = FusionConnectivity::NapiAsyncWorkFactory::CreateAsyncWork(
        env, info, asyncWorkFunc, FusionConnectivity::ASYNC_WORK_NO_NEED_CALLBACK);
    asyncWork->Run();
    NAPI_FCM_ASSERT_RETURN_UNDEF(env, asyncWork, FusionConnectivity::FCM_ERR_INTERNAL_ERROR);
    {
        std::lock_guard<std::mutex> lock(g_stateChangeCallbacksMutex);
        auto it = g_stateChangeCallbacks.find(env);
        if (it != g_stateChangeCallbacks.end()) {
            HILOGI("OffRangingStateChange: cleanup callback ref");
            g_stateChangeCallbacks.erase(it);
        }
    }
    return asyncWork->GetRet();
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