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

#ifndef FUSION_RANGING_OBSERVER_H
#define FUSION_RANGING_OBSERVER_H

#include <functional>
#include "ranging_observer_stub.h"
#include "system_ability_status_change_stub.h"

namespace OHOS {
namespace FusionRanging {

class RangingObserverHandler : public RefBase {
public:
    virtual void OnRangingStateChanged(const RangingStateChangeInfo &info) = 0;
    virtual void OnRangingResult(const RangingResult &result) = 0;
};

class RangingObserverImpl : public RangingObserverStub {
public:
    explicit RangingObserverImpl(const wptr<RangingObserverHandler> &handler) : handler_(handler) {}
    ~RangingObserverImpl() = default;

    int32_t OnRangingStateChanged(const RangingStateChangeInfo &info) override;
    int32_t OnRangingResult(const RangingResult &result) override;

private:
    wptr<RangingObserverHandler> handler_;
};
}  // namespace FusionRanging
}  // namespace OHOS
#endif  // FUSION_RANGING_OBSERVER_H