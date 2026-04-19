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

#ifndef FUSION_RANGING_SESSION_MANAGER_H
#define FUSION_RANGING_SESSION_MANAGER_H

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "ranging_result_observer_stub.h"
#include "refbase.h"

namespace OHOS {
namespace FusionRanging {

class SessionManager {
public:
    using SessionKey = std::string;

    struct RangingSession {
    public:
        SessionKey key;
        int32_t uid;
        std::string deviceId;
        sptr<IRangingResultObserver> resultObserver;

        RangingSession() = default;
        RangingSession(SessionKey k, int32_t u, const std::string &d, const sptr<IRangingResultObserver> &o)
            : key(k),
              uid(u),
              deviceId(d),
              resultObserver(o)
        {
        }
        ~RangingSession() = default;
    };

    SessionManager() = default;
    ~SessionManager() = default;

    bool AddSession(int32_t uid, const std::string &deviceId, const sptr<IRangingResultObserver> &observer);
    void RemoveSession(const SessionKey &key);
    void RemoveSessionByUid(int32_t uid);
    std::vector<SessionKey> GetSessionKeysByDeviceId(const std::string &deviceId);
    std::vector<SessionKey> GetSessionKeysByUid(int32_t uid);
    std::vector<SessionKey> GetAllSessionKeys() const;
    bool HasSession(const SessionKey &key) const;
    bool HasSessionsByUid(int32_t uid) const;
    size_t GetSessionCount() const;
    void ClearAll();

private:
    SessionKey MakeKey(const std::string &deviceId);

    mutable std::mutex mutex_;
    std::map<SessionKey, RangingSession> sessions_;
};

}  // namespace FusionRanging
}  // namespace OHOS

#endif  // FUSION_RANGING_SESSION_MANAGER_H