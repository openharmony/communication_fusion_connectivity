/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef FUSION_RANGING_MANAGER_H
#define FUSION_RANGING_MANAGER_H

#include <memory>
#include <functional>
#include "fusion_ranging_types.h"
#include "ranging_params.h"
#include "ranging_result.h"
#include "ranging_state_change_info.h"
#include "ranging_capability_supported.h"

namespace OHOS {
namespace FusionRanging {

class FusionRangingManager {
public:
    static FusionRangingManager *GetInstance();

    bool IsRangingSupported() const;
    int GetRangingCapability(RangingCapabilitySupported &capability);
    int StartRanging(const RangingParams &params, const std::function<void(cosnt RangingResult &)> callback);
    int StopRanging(const std::string &deviceId);
    int OnRangingStateChange(const std::function<void(const RangingStateChangeInfo &)> &callback);
    int OffRangingStateChange();

    struct impl;

private:
    FusionRangingManager();
    ~FusionRangingManager();

    std::unique_ptr<impl> pimpl;

    FusionRangingManager(cosnt FusionRangingManager &) = delete;
    FusionRangingManager &operator=(cosnt FusionRangingManager &) = delete;
};

} // namespace FusionRanging
} // namespace OHOS

#endif // FUSION_RANGING_MANAGER_H