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

#include "napi_fusion_ranging_utils.h"

namespace OHOS {
namespace FusionRanging {

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

napi_value NapiNativeRangingStateChange::ToNapiValue(napi_env env) const
{
    return GenerateRangingStateChangeInfo(env, stateInfo_);
}

napi_value NapiNativeRangingResult::ToNapiValue(napi_env env) const
{
    return GenerateRangingResult(env, resultData_);
}
}
}