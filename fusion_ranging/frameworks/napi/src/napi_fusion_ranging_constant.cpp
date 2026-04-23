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
constexpr int32_t RANGING_STATE_STARTING = 1;
constexpr int32_t RANGING_STATE_STARTED = 2;

constexpr int32_t RANGING_ROLE_INITIATOR = 0;
constexpr int32_t RANGING_ROLE_RESPONDER = 1;

constexpr int32_t RANGING_STOPPED_CAUSE_NO_ERROR = 0;
constexpr int32_t RANGING_STOPPED_CAUSE_INTERNAL_ERROR = 1;
constexpr int32_t RANGING_STOPPED_CAUSE_BUSINESS_CONFLICT = 2;
constexpr int32_t RANGING_STOPPED_CAUSE_LIMITED_RESOURCE = 3;

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
    napi_value rangingRoleObj = RangingRoleInit(env);
    napi_value rangingStoppedCauseObj = RangingStoppedCauseInit(env);
    napi_value rangingConfidenceObj = RangingConfidenceInit(env);
    napi_property_descriptor exportFuncs[] = {
        DECLARE_NAPI_PROPERTY("RangingTypes", rangingTypesObj),
        DECLARE_NAPI_PROPERTY("RangingState", rangingStateObj),
        DECLARE_NAPI_PROPERTY("RangingRole", rangingRoleObj),
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
    if (status != napi_ok) {
        HILOGE("RangingTypesInit create object failed");
        return nullptr;
    }
    FusionConnectivity::SetNamedPropertyByInteger(env, rangingTypes, RANGING_TYPES_NEARLINK_HADM, "NEARLINK_HADM");
    return rangingTypes;
}

napi_value NapiFusionRangingConstant::RangingStateInit(napi_env env)
{
    napi_value rangingState = nullptr;
    napi_status status = napi_create_object(env, &rangingState);
    if (status != napi_ok) {
        HILOGE("RangingStateInit: create object failed");
        return nullptr;
    }
    FusionConnectivity::SetNamedPropertyByInteger(env, rangingState, RANGING_STATE_STOPPED, "STATE_STOPPED");
    FusionConnectivity::SetNamedPropertyByInteger(env, rangingState, RANGING_STATE_STARTING, "STATE_STARTING");
    FusionConnectivity::SetNamedPropertyByInteger(env, rangingState, RANGING_STATE_STARTED, "STATE_STARTED");
    return rangingState;
}

napi_value NapiFusionRangingConstant::RangingRoleInit(napi_env env)
{
    napi_value rangingRole = nullptr;
    napi_status status = napi_create_object(env, &rangingRole);
    if (status != napi_ok) {
        HILOGE("RangingRoleInit: create object failed");
        return nullptr;
    }
    FusionConnectivity::SetNamedPropertyByInteger(env, rangingRole, RANGING_ROLE_INITIATOR, "ROLE_INITIATOR");
    FusionConnectivity::SetNamedPropertyByInteger(env, rangingRole, RANGING_ROLE_RESPONDER, "ROLE_RESPONDER");
    return rangingRole;
}

napi_value NapiFusionRangingConstant::RangingStoppedCauseInit(napi_env env)
{
    napi_value rangingStoppedCause = nullptr;
    napi_status status = napi_create_object(env, &rangingStoppedCause);
    if (status != napi_ok) {
        HILOGE("RangingStoppedCauseInit: create object failed");
        return nullptr;
    }
    FusionConnectivity::SetNamedPropertyByInteger(env, rangingStoppedCause, RANGING_STOPPED_CAUSE_NO_ERROR, "NO_ERROR");
    FusionConnectivity::SetNamedPropertyByInteger(env, rangingStoppedCause, RANGING_STOPPED_CAUSE_INTERNAL_ERROR,
                                                  "INTERNAL_ERROR");
    FusionConnectivity::SetNamedPropertyByInteger(env, rangingStoppedCause, RANGING_STOPPED_CAUSE_BUSINESS_CONFLICT,
                                                  "BUSINESS_CONFLICT");
    FusionConnectivity::SetNamedPropertyByInteger(env, rangingStoppedCause, RANGING_STOPPED_CAUSE_LIMITED_RESOURCE,
                                                  "LIMITED_RESOURCE");
    return rangingStoppedCause;
}

napi_value NapiFusionRangingConstant::RangingConfidenceInit(napi_env env)
{
    napi_value rangingConfidence = nullptr;
    napi_status status = napi_create_object(env, &rangingConfidence);
    if (status != napi_ok) {
        HILOGE("RangingConfidenceInit: create object failed");
        return nullptr;
    }
    FusionConnectivity::SetNamedPropertyByInteger(env, rangingConfidence, RANGING_CONFIDENCE_HIGH, "HIGH");
    FusionConnectivity::SetNamedPropertyByInteger(env, rangingConfidence, RANGING_CONFIDENCE_MEDIUM, "MEDIUM");
    FusionConnectivity::SetNamedPropertyByInteger(env, rangingConfidence, RANGING_CONFIDENCE_LOW, "LOW");
    return rangingConfidence;
}
}  // namespace FusionRanging
}  // namespace OHOS