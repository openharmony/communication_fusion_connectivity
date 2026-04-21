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
#define LOG_TAG "FusionRangingServer"
#endif

#include "fusion_ranging_server.h"

#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <vector>

#include "app_state_observer.h"
#include "fusion_ranging_errorcode.h"
#include "fusion_ranging_service.h"
#include "ipc_skeleton.h"
#include "log_util.h"
#include "process_death_manager.h"
#include "session_manager.h"
#include "state_observer_registry.h"

namespace OHOS {
namespace FusionRanging {

constexpr int32_t FUSION_RANGING_SYS_ABILITY_ID = 8631;

const bool REGISTER_RESULT = SystemAbility::MakeAndRegisterAbility(FusionRangingServer::GetInstance().GetRefPtr());

sptr<FusionRangingServer> FusionRangingServer::GetInstance()
{
    static std::mutex instanceMutex;
    std::lock_guard<std::mutex> lock(instanceMutex);
    static FusionRangingServer *instance = nullptr;
    if (instance == nullptr) {
        instance = new (std::nothrow) FusionRangingServer();
    }
    return instance;
}

FusionRangingServer::FusionRangingServer() : SystemAbility(FUSION_RANGING_SYS_ABILITY_ID, true), isStopping_(false)
{
    sessionManager_ = std::make_unique<SessionManager>();
    stateObserverRegistry_ = std::make_unique<StateObserverRegistry>();
    processDeathManager_ = std::make_unique<ProcessDeathManager>();
    appStateObserver_ = std::make_unique<RangingAppStateObserver>();

    processDeathManager_->SetDeathCallback([this](int32_t uid) { HandleProcessDeath(uid); });
    appStateObserver_->SetServer(this);
}

FusionRangingServer::~FusionRangingServer()
{
    HILOGI("FusionRangingServer destroyed.");
}

void FusionRangingServer::OnStart()
{
    HILOGI("FusionRangingServer starting service.");

    bool res = Publish(this);
    HILOGI("Publish result is %{public}d.", res);

    if (appStateObserver_) {
        appStateObserver_->SubscribeAppState();
    }
}

void FusionRangingServer::OnStop()
{
    bool expected = false;
    if (!isStopping_.compare_exchange_strong(expected, true)) {
        HILOGW("OnStop: already stopping, skip");
        return;
    }

    HILOGI("FusionRangingServer stopping service.");

    if (appStateObserver_) {
        appStateObserver_->UnSubscribeAppState();
    }

    sessionManager_->ClearAll();
    stateObserverRegistry_->ClearAll();
    processDeathManager_->ClearAll();
}

ErrCode FusionRangingServer::GetRangingCapability(RangingCapabilitySupported &capability)
{
    HILOGI("GetRangingCapability server.");
    capability.SetNearlinkHadm(true);
    return static_cast<int32_t>(RangingErrCode::RANGING_NO_ERROR);
}

ErrCode FusionRangingServer::StartRanging(const RangingParams &params, const sptr<IRangingResultObserver> &observer)
{
    HILOGI("StartRanging server.");

    auto ranging = FusionRangingService::GetInstance();
    if (!ranging) {
        HILOGE("StartRanging: ranging service is null");
        return static_cast<int32_t>(RangingErrCode::RANGING_ERR_INTERNAL_ERROR);
    }

    if (!observer) {
        HILOGE("StartRanging: observer is null");
        return static_cast<int32_t>(RangingErrCode::RANGING_ERR_INVALID_PARAM);
    }

    auto callback = [observer](const RangingResult &result) {
        if (observer != nullptr) {
            observer->OnRangingResult(result);
        }
    };

    int32_t ret = ranging->StartRanging(params, callback);
    HILOGI("StartRanging: ret=%{public}d", ret);
    if (ret != 0) {
        return ret;
    }

    int32_t callerUid = IPCSkeleton::GetCallingUid();
    std::string deviceId = params.GetDeviceId();
    if (callerUid != 0 && !deviceId.empty()) {
        if (!sessionManager_->AddSession(callerUid, deviceId, observer)) {
            HILOGW("StartRanging: failed to add session");
            ranging->StopRanging(deviceId);
            return static_cast<int32_t>(RangingErrCode::RANGING_ERR_INTERNAL_ERROR);
        }

        sptr<IRemoteObject> remoteObj = observer->AsObject();
        if (remoteObj != nullptr && !processDeathManager_->HasProcessDeathHandler(callerUid)) {
            processDeathManager_->RegisterProcessDeath(callerUid, remoteObj);
            HILOGI("StartRanging: registered process death handler for uid=%{public}d", callerUid);
        }
        HILOGI("StartRanging: success, uid=%{public}d, deviceId=%{public}s", callerUid, GET_ENCRYPT_ADDR(deviceId));
    }

    return static_cast<int32_t>(RangingErrCode::RANGING_NO_ERROR);
}

ErrCode FusionRangingServer::StopRanging(const std::string &deviceId, const sptr<IRangingResultObserver> &observer)
{
    HILOGI("StopRanging server, deviceId=%{public}s", GET_ENCRYPT_ADDR(deviceId));

    auto ranging = FusionRangingService::GetInstance();
    if (!ranging) {
        HILOGE("StopRanging: ranging service is null");
        return static_cast<int32_t>(RangingErrCode::RANGING_ERR_INTERNAL_ERROR);
    }

    int32_t ret = ranging->StopRanging(deviceId);
    HILOGI("StopRanging: ret=%{public}d", ret);

    sessionManager_->RemoveSession(deviceId);

    CheckAndUnloadIfIdle();
    return ret;
}

ErrCode FusionRangingServer::RegisterStateObserver(const sptr<IRangingStateObserver> &observer)
{
    if (!observer) {
        HILOGE("RegisterStateObserver: observer is null");
        return static_cast<int32_t>(RangingErrCode::RANGING_ERR_INVALID_PARAM);
    }

    int32_t callerUid = IPCSkeleton::GetCallingUid();

    bool ret = stateObserverRegistry_->Register(callerUid, observer);
    if (ret) {
        sptr<IRemoteObject> remoteObj = observer->AsObject();
        if (remoteObj != nullptr && !processDeathManager_->HasProcessDeathHandler(callerUid)) {
            processDeathManager_->RegisterProcessDeath(callerUid, remoteObj);
            HILOGI("RegisterStateObserver: registered process death handler for uid=%{public}d", callerUid);
        }
        HILOGI("RegisterStateObserver: success, uid=%{public}d", callerUid);
    }

    return ret ? static_cast<int32_t>(RangingErrCode::RANGING_NO_ERROR) :
                 static_cast<int32_t>(RangingErrCode::RANGING_ERR_INTERNAL_ERROR);
}

ErrCode FusionRangingServer::UnregisterStateObserver(const sptr<IRangingStateObserver> &observer)
{
    if (!observer) {
        HILOGE("UnregisterStateObserver: observer is null");
        return static_cast<int32_t>(RangingErrCode::RANGING_ERR_INVALID_PARAM);
    }

    int32_t callerUid = IPCSkeleton::GetCallingUid();

    bool ret = stateObserverRegistry_->Unregister(callerUid);
    HILOGI("UnregisterStateObserver: uid=%{public}d, result=%{public}d", callerUid, ret);

    CheckAndUnloadIfIdle();
    return ret ? static_cast<int32_t>(RangingErrCode::RANGING_NO_ERROR) :
                 static_cast<int32_t>(RangingErrCode::RANGING_ERR_INTERNAL_ERROR);
}

void FusionRangingServer::NotifyStateObservers(int32_t uid, const RangingStateChangeInfo &info)
{
    auto observer = stateObserverRegistry_->Find(uid);
    if (observer != nullptr) {
        observer->OnRangingStateChanged(info);
        HILOGI("NotifyStateObservers: notified uid=%{public}d", uid);
    }
}

void FusionRangingServer::StopAllDeviceIdsByUid(int32_t uid)
{
    auto ranging = FusionRangingService::GetInstance();
    if (!ranging) {
        HILOGE("StopAllDeviceIdsByUid: ranging service is null");
        return;
    }

    auto sessionKeys = sessionManager_->GetSessionKeysByUid(uid);
    for (const auto &key : sessionKeys) {
        ranging->StopRanging(key);
        HILOGI("StopAllDeviceIdsByUid: stop deviceId=%{public}s for uid=%{public}d", GET_ENCRYPT_ADDR(key), uid);
    }
}

void FusionRangingServer::HandleProcessDeath(int32_t uid)
{
    HILOGI("HandleProcessDeath: uid=%{public}d, cleaning resources", uid);

    StopAllDeviceIdsByUid(uid);
    sessionManager_->RemoveSessionByUid(uid);
    stateObserverRegistry_->Unregister(uid);
    processDeathManager_->UnregisterProcessDeath(uid);

    CheckAndUnloadIfIdle();
}

void FusionRangingServer::PauseRangingByUid(int32_t uid)
{
    HILOGI("PauseRangingByUid: pause for uid=%{public}d", uid);
    auto ranging = FusionRangingService::GetInstance();
    if (!ranging) {
        HILOGE("PauseRangingByUid: ranging service is null");
        return;
    }

    auto sessionKeys = sessionManager_->GetSessionKeysByUid(uid);
    HILOGI("PauseRangingByUid: keys size=%{public}zu", sessionKeys.size());
    for (const auto &key : sessionKeys) {
        ranging->PauseRanging(key);

        RangingStateChangeInfo info;
        info.SetState(RangingState::STATE_STOPPED);
        info.SetCause(RangingStoppedCause::NO_ERROR);
        NotifyStateObservers(uid, info);
        HILOGI("PauseRangingByUid: paused deviceId=%{public}s for uid=%{public}d", GET_ENCRYPT_ADDR(key), uid);
    }
}

void FusionRangingServer::ResumeRangingByUid(int32_t uid)
{
    auto ranging = FusionRangingService::GetInstance();
    if (!ranging) {
        HILOGE("ResumeRangingByUid: ranging service is null");
        return;
    }

    auto sessionKeys = sessionManager_->GetSessionKeysByUid(uid);
    for (const auto &key : sessionKeys) {
        ranging->ResumeRanging(key);

        RangingStateChangeInfo info;
        info.SetState(RangingState::STATE_STARTED);
        info.SetCause(RangingStoppedCause::NO_ERROR);
        NotifyStateObservers(uid, info);
        HILOGI("ResumeRangingByUid: resume deviceId=%{public}s for uid=%{public}d", GET_ENCRYPT_ADDR(key), uid);
    }
}

void FusionRangingServer::StopRangingByUid(int32_t uid)
{
    StopAllDeviceIdsByUid(uid);
    sessionManager_->RemoveSessionByUid(uid);

    CheckAndUnloadIfIdle();

    HILOGI("StopRangingByUid: completed for uid=%{public}d", uid);
}

void FusionRangingServer::CleanupAll()
{
    HILOGI("CleanupAll: start cleanup");

    auto sessionKeys = sessionManager_->GetAllSessionKeys();
    for (const auto &key : sessionKeys) {
        auto ranging = FusionRangingService::GetInstance();
        if (ranging) {
            ranging->StopRanging(key);
            HILOGI("CleanupAll: stop deviceId=%{public}s", GET_ENCRYPT_ADDR(key));
        }
    }

    sessionManager_->ClearAll();
    stateObserverRegistry_->ClearAll();
    processDeathManager_->ClearAll();

    HILOGI("CleanupAll: cleanup completed");
}

void FusionRangingServer::CheckAndUnloadIfIdle()
{
    bool expected = false;
    if (!isStopping_.compare_exchange_strong(expected, true)) {
        HILOGW("CheckAndUnloadIfIdle: already stopping, skip");
        return;
    }

    bool hasActiveSessions = sessionManager_->GetSessionCount() > 0;
    bool hasStateObserver = stateObserverRegistry_->GetObserverCount() > 0;
    if (!hasActiveSessions && !hasStateObserver) {
        HILOGI("CheckAndUnloadIfIdle: no active sessions or state observers, stopping SA");
        if (appStateObserver_) {
            appStateObserver_->UnSubscribeAppState();
        }
        sessionManager_->ClearAll();
        stateObserverRegistry_->ClearAll();
        processDeathManager_->ClearAll();
    } else {
        bool current = true;
        isStopping_.compare_exchange_strong(current, false);
    }
}

void FusionRangingServer::CheckAndUnloadSA()
{
    CheckAndUnloadIfIdle();
}

}  // namespace FusionRanging
}  // namespace OHOS