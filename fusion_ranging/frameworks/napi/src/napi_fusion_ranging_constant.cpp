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

#include "napi_fusion_ranging_constant.h"
#include "napi_parser_utils.h"

namespace OHOS {
namespace FusionRanging {
namespace {
constexpr int32_t RANGING_TYPES_NEARLINK_HADM = 1;

constexpr int32_t RANGING_STATE_STOPPED = 0;
constexpr int32_t RANGING_STATE_STARTED = 1;

constexpr int32_t RANGING_STOPPED_CAUSE_NO_ERROR = 0;
constexpr int32_t RANGING_STOPPED_CAUSE_INTERNAL_ERROR = 1;
constexpr int32_t RANGING_STOPPED_CAUSE_BUSINESS_CONFLICT = 2;
constexpr int32_t RANGING_STOPPED_CAUSE_BACKGROUND_PAUSED = 3;

constexpr int32_t RANGING_CONFIDENCE_HIGH = 0;
constexpr int32_t RANGING_CONFIDENCE_MEDIUM = 1;
constexpr int32_t RANGING_CONFIDENCE_LOW = 2;
}  // namespace

napi_value NapiFusionRangingConstant::DefineJSConstant(napi_env env, napi_value exports)
{
    ConstantPropertyValueInit(env, exports);
    return exports;
}

napi_value NapiFusionRangingConstant::ConstantPropertyValueInit(napi_env env, napi_value exports)
{
    HILOGD("enter");
    napi_value rangingTypesObj = RangingTypesInit(env);
    napi_value rangingStateObj = RangingStateInit(env);
    napi_value rangingStoppedCauseObj = RangingStoppedCauseInit(env);
    napi_value rangingConfidenceObj = RangingConfidenceInit(env);
    napi_property_descriptor exportFuncs[] = {
        DECLARE_NAPI_PROPERTY("RangingTypes", rangingTypesObj),
        DECLARE_NAPI_PROPERTY("RangingState", rangingStateObj),
        DECLARE_NAPI_PROPERTY("RangingStoppedCause", rangingStoppedCauseObj),
        DECLARE_NAPI_PROPERTY("RangingConfidence", rangingConfidenceObj),
    };
    napi_define_properties(env, exports, sizeof(exportFuncs) / sizeof(*exportFuncs), exportFuncs);
    return exports;
}

napi_value NapiFusionRangingConstant::RangingTypesInit(napi_env env)
{
    napi_value rangingTypes = nullptr;
    napi_status status = napi_create_object(env, &rangingTypes);
    NAPI_FCM_RETURN_IF(status != napi_ok, "RangingTypesInit: create object failed", nullptr);
    FusionConnectivity::SetNamedPropertyByInteger(env, rangingTypes, RANGING_TYPES_NEARLINK_HADM, "NEARLINK_HADM");
    return rangingTypes;
}

napi_value NapiFusionRangingConstant::RangingStateInit(napi_env env)
{
    napi_value rangingState = nullptr;
    napi_status status = napi_create_object(env, &rangingState);
    NAPI_FCM_RETURN_IF(status != napi_ok, "RangingStateInit: create object failed", nullptr);
    FusionConnectivity::SetNamedPropertyByInteger(env, rangingState, RANGING_STATE_STOPPED, "RANGING_STOPPED");
    FusionConnectivity::SetNamedPropertyByInteger(env, rangingState, RANGING_STATE_STARTED, "RANGING_STARTED");
    return rangingState;
}

napi_value NapiFusionRangingConstant::RangingStoppedCauseInit(napi_env env)
{
    napi_value rangingStoppedCause = nullptr;
    napi_status status = napi_create_object(env, &rangingStoppedCause);
    NAPI_FCM_RETURN_IF(status != napi_ok, "RangingStoppedCauseInit: create object failed", nullptr);
    FusionConnectivity::SetNamedPropertyByInteger(env, rangingStoppedCause, RANGING_STOPPED_CAUSE_NO_ERROR, "NO_ERROR");
    FusionConnectivity::SetNamedPropertyByInteger(env, rangingStoppedCause, RANGING_STOPPED_CAUSE_INTERNAL_ERROR,
                                                  "INTERNAL_ERROR");
    FusionConnectivity::SetNamedPropertyByInteger(env, rangingStoppedCause, RANGING_STOPPED_CAUSE_BUSINESS_CONFLICT,
                                                  "BUSINESS_CONFLICT");
    FusionConnectivity::SetNamedPropertyByInteger(env, rangingStoppedCause, RANGING_STOPPED_CAUSE_BACKGROUND_PAUSED,
                                                  "BACKGROUND_PAUSED");
    return rangingStoppedCause;
}

napi_value NapiFusionRangingConstant::RangingConfidenceInit(napi_env env)
{
    napi_value rangingConfidence = nullptr;
    napi_status status = napi_create_object(env, &rangingConfidence);
    NAPI_FCM_RETURN_IF(status != napi_ok, "RangingConfidenceInit: create object failed", nullptr);
    FusionConnectivity::SetNamedPropertyByInteger(env, rangingConfidence, RANGING_CONFIDENCE_HIGH, "HIGH");
    FusionConnectivity::SetNamedPropertyByInteger(env, rangingConfidence, RANGING_CONFIDENCE_MEDIUM, "MEDIUM");
    FusionConnectivity::SetNamedPropertyByInteger(env, rangingConfidence, RANGING_CONFIDENCE_LOW, "LOW");
    return rangingConfidence;
}
}  // namespace FusionRanging
}  // namespace OHOS