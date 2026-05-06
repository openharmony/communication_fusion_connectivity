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
#include "fusion_ranging_server.h"
#include "log_utils.h"

namespace OHOS {
namespace FusionRanging {

bool ProcessDeathManager::RegisterProcessDeath(int32_t uid, const sptr<IRemoteObject> &remoteObject)
{
    if (remoteObject == nullptr) {
        HILOGE("remoteObject is nullptr");
        return false;
    }
    std::shared_ptr<ProcessDeathHandler> handler = nullptr;
    auto ret = processRecipients_.Find(uid, handler);
    if (ret && handler != nullptr) {
        HILOGW("handler already exists for uid=%{public}d", uid);
        return true;
    }

    auto deathRecipient = sptr<ProcessDeathRecipient>::MakeSptr(
        uid, [this](int32_t uid) { FusionRangingServer::GetInstance()->HandleProcessDeath(uid); });
    if (deathRecipient == nullptr) {
        HILOGE("failed to create death recipient");
        return false;
    }

    bool addResult = remoteObject->AddDeathRecipient(deathRecipient);
    if (!addResult) {
        HILOGE("failed to add death recipient to remoteObject");
        return false;
    }

    auto object = std::make_shared<ProcessDeathHandler>(deathRecipient, remoteObject);
    if (object == nullptr) {
        HILOGE("crate death recipient object fail");
        remoteObject->RemoveDeathRecipient(deathRecipient);
        return false;
    }
    processRecipients_.EnsureInsert(uid, object);
    HILOGI("success, uid=%{public}d", uid);
    return true;
}

bool ProcessDeathManager::DeregisterProcessDeath(int32_t uid)
{
    std::shared_ptr<ProcessDeathHandler> handler = nullptr;
    auto ret = processRecipients_.Find(uid, handler);
    if (ret && handler != nullptr) {
        sptr<IRemoteObject> remoteObj = handler->remoteObject.promote();
        if (remoteObj != nullptr) {
            remoteObj->RemoveDeathRecipient(handler->recipient);
        }
        processRecipients_.Erase(uid);
        return true;
    }
    return false;
}

bool ProcessDeathManager::HasProcessDeathHandler(int32_t uid)
{
    std::shared_ptr<ProcessDeathHandler> handler = nullptr;
    auto ret = processRecipients_.Find(uid, handler);
    return (ret && handler != nullptr);
}

void ProcessDeathManager::ClearAll()
{
    processRecipients_.Iterate([&](const int32_t uid, const std::shared_ptr<ProcessDeathHandler> &value) {
        sptr<IRemoteObject> remoteObj = value->remoteObject.promote();
        if (remoteObj != nullptr) {
            remoteObj->RemoveDeathRecipient(value->recipient);
        }
    });
    processRecipients_.Clear();
}
}  // namespace FusionRanging
}  // namespace OHOS