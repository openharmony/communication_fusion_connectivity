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

#ifndef FUSION_RANGING_OBSERVER_H
#define FUSION_RANGING_OBSERVER_H

#include <functional>
#include "ranging_state_observer_stub.h"
#include "ranging_result_observer_stub.h"
#include "system_ability_status_change_stub.h"

namespace OHOS {
namespace FusionRanging {

class RangingStateObserverImpl : public RangingStateObserverStub {
public:
    explicit RangingStateObserverImpl();
    ~RangingStateObserverImpl() = default;

    void SetStateCallback(std::function<void(const RangingStateChangeInfo &)> callback);

    int32_t OnRangingStateChanged(const RangingStateChangeInfo &info) override;

private:
    std::function<void(const RangingStateChangeInfo &)> stateCallback_;
};

class RangingResultObserverImpl : public RangingResultObserverStub {
public:
    explicit RangingResultObserverImpl();
    ~RangingResultObserverImpl() = default;

    void SetResultCallback(std::function<void(const RangingResult &)> callback);

    int32_t OnRangingResult(const RangingResult &result) override;

private:
    std::function<void(const RangingResult &)> resultCallback_;
};

class FusionRangingSaStatusChange : public SystemAbilityStatusChangeStub {
public:
    FusionRangingSaStatusChange();
    ~FusionRangingSaStatusChange() = default;

    void SetRemoveCallback(std::fuction<void()> callback);

    void OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override;
    void OnRemoveSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override;

private:
    std::function<void()> removeCallback_;
}
}
}
#endif FUSION_RANGING_OBSERVER_H