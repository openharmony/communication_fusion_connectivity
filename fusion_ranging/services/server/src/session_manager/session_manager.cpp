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

#ifndef LOG_TAG
#define LOG_TAG "SessionManager"
#endif

#include "session_manager.h"
#include <algorithm>
#include "log_util.h"

namespace OHOS {
namespace FusionRanging {

SessionManager::SessionKey SessionManager::MakeKey(const std::string &deviceId)
{
    return deviceId;
}

bool SessionManager::AddSession(int32_t uid, const std::string &deviceId, const sptr<IRangingResultObserver> &observer)
{
    HILOGI("AddSession: uid:%{public}d, deviceId=%{public}s", uid, GET_ENCRYPT_ADDR(deviceId));
    if (deviceId.empty() || observer == nullptr) {
        HILOGE("AddSession: invalid parameter, deviceId=%{public}s", GET_ENCRYPT_ADDR(deviceId));
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    SessionKey key = MakeKey(deviceId);
    if (sessions_.find(key) != sessions_.end()) {
        HILOGW("AddSession: session already exists, key=%{public}s", key.c_str());
        return false;
    }

    RangingSession session;
    session.key = key;
    session.uid = uid;
    session.deviceId = deviceId;
    session.resultObserver = observer;

    sessions_[key] = session;
    HILOGI("AddSession: success, key=%{public}s, uid=%{public}d", key.c_str(), uid);
    return true;
}

void SessionManager::RemoveSession(const SessionKey &key)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(key);
    if (it != sessions_.end()) {
        HILOGI("RemoveSession: remove session, key=%{public}s", key.c_str());
        sessions_.erase(it);
    }
}

void SessionManager::RemoveSessionByUid(int32_t uid)
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<SessionKey> keysToRemove;
    for (const auto &pair : sessions_) {
        if (pair.second.uid == uid) {
            keysToRemove.push_back(pair.first);
        }
    }
    for (const auto &key : keysToRemove) {
        HILOGI("RemoveSessionByUid: remove session, key=%{public}s", key.c_str());
        sessions_.erase(key);
    }
}

std::vector<SessionManager::SessionKey> SessionManager::GetSessionKeysByDeviceId(const std::string &deviceId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<SessionKey> keys;
    for (const auto &pair : sessions_) {
        if (pair.second.deviceId == deviceId) {
            keys.push_back(pair.first);
        }
    }
    HILOGI("GetSessionKeysByDeviceId: keysSize=%{public}zu", keys.size());
    return keys;
}

std::vector<SessionManager::SessionKey> SessionManager::GetSessionKeysByUid(int32_t uid)
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<SessionKey> keys;
    for (const auto &pair : sessions_) {
        HILOGD("GetSessionKeysByUid: uid:%{public}d, key=%{public}s", uid, pair.second.key.c_str());
        if (pair.second.uid == uid) {
            keys.push_back(pair.first);
        }
    }
    HILOGI("GetSessionKeysByUid: uid:%{public}d, keysSize=%{public}zu", uid, keys.size());
    return keys;
}

std::vector<SessionManager::SessionKey> SessionManager::GetAllSessionKeys() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<SessionKey> keys;
    for (const auto &pair : sessions_) {
        keys.push_back(pair.first);
    }
    HILOGI("GetAllSessionKeys: keysSize=%{public}zu", keys.size());
    return keys;
}

bool SessionManager::HasSession(const SessionKey &key) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return sessions_.find(key) != sessions_.end();
}

bool SessionManager::HasSessionsByUid(int32_t uid) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto &pair : sessions_) {
        if (pair.second.uid == uid) {
            return true;
        }
    }
    return false;
}

size_t SessionManager::GetSessionCount() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return sessions_.size();
}

void SessionManager::ClearAll()
{
    std::lock_guard<std::mutex> lock(mutex_);
    HILOGI("ClearAll: clear all sessions, count=%{public}zu", sessions_.size());
    sessions_.clear();
}

}  // namespace FusionRanging
}  // namespace OHOS