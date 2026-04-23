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

#include "state_observer_registry.h"
#include "log_utils.h"

namespace OHOS {
namespace FusionRanging {

bool StateObserverRegistry::Register(int32_t uid, const sptr<IRangingStateObserver> &observer)
{
    if (observer == nullptr) {
        HILOGE("Register: observer is nullptr");
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (observers_.find(uid) != observers_.end()) {
        HILOGW("Register: observer already registered for uid=%{public}d", uid);
        return false;
    }

    observers_[uid] = observer;
    HILOGI("Register: success, uid=%{public}d", uid);
    return true;
}

bool StateObserverRegistry::Unregister(int32_t uid)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = observers_.find(uid);
    if (it == observers_.end()) {
        HILOGW("Unregister: observer not found for uid=%{public}d", uid);
        return false;
    }

    observers_.erase(it);
    HILOGI("Unregister: success, uid=%{public}d", uid);
    return true;
}

sptr<IRangingStateObserver> StateObserverRegistry::Find(int32_t uid) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = observers_.find(uid);
    if (it != observers_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<int32_t> StateObserverRegistry::GetAllUids() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<int32_t> uids;
    for (const auto &pair : observers_) {
        uids.push_back(pair.first);
    }
    return uids;
}

bool StateObserverRegistry::HasObserver(int32_t uid) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return observers_.find(uid) != observers_.end();
}

size_t StateObserverRegistry::GetObserverCount() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return observers_.size();
}

void StateObserverRegistry::ClearAll()
{
    std::lock_guard<std::mutex> lock(mutex_);
    HILOGI("ClearAll: clear all state observers, count=%{public}zu", observers_.size());
    observers_.clear();
}
}  // namespace FusionRanging
}  // namespace OHOS