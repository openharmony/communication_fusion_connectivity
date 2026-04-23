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

#include "process_death_manager.h"

#include "log_utils.h"

namespace OHOS {
namespace FusionRanging {

void ProcessDeathManager::SetDeathCallback(DeathCallback callback)
{
    deathCallback_ = callback;
}

bool ProcessDeathManager::RegisterProcessDeath(int32_t uid, const sptr<IRemoteObject> &remoteObject)
{
    if (remoteObject == nullptr) {
        HILOGE("RegisterProcessDeath: remoteObject is nullptr");
        return false;
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (deathHandlers_.find(uid) != deathHandlers_.end()) {
            HILOGW("RegisterProcessDeath: handler already exists for uid=%{public}d", uid);
            return false;
        }
    }
    auto recipient = sptr<ProcessDeathRecipient>::MakeSptr(uid, deathCallback_);
    if (recipient == nullptr) {
        HILOGE("RegisterProcessDeath: failed to create death recipient");
        return false;
    }

    bool addResult = remoteObject->AddDeathRecipient(recipient);
    if (!addResult) {
        HILOGE("RegisterProcessDeath: failed to add death recipient to remoteObject");
        return false;
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        deathHandlers_[uid] = ProcessDeathHandler(recipient, remoteObject);
    }
    HILOGI("RegisterProcessDeath: success, uid=%{public}d", uid);
    return true;
}

bool ProcessDeathManager::UnregisterProcessDeath(int32_t uid)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = deathHandlers_.find(uid);
    if (it == deathHandlers_.end()) {
        HILOGW("UnregisterProcessDeath: handler not found for uid=%{public}d", uid);
        return false;
    }

    sptr<IRemoteObject> remoteObj = it->second.remoteObject.promote();
    if (remoteObj != nullptr) {
        remoteObj->RemoveDeathRecipient(it->second.recipient);
    }

    deathHandlers_.erase(it);
    HILOGI("UnregisterProcessDeath: success, uid=%{public}d", uid);
    return true;
}

bool ProcessDeathManager::HasProcessDeathHandler(int32_t uid) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return deathHandlers_.find(uid) != deathHandlers_.end();
}

void ProcessDeathManager::ClearAll()
{
    std::lock_guard<std::mutex> lock(mutex_);
    HILOGI("ClearAll: clear all process death handlers, count=%{public}zu", deathHandlers_.size());
    for (const auto &pair : deathHandlers_) {
        sptr<IRemoteObject> remoteObj = pair.second.remoteObject.promote();
        if (remoteObj != nullptr) {
            remoteObj->RemoveDeathRecipient(pair.second.recipient);
        }
    }
    deathHandlers_.clear();
}
}  // namespace FusionRanging
}  // namespace OHOS