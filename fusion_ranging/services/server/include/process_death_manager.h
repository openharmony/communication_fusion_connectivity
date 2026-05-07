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

#ifndef FUSION_RANGING_PROCESS_DEATH_MANAGER_H
#define FUSION_RANGING_PROCESS_DEATH_MANAGER_H

#include <functional>
#include <map>
#include <memory>
#include <mutex>

#include "iremote_object.h"
#include "refbase.h"
#include "safe_map.h"

namespace OHOS {
namespace FusionRanging {

class ProcessDeathManager {
public:
    using DeathCallback = std::function<void(int32_t uid)>;

    ProcessDeathManager() = default;
    ~ProcessDeathManager() = default;

    bool RegisterProcessDeath(int32_t uid, const sptr<IRemoteObject> &remoteObject);
    bool DeregisterProcessDeath(int32_t uid);
    bool HasProcessDeathHandler(int32_t uid);
    void ClearAll();

private:
    class ProcessDeathRecipient : public IRemoteObject::DeathRecipient {
    public:
        explicit ProcessDeathRecipient(int32_t uid, DeathCallback callback) : uid_(uid), callback_(callback) {}
        ~ProcessDeathRecipient() override = default;

        void OnRemoteDied(const wptr<IRemoteObject> &) override
        {
            if (callback_) {
                callback_(uid_);
            }
        }

    private:
        int32_t uid_;
        DeathCallback callback_;
    };

    struct ProcessDeathHandler {
        ProcessDeathHandler() = default;
        ProcessDeathHandler(sptr<IRemoteObject::DeathRecipient> r, const wptr<IRemoteObject> &obj)
            : recipient(r),
              remoteObject(obj)
        {
        }
        sptr<IRemoteObject::DeathRecipient> recipient;
        wptr<IRemoteObject> remoteObject;
    };

    SafeMap<int32_t, std::shared_ptr<ProcessDeathHandler>> processRecipients_;
};
}  // namespace FusionRanging
}  // namespace OHOS
#endif  // FUSION_RANGING_PROCESS_DEATH_MANAGER_H