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

#ifndef FUSION_RANGING_STATE_OBSERVER_REGISTRY_H
#define FUSION_RANGING_STATE_OBSERVER_REGISTRY_H

#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include "ranging_state_observer_stub.h"
#include "refbase.h"

namespace OHOS {
namespace FusionRanging {

class StateObserverRegistry {
public:
    StateObserverRegistry() = default;
    ~StateObserverRegistry() = default;

    bool Register(int32_t uid, const sptr<IRangingStateObserver> &observer);
    bool Unregister(int32_t uid);
    sptr<IRangingStateObserver> Find(int32_t uid) const;
    std::vector<int32_t> GetAllUids() const;
    bool HasObserver(int32_t uid) const;
    size_t GetObserverCount() const;
    void ClearAll();

private:
    mutable std::mutex mutex_;
    std::map<int32_t, sptr<IRangingStateObserver>> observers_;
};
}  // namespace FusionRanging
}  // namespace OHOS
#endif  // FUSION_RANGING_STATE_OBSERVER_REGISTRY_H