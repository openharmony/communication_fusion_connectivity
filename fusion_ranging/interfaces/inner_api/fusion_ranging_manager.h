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

class FusionRangingObserver {
public:
    virtual ~FusionRangingObserver() = default;

    virtual void OnRangingStateChanged(const RangingStateChangeInfo &statechange) = 0;
    virtual void OnRangingResult(const RangingResult &rangingResult) = 0;
};

class FusionRangingManager {
public:
    static FusionRangingManager *GetInstance();

    bool IsRangingSupported() const;
    int GetRangingCapability(RangingCapabilitySupported &capability);
    int RegisterFusionRangingObserver(const std::shared_ptr<FusionRangingObserver> &observer);
    int DeregisterFusionRangingObserver(const std::shared_ptr<FusionRangingObserver> &observer);
    int StartRanging(const RangingParams &params);
    int StopRanging(const RangingParams &params);
    int StartPassiveRanging(RangingTypes capabilityType, int32_t &handle);
    int StopPassiveRanging(RangingTypes capabilityType, int32_t handle);

private:
    FusionRangingManager();
    ~FusionRangingManager();

    struct impl;
    std::unique_ptr<impl> pimpl;

    FusionRangingManager(const FusionRangingManager &) = delete;
    FusionRangingManager &operator=(const FusionRangingManager &) = delete;
};
}  // namespace FusionRanging
}  // namespace OHOS
#endif  // FUSION_RANGING_MANAGER_H