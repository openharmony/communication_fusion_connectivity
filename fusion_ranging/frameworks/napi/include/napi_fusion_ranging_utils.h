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

#ifndef NAPI_FUSION_RANGING_UTILS_H
#define NAPI_FUSION_RANGING_UTILS_H

#include "ranging_result.h"
#include "napi_native_object.h"
#include "ranging_state_change_info.h"

namespace OHOS {
namespace FusionRanging {

class NapiNativeRangingStateChange : public FusionConnectivity::NapiNativeObject {
public:
    explicit NapiNativeRangingStateChange(const RangingStateChangeInfo &stateInfo) : stateInfo_(stateInfo) {}
    ~NapiNativeRangingStateChange() override = default;

    napi_value ToNapiValue(napi_env env) const override;

private:
    RangingStateChangeInfo stateInfo_;
};

class NapiNativeRangingResult : public FusionConnectivity::NapiNativeObject {
public:
    explicit NapiNativeRangingResult(const RangingResult &resultData) : resultData_(resultData) {}
    ~NapiNativeRangingResult() override = default;

    napi_value ToNapiValue(napi_env env) const override;

private:
    RangingResult resultData_;
};
}
}
#endif  // NAPI_FUSION_RANGING_UTILS_H