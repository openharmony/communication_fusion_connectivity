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
#include "log_utils.h"
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
    static sptr<FusionRangingServer> instance = nullptr;
    if (instance == nullptr) {
        instance = new (std::nothrow) FusionRangingServer();
        if (instance == nullptr) {
            HILOGI("FusionRangingServer instance create fail.");
        }
    }
    return instance;
}

FusionRangingServer::FusionRangingServer() : SystemAbility(FUSION_RANGING_SYS_ABILITY_ID, true), isStopping_(false)
{
    sessionManager_ = std::make_shared<SessionManager>();
    stateObserverRegistry_ = std::make_shared<StateObserverRegistry>();
    processDeathManager_ = std::make_shared<ProcessDeathManager>();
    appStateObserver_ = std::make_shared<RangingAppStateObserver>();
    if (processDeathManager_) {
        processDeathManager_->SetDeathCallback([this](int32_t uid) { HandleProcessDeath(uid); });
    }
    InitializePermissionsMap();
}

FusionRangingServer::~FusionRangingServer()
{
    HILOGI("FusionRangingServer destroyed.");
}

void FusionRangingServer::InitializePermissionsMap()
{
    constexpr const char *PERMISSION_ACCESS_NEARLINK = "ohos.permission.ACCESS_NEARLINK";
    permissionsMap_ = {
        {static_cast<int>(IFusionRangingIpcCode::COMMAND_GET_RANGING_CAPABILITY),
         FusionConnectivity::PermissionItem(FusionConnectivity::PUBLIC_API, std::set<std::string>{})},
        {static_cast<int>(IFusionRangingIpcCode::COMMAND_START_RANGING),
         FusionConnectivity::PermissionItem(FusionConnectivity::PUBLIC_API, PERMISSION_ACCESS_NEARLINK)},
        {static_cast<int>(IFusionRangingIpcCode::COMMAND_STOP_RANGING),
         FusionConnectivity::PermissionItem(FusionConnectivity::PUBLIC_API, PERMISSION_ACCESS_NEARLINK)},
        {static_cast<int>(IFusionRangingIpcCode::COMMAND_REGISTER_STATE_OBSERVER),
         FusionConnectivity::PermissionItem(FusionConnectivity::PUBLIC_API, PERMISSION_ACCESS_NEARLINK)},
        {static_cast<int>(IFusionRangingIpcCode::COMMAND_UNREGISTER_STATE_OBSERVER),
         FusionConnectivity::PermissionItem(FusionConnectivity::PUBLIC_API, PERMISSION_ACCESS_NEARLINK)},
    };
}

int32_t FusionRangingServer::CallbackEnter(uint32_t code)
{
    HILOGD("FusionRangingServer CallbackEnter ipc code: %{public}u", code);
    auto it = permissionsMap_.find(static_cast<int>(code));
    if (it == permissionsMap_.end()) {
        HILOGE("Unknown ipc code: %{public}u", code);
        return static_cast<int32_t>(RangingErrCode::RANGING_ERR_API_NOT_SUPPORT);
    }

    auto &item = it->second;
    FusionConnectivity::FcmErrCode result = FusionConnectivity::PermissionManager::VerifyPermissions(item);
    if (result != FusionConnectivity::FcmErrCode::FCM_NO_ERROR) {
        HILOGE("Permission check failed for code: %{public}u, result: %{public}d", code, result);
        return static_cast<int32_t>(RangingErrCode::RANGING_ERR_PERMISSION_FAILED);
    }
    return RANGING_NO_ERROR;
}

void FusionRangingServer::OnStart()
{
    HILOGI("FusionRangingServer starting service.");

    bool res = Publish(this);
    HILOGI("Publish result is %{public}d.", res);

    if (appStateObserver_) {
        appStateObserver_->SetServer(this);
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
        appStateObserver_.reset();
    }

    if (sessionManager_) {
        sessionManager_->ClearAll();
    }
    if (stateObserverRegistry_) {
        stateObserverRegistry_->ClearAll();
    }
    if (processDeathManager_) {
        processDeathManager_->ClearAll();
    }
}

ErrCode FusionRangingServer::GetRangingCapability(RangingCapabilitySupported &capability)
{
    auto isSupport = FusionRangingService::GetInstance()->IsRangingSupported(RangingTypes::NEARLINK_HADM);
    HILOGI("GetRangingCapability isSupport:%{public}d", isSupport);
    capability.SetNearlinkHadm(isSupport);
    return static_cast<int32_t>(RangingErrCode::RANGING_NO_ERROR);
}

ErrCode FusionRangingServer::StartRanging(const RangingParams &params, const sptr<IRangingResultObserver> &observer)
{
    HILOGI("StartRanging server.");
    if (!observer) {
        HILOGE("StartRanging: observer is null");
        return static_cast<int32_t>(RangingErrCode::RANGING_ERR_INVALID_PARAM);
    }
    std::string deviceId = params.GetDeviceId();
    auto callback = [observer](const RangingResult &result) {
        if (observer != nullptr) {
            observer->OnRangingResult(result);
        }
    };
    int32_t ret = FusionRangingService::GetInstance()->StartRanging(params, callback);
    HILOGI("StartRanging: ret=%{public}d", ret);
    if (ret != 0) {
        return ret;
    }

    int32_t callerUid = IPCSkeleton::GetCallingUid();
    if (callerUid != 0 && !deviceId.empty()) {
        FCM_CHECK_RETURN_RET(sessionManager_, RANGING_ERR_INTERNAL_ERROR, "sessionManager_ nullptr");
        if (!sessionManager_->AddSession(callerUid, deviceId, observer)) {
            HILOGW("StartRanging: failed to add session");
            FusionRangingService::GetInstance()->StopRanging(deviceId);
            return static_cast<int32_t>(RangingErrCode::RANGING_ERR_INTERNAL_ERROR);
        }

        sptr<IRemoteObject> remoteObj = observer->AsObject();
        if (remoteObj != nullptr && !processDeathManager_->HasProcessDeathHandler(callerUid)) {
            processDeathManager_->RegisterProcessDeath(callerUid, remoteObj);
            HILOGI("StartRanging: registered process death handler for uid=%{public}d", callerUid);
        }
        HILOGI("StartRanging: success, uid=%{public}d, deviceId=%{public}s", callerUid, GET_ENCRYPT_ADDR(deviceId));
    }
    RangingStateChangeInfo info;
    info.SetState(RangingState::STATE_STARTED);
    info.SetCause(RangingStoppedCause::NO_ERROR);
    NotifyStateObservers(callerUid, info);
    return static_cast<int32_t>(RangingErrCode::RANGING_NO_ERROR);
}

ErrCode FusionRangingServer::StopRanging(const std::string &deviceId, const sptr<IRangingResultObserver> &observer)
{
    HILOGI("StopRanging server, deviceId=%{public}s", GET_ENCRYPT_ADDR(deviceId));
    int32_t ret = FusionRangingService::GetInstance()->StopRanging(deviceId);
    if (ret != RANGING_NO_ERROR) {
        HILOGE("StopRanging: ret=%{public}d", ret);
    }
    FCM_CHECK_RETURN_RET(sessionManager_, RANGING_ERR_INTERNAL_ERROR, "sessionManager_ nullptr");
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
    FCM_CHECK_RETURN(sessionManager_, "sessionManager_ nullptr");
    auto sessions = sessionManager_->GetSessionKeysByUid(uid);
    for (const auto &session : sessions) {
        FusionRangingService::GetInstance()->StopRanging(session);
    }
}

void FusionRangingServer::HandleProcessDeath(int32_t uid)
{
    HILOGI("HandleProcessDeath: uid=%{public}d", uid);
    if (!sessionManager_->HasSessionsByUid(uid)) {
        HILOGI("HandleProcessDeath not target uid");
        return;
    }
    StopAllDeviceIdsByUid(uid);
    if (sessionManager_) {
        sessionManager_->RemoveSessionByUid(uid);
    }
    if (stateObserverRegistry_) {
        stateObserverRegistry_->Unregister(uid);
    }
    if (processDeathManager_) {
        processDeathManager_->UnregisterProcessDeath(uid);
    }
    CheckAndUnloadIfIdle();
}

void FusionRangingServer::PauseRangingByUid(int32_t uid)
{
    FCM_CHECK_RETURN(sessionManager_, "sessionManager_ nullptr");
    auto sessions = sessionManager_->GetSessionKeysByUid(uid);
    HILOGI("PauseRangingByUid: keys size=%{public}zu", sessions.size());
    for (const auto &key : sessions) {
        FusionRangingService::GetInstance()->PauseRanging(key);
    }
    RangingStateChangeInfo info;
    info.SetState(RangingState::STATE_STOPPED);
    info.SetCause(RangingStoppedCause::BACKGROUND_NOT_ALLOWED);
    NotifyStateObservers(uid, info);
}

void FusionRangingServer::ResumeRangingByUid(int32_t uid)
{
    FCM_CHECK_RETURN(sessionManager_, "sessionManager_ nullptr");
    auto sessions = sessionManager_->GetSessionKeysByUid(uid);
    for (const auto &key : sessions) {
        FusionRangingService::GetInstance()->ResumeRanging(key);
    }
    RangingStateChangeInfo info;
    info.SetState(RangingState::STATE_STARTED);
    info.SetCause(RangingStoppedCause::NO_ERROR);
    NotifyStateObservers(uid, info);
}

void FusionRangingServer::StopRangingByAppTerminate(int32_t uid)
{
    StopAllDeviceIdsByUid(uid);
    if (sessionManager_) {
        sessionManager_->RemoveSessionByUid(uid);
    }
    CheckAndUnloadIfIdle();
    HILOGI("StopRangingByAppTerminate: completed for uid=%{public}d", uid);
}

void FusionRangingServer::CheckAndUnloadIfIdle()
{
    if (isStopping_.load()) {
        HILOGW("CheckAndUnloadIfIdle: already stopping, skip");
        return;
    }
    FCM_CHECK_RETURN(sessionManager_, "sessionManager_ nullptr");
    bool hasActiveSessions = sessionManager_->GetSessionCount() > 0;
    FCM_CHECK_RETURN(stateObserverRegistry_, "sessionManager_ nullptr");
    bool hasStateObserver = stateObserverRegistry_->GetObserverCount() > 0;
    if (!hasActiveSessions && !hasStateObserver) {
        HILOGI("CheckAndUnloadIfIdle: no active sessions or state observers, stopping SA");
        if (appStateObserver_) {
            appStateObserver_->UnSubscribeAppState();
        }
        sessionManager_->ClearAll();
        stateObserverRegistry_->ClearAll();
        if (processDeathManager_) {
            processDeathManager_->ClearAll();
        }
        CheckAndUnloadSA();
    } else {
        bool current = true;
        isStopping_.compare_exchange_strong(current, false);
    }
}

void FusionRangingServer::CheckAndUnloadSA()
{
    HILOGI("CheckAndUnloadSA: start");
    sptr<ISystemAbilityManager> samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgr == nullptr) {
        HILOGE("CheckAndUnloadSA: get system ability manager failed!");
        return;
    }
    int32_t ret = samgr->UnloadSystemAbility(FUSION_RANGING_SYS_ABILITY_ID);
    if (ret != ERR_NONE) {
        HILOGE("CheckAndUnloadSA: Failed to unload system ability, SA Id=%{public}d, ret=%{public}d",
               FUSION_RANGING_SYS_ABILITY_ID, ret);
    } else {
        HILOGI("CheckAndUnloadSA: successfully unloaded SA Id=%{public}d", FUSION_RANGING_SYS_ABILITY_ID);
    }
}
}  // namespace FusionRanging
}  // namespace OHOS