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
#include "napi_fusion_ranging_callback.h"
#include "napi_fusion_ranging_utils.h"
#include "log_utils.h"

namespace OHOS {
namespace FusionRanging {
using namespace FusionConnectivity;
namespace {
std::shared_ptr<NapiFusionRangingObserver> napiCallback_ = std::make_shared<NapiFusionRangingObserver>();

static bool IsAllowedErrCode(int32_t ret, const int32_t *allowedCodes, size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        if (allowedCodes[i] == ret) {
            return true;
        }
    }
    return false;
}

static int32_t StandardizeErrCode(int32_t ret, const int32_t *allowedCodes, size_t count,
                                  int32_t fallback = RANGING_ERR_OPERATION_FAILED)
{
    if (IsAllowedErrCode(ret, allowedCodes, count)) {
        return ret;
    }
    HILOGW("Unexpected errCode %{public}d, fallback to %{public}d", ret, fallback);
    return fallback;
}

#define DECLARE_ALLOWED_ERR_CODES(name, ...) constexpr int32_t name[] = {__VA_ARGS__}

DECLARE_ALLOWED_ERR_CODES(ERR_CODES_GET_RANGING_CAPABILITY, RANGING_NO_ERROR, RANGING_ERR_PERMISSION_FAILED,
                          RANGING_ERR_INVALID_PARAM, RANGING_ERR_API_NOT_SUPPORT, RANGING_ERR_RANGING_SERVICE_DISABLED);

DECLARE_ALLOWED_ERR_CODES(ERR_CODES_START_RANGING, RANGING_NO_ERROR, RANGING_ERR_PERMISSION_FAILED,
                          RANGING_ERR_INVALID_PARAM, RANGING_ERR_API_NOT_SUPPORT, RANGING_ERR_DEVICE_ALREADY_INITIATED,
                          RANGING_ERR_RANGING_TYPE_NOT_SUPPORT, RANGING_ERR_RANGING_SERVICE_DISABLED,
                          RANGING_ERR_PARAM_NOT_MEET_SPECIFICATIONS, RANGING_ERR_OPERATION_FAILED);

DECLARE_ALLOWED_ERR_CODES(ERR_CODES_STOP_RANGING, RANGING_NO_ERROR, RANGING_ERR_PERMISSION_FAILED,
                          RANGING_ERR_INVALID_PARAM, RANGING_ERR_API_NOT_SUPPORT, RANGING_ERR_DEVICE_NOT_INITIATED,
                          RANGING_ERR_RANGING_TYPE_NOT_SUPPORT, RANGING_ERR_PARAM_NOT_MEET_SPECIFICATIONS,
                          RANGING_ERR_OPERATION_FAILED);

DECLARE_ALLOWED_ERR_CODES(ERR_CODES_START_PASSIVE_RANGING, RANGING_NO_ERROR, RANGING_ERR_PERMISSION_FAILED,
                          RANGING_ERR_INVALID_PARAM, RANGING_ERR_API_NOT_SUPPORT, RANGING_ERR_RANGING_TYPE_NOT_SUPPORT,
                          RANGING_ERR_RANGING_SERVICE_DISABLED, RANGING_ERR_OPERATION_FAILED);

DECLARE_ALLOWED_ERR_CODES(ERR_CODES_STOP_PASSIVE_RANGING, RANGING_NO_ERROR, RANGING_ERR_PERMISSION_FAILED,
                          RANGING_ERR_INVALID_PARAM, RANGING_ERR_API_NOT_SUPPORT, RANGING_ERR_RANGING_TYPE_NOT_SUPPORT,
                          RANGING_ERR_PARAM_NOT_MEET_SPECIFICATIONS, RANGING_ERR_OPERATION_FAILED);

#define OUTPUT_ERR(ret, codes, ...) \
    StandardizeErrCode((ret), (codes), sizeof((codes)) / sizeof((codes)[0]), ##__VA_ARGS__)

static bool IsCapabilityTypeValid(int32_t capabilityType)
{
    return capabilityType >= static_cast<int32_t>(RangingTypes::NEARLINK_HADM) &&
           capabilityType < static_cast<int32_t>(RangingTypes::RANGING_TYPE_MAX);
}

static napi_value GenerateRangingCapabilitySupported(napi_env env, const RangingCapabilitySupported &cap)
{
    napi_value result = nullptr;
    napi_create_object(env, &result);
    NAPI_FCM_RETURN_IF(result == nullptr, "Generate Capability err", result);
    napi_value nearlinkHadm = NapiGetBooleanRet(env, cap.GetNearlinkHadm());
    NAPI_FCM_RETURN_IF(nearlinkHadm == nullptr, "Create Napi nearlinkHadm err", result);
    napi_set_named_property(env, result, "nearlinkHadm", nearlinkHadm);
    return result;
}
}  // anonymous namespace

class NapiNativeRangingCapabilityData : public NapiNativeObject {
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

void DefineRangingInterface(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("isRangingSupported", IsRangingSupported),
        DECLARE_NAPI_FUNCTION("getRangingCapability", GetRangingCapability),
        DECLARE_NAPI_FUNCTION("startRanging", StartRanging),
        DECLARE_NAPI_FUNCTION("stopRanging", StopRanging),
        DECLARE_NAPI_FUNCTION("startPassiveRanging", StartPassiveRanging),
        DECLARE_NAPI_FUNCTION("stopPassiveRanging", StopPassiveRanging),
        DECLARE_NAPI_FUNCTION("onRangingStateChange", OnRangingStateChange),
        DECLARE_NAPI_FUNCTION("offRangingStateChange", OffRangingStateChange),
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
}

napi_value IsRangingSupported(napi_env env, napi_callback_info info)
{
    NAPI_FCM_ASSERT_RETURN_UNDEF(env, CheckEmptyParams(env, info) == napi_ok, RANGING_ERR_INVALID_PARAM);
    bool isSupported = FusionRangingManager::GetInstance()->IsRangingSupported();
    HILOGI("IsRangingSupported result is %{public}d", isSupported);
    return NapiGetBooleanRet(env, isSupported);
}

napi_value GetRangingCapability(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    NAPI_FCM_ASSERT_RETURN_UNDEF(env, CheckEmptyParams(env, info) == napi_ok, RANGING_ERR_INVALID_PARAM);
    auto asyncWorkFunc = [env]() -> NapiAsyncWorkRet {
        RangingCapabilitySupported cap;
        int ret = FusionRangingManager::GetInstance()->GetRangingCapability(cap);
        ret = OUTPUT_ERR(ret, ERR_CODES_GET_RANGING_CAPABILITY, RANGING_ERR_RANGING_SERVICE_DISABLED);
        auto nativeObj = std::make_shared<NapiNativeRangingCapabilityData>(cap);
        return NapiAsyncWorkRet{ret, nativeObj};
    };

    auto asyncWork = NapiAsyncWorkFactory::CreateAsyncWork(env, info, asyncWorkFunc, ASYNC_WORK_NO_NEED_CALLBACK);
    NAPI_FCM_ASSERT_RETURN_UNDEF(env, asyncWork, RANGING_ERR_INVALID_PARAM);
    asyncWork->Run();
    return asyncWork->GetRet();
}

static int ParseRangingParams(napi_env env, napi_value object, RangingParams &params)
{
    NAPI_FCM_RETURN_IF(NapiCheckObjectPropertiesName(env, object, {"deviceId", "capabilityType"}) != napi_ok,
                       "params not equal", RANGING_ERR_INVALID_PARAM);
    std::string deviceId = "";
    int32_t capabilityType = 0;
    NAPI_FCM_RETURN_IF(NapiParseStringObject(env, object, "deviceId", deviceId) != napi_ok, "parse device id err",
                       RANGING_ERR_INVALID_PARAM);
    NAPI_FCM_RETURN_IF(!IsValidAddress(deviceId), "Invalid deviceId", RANGING_ERR_INVALID_PARAM);
    NAPI_FCM_RETURN_IF(NapiParseInt32Object(env, object, "capabilityType", capabilityType) != napi_ok, "parse cap err",
                       RANGING_ERR_INVALID_PARAM);
    NAPI_FCM_RETURN_IF(!IsCapabilityTypeValid(capabilityType), "invalid cap err", RANGING_ERR_INVALID_PARAM);
    HILOGI("ParseRangingParams deviceId:%{public}s, cap:%{public}d", GET_ENCRYPT_ADDR(deviceId), capabilityType);
    params.SetDeviceId(deviceId);
    params.SetCapabilityType(static_cast<RangingTypes>(capabilityType));
    return RANGING_NO_ERROR;
}

napi_value StartRanging(napi_env env, napi_callback_info info)
{
    HILOGI("StartRanging napi enter.");
    size_t argc = ARGS_SIZE_TWO;
    napi_value argv[ARGS_SIZE_TWO] = {nullptr};
    NAPI_FCM_ASSERT_RETURN_UNDEF(env, (napiCallback_ != nullptr), RANGING_ERR_INVALID_PARAM);
    napi_status cbInfoStatus = napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    HILOGI("StartRanging: cbInfoStatus=%{public}d argc=%{public}zu", cbInfoStatus, argc);
    NAPI_FCM_ASSERT_RETURN_UNDEF(env, argc == ARGS_SIZE_TWO, RANGING_ERR_INVALID_PARAM);
    RangingParams params;
    auto parseRet = ParseRangingParams(env, argv[PARAM0], params);
    NAPI_FCM_ASSERT_RETURN_UNDEF(env, (parseRet == RANGING_NO_ERROR), parseRet);
    auto status = NapiIsFunction(env, argv[PARAM1]);
    NAPI_FCM_ASSERT_RETURN_UNDEF(env, (status == napi_ok), RANGING_ERR_INVALID_PARAM);

    auto napiCallback = std::make_shared<NapiCallback>(env, argv[PARAM1]);
    auto resultCallback = [env, napiCallback](const RangingResult &result) {
        auto ret = std::make_shared<NapiNativeRangingResult>(result);
        auto callback = [env, napiCallback, ret]() {
            NapiHandleScope scope(env);
            napiCallback->CallFunction(ret);
        };
        DoInJsMainThread(env, callback);
    };
    FusionRangingManager::GetInstance()->RegisterFusionRangingObserver(napiCallback_);
    napiCallback_->RegisterRangingResultCallback(params, napiCallback, resultCallback);
    auto ret = FusionRangingManager::GetInstance()->StartRanging(params);
    HILOGI("StartRanging: ret=%{public}d", ret);

    if (ret == RANGING_ERR_DEVICE_ALREADY_INITIATED) {
        NAPI_FCM_ASSERT_RETURN_UNDEF(env, false, ret);
    } else if (ret != RANGING_NO_ERROR) {
        napiCallback_->DeregisterRangingResultCallbackWithDeviceId(env, argv[PARAM1], params.GetDeviceId());
        FusionRangingManager::GetInstance()->DeregisterFusionRangingObserver(napiCallback_);
        NAPI_FCM_ASSERT_RETURN_UNDEF(env, false, OUTPUT_ERR(ret, ERR_CODES_START_RANGING));
    }
    return NapiGetUndefined(env);
}

static napi_value StopRangingWithParams(napi_env env, napi_value napiCallback, napi_value object)
{
    RangingParams userParams;
    auto parseRet = ParseRangingParams(env, object, userParams);
    NAPI_FCM_ASSERT_RETURN_UNDEF(env, (parseRet == RANGING_NO_ERROR), parseRet);
    int stopRet = FusionRangingManager::GetInstance()->StopRanging(userParams);
    napiCallback_->DeregisterRangingResultCallbackWithDeviceId(env, napiCallback, userParams.GetDeviceId());
    if (napiCallback_->IsRangingResultCallbackEmpty()) {
        FusionRangingManager::GetInstance()->DeregisterFusionRangingObserver(napiCallback_);
    }
    NAPI_FCM_ASSERT_RETURN_UNDEF(env, stopRet == RANGING_NO_ERROR, OUTPUT_ERR(stopRet, ERR_CODES_STOP_RANGING));
    return NapiGetUndefined(env);
}

napi_value StopRanging(napi_env env, napi_callback_info info)
{
    size_t argc = ARGS_SIZE_TWO;
    napi_value argv[ARGS_SIZE_TWO] = {nullptr};
    napi_status cbInfoStatus = napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    HILOGI("StopRanging: cbInfoStatus=%{public}d argc=%{public}zu", cbInfoStatus, argc);
    NAPI_FCM_ASSERT_RETURN_UNDEF(env, cbInfoStatus == napi_ok, RANGING_ERR_INVALID_PARAM);
    NAPI_FCM_ASSERT_RETURN_UNDEF(env, argc >= ARGS_SIZE_ONE && argc <= ARGS_SIZE_TWO, RANGING_ERR_INVALID_PARAM);
    napi_status ret = NapiIsFunction(env, argv[PARAM0]);
    NAPI_FCM_ASSERT_RETURN_UNDEF(env, (ret == napi_ok), RANGING_ERR_INVALID_PARAM);
    if (argc == ARGS_SIZE_TWO) {
        return StopRangingWithParams(env, argv[PARAM0], argv[PARAM1]);
    }

    auto rangParams = napiCallback_->GetRangParamsByNapiCallback(env, argv[PARAM0]);
    int stopRet = RANGING_NO_ERROR;
    for (auto it = rangParams.begin(); it != rangParams.end(); ++it) {
        stopRet = FusionRangingManager::GetInstance()->StopRanging(*it);
        napiCallback_->DeregisterRangingResultCallbackWithDeviceId(env, argv[PARAM0], it->GetDeviceId());
        if (stopRet != RANGING_NO_ERROR) {
            break;
        }
    }
    if (napiCallback_->IsRangingResultCallbackEmpty()) {
        FusionRangingManager::GetInstance()->DeregisterFusionRangingObserver(napiCallback_);
    }
    NAPI_FCM_ASSERT_RETURN_UNDEF(env, stopRet == RANGING_NO_ERROR, OUTPUT_ERR(stopRet, ERR_CODES_STOP_RANGING));
    return NapiGetUndefined(env);
}

napi_value StartPassiveRanging(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    size_t argc = ARGS_SIZE_ONE;
    napi_value argv[ARGS_SIZE_ONE] = {nullptr};
    napi_status cbInfoStatus = napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    HILOGI("StartPassiveRanging: cbInfoStatus=%{public}d argc=%{public}zu", cbInfoStatus, argc);
    NAPI_FCM_ASSERT_RETURN_UNDEF(env, argc == ARGS_SIZE_ONE, RANGING_ERR_INVALID_PARAM);
    int32_t capabilityType = 0;
    NAPI_FCM_ASSERT_RETURN_UNDEF(env, ParseInt32(env, capabilityType, argv[PARAM0]), RANGING_ERR_INVALID_PARAM);
    NAPI_FCM_ASSERT_RETURN_UNDEF(env, IsCapabilityTypeValid(capabilityType), RANGING_ERR_INVALID_PARAM);

    auto asyncWorkFunc = [env, capabilityType]() -> NapiAsyncWorkRet {
        int handle = -1; /* -1 default as invaild handle */
        auto ret =
            FusionRangingManager::GetInstance()->StartPassiveRanging(static_cast<RangingTypes>(capabilityType), handle);
        ret = OUTPUT_ERR(ret, ERR_CODES_START_PASSIVE_RANGING);
        auto nativeObj = std::make_shared<NapiNativeInt>(handle);
        return NapiAsyncWorkRet{ret, nativeObj};
    };

    auto asyncWork = NapiAsyncWorkFactory::CreateAsyncWork(env, info, asyncWorkFunc, ASYNC_WORK_NO_NEED_CALLBACK);
    NAPI_FCM_ASSERT_RETURN_UNDEF(env, asyncWork, RANGING_ERR_OPERATION_FAILED);
    asyncWork->Run();
    FusionRangingManager::GetInstance()->RegisterFusionRangingObserver(napiCallback_);
    return asyncWork->GetRet();
}

napi_value StopPassiveRanging(napi_env env, napi_callback_info info)
{
    size_t argc = ARGS_SIZE_TWO;
    napi_value argv[ARGS_SIZE_TWO] = {nullptr};
    napi_status cbInfoStatus = napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    HILOGI("StopPassiveRanging: cbInfoStatus=%{public}d argc=%{public}zu", cbInfoStatus, argc);
    NAPI_FCM_ASSERT_RETURN_UNDEF(env, cbInfoStatus == napi_ok && argc == ARGS_SIZE_TWO, RANGING_ERR_INVALID_PARAM);
    int handle = -1; /* default -1 as invalid handle. */
    NAPI_FCM_ASSERT_RETURN_UNDEF(env, ParseInt32(env, handle, argv[PARAM0]), RANGING_ERR_INVALID_PARAM);
    NAPI_FCM_ASSERT_RETURN_UNDEF(env, handle >= 0, RANGING_ERR_INVALID_PARAM);

    int capabilityType = 0;
    NAPI_FCM_ASSERT_RETURN_UNDEF(env, ParseInt32(env, capabilityType, argv[PARAM1]), RANGING_ERR_INVALID_PARAM);
    NAPI_FCM_ASSERT_RETURN_UNDEF(env, IsCapabilityTypeValid(capabilityType), RANGING_ERR_INVALID_PARAM);
    HILOGI("StopPassiveRanging: capabilityType=%{public}d handle=%{public}d", capabilityType, handle);
    int stopRet =
        FusionRangingManager::GetInstance()->StopPassiveRanging(static_cast<RangingTypes>(capabilityType), handle);
    NAPI_FCM_ASSERT_RETURN_UNDEF(env, stopRet == RANGING_NO_ERROR, OUTPUT_ERR(stopRet, ERR_CODES_STOP_PASSIVE_RANGING));
    return NapiGetUndefined(env);
}

napi_value OnRangingStateChange(napi_env env, napi_callback_info info)
{
    NAPI_FCM_ASSERT_RETURN_UNDEF(env, FusionRangingManager::GetInstance()->IsRangingSupported(),
                                 RANGING_ERR_API_NOT_SUPPORT);
    if (napiCallback_ != nullptr) {
        auto status =
            napiCallback_->eventSubscribe_.RegisterWithName(env, info, STR_FUSION_RANGING_CALLBACK_STATE_CHANGE);
        NAPI_FCM_ASSERT_RETURN_UNDEF(env, status == napi_ok, RANGING_ERR_INVALID_PARAM);
    }
    return NapiGetUndefined(env);
}

napi_value OffRangingStateChange(napi_env env, napi_callback_info info)
{
    NAPI_FCM_ASSERT_RETURN_UNDEF(env, FusionRangingManager::GetInstance()->IsRangingSupported(),
                                 RANGING_ERR_API_NOT_SUPPORT);
    if (napiCallback_) {
        auto status =
            napiCallback_->eventSubscribe_.DeregisterWithName(env, info, STR_FUSION_RANGING_CALLBACK_STATE_CHANGE);
        NAPI_FCM_ASSERT_RETURN_UNDEF(env, status == napi_ok, RANGING_ERR_INVALID_PARAM);
    }
    return NapiGetUndefined(env);
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