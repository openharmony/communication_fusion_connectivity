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
#include "napi_parser_utils.h"

namespace OHOS {
namespace FusionRanging {
using namespace FusionConnectivity;

static napi_value CreateNapiRangingMeasureMentValue(napi_env env, const RangingMeasurement &measurement)
{
    napi_value result = nullptr;
    napi_create_object(env, &result);
    NAPI_FCM_RETURN_IF(result == nullptr, "Create Napi Measurement err", result);
    SetNamedPropertyByInteger(env, result, measurement.GetValue(), "value");
    SetNamedPropertyByInteger(env, result, static_cast<int32_t>(measurement.GetConfidence()), "confidence");
    return result;
}

static napi_value GenerateRangingStateChangeInfo(napi_env env, const RangingStateChangeInfo &info)
{
    napi_value result = nullptr;
    napi_create_object(env, &result);
    NAPI_FCM_RETURN_IF(result == nullptr, "Generate statechange Err", result);
    FusionConnectivity::SetNamedPropertyByInteger(env, result, static_cast<int32_t>(info.GetState()), "state");
    FusionConnectivity::SetNamedPropertyByInteger(env, result, static_cast<int32_t>(info.GetCause()), "cause");
    return result;
}

static napi_value GenerateRangingResult(napi_env env, const RangingResult &result)
{
    napi_value retObj = nullptr;
    napi_create_object(env, &retObj);
    NAPI_FCM_RETURN_IF(retObj == nullptr, "Generate result err", retObj);
    FusionConnectivity::SetNamedPropertyByString(env, retObj, result.GetDeviceId(), "deviceId");

    napi_value distance = CreateNapiRangingMeasureMentValue(env, result.GetDistance());
    NAPI_FCM_RETURN_IF(distance == nullptr, "Create Napi distance err", retObj);
    napi_set_named_property(env, retObj, "distance", distance);

    napi_value angle = CreateNapiRangingMeasureMentValue(env, result.GetAngle());
    NAPI_FCM_RETURN_IF(angle == nullptr, "Create Napi angle err", retObj);
    napi_set_named_property(env, retObj, "angle", angle);

    FusionConnectivity::SetNamedPropertyByInteger(env, retObj, result.GetRssi(), "rssi");
    return retObj;
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