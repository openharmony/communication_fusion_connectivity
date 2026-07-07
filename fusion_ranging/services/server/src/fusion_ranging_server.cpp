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
#include <mutex>
#include "fusion_ranging_errorcode.h"
#include "fusion_ranging_service.h"
#include "ipc_skeleton.h"
#include "process_death_manager.h"
#include "fcm_thread_util.h"
#include "application_state_observer_stub.h"
#include "system_ability_definition.h"
#include "common_utils.h"
#include "log_utils.h"

namespace OHOS {
namespace FusionRanging {

constexpr int32_t FUSION_RANGING_SYS_ABILITY_ID = 8631;
constexpr int32_t FUSION_RANGING_DELAY_CHECK_SA_UNLOAD_MS = 10000;

sptr<FusionRangingServer> FusionRangingServer::instance_ = nullptr;
std::mutex FusionRangingServer::instanceLock_;

struct FusionRangingServer::impl {
    impl();
    ~impl();

    class AppStateObserver {
    public:
        AppStateObserver() = default;
        bool SubscribeAppState();
        bool UnSubscribeAppState();
        sptr<AppExecFwk::IAppMgr> GetAppMgrProxy();

    private:
        class AppStateAwareObserver : public AppExecFwk::ApplicationStateObserverStub {
        public:
            void OnForegroundApplicationChanged(const AppExecFwk::AppStateData &appStateData) override;

        private:
            inline bool ValidateAppStateData(const AppExecFwk::AppStateData &appStateData);
        };

        std::mutex mutex_{};
        sptr<AppStateAwareObserver> appStateAwareObserver_;

        FCM_DISALLOW_COPY_AND_ASSIGN(AppStateObserver);
    };
    void RegisterAppObserver();
    void DeregisterAppObserver();

    std::shared_ptr<ProcessDeathManager> processDeathManager_;
    SafeMap<int, sptr<IRangingObserver>> observers_{}; /* int: calleruid */
    std::unique_ptr<AppStateObserver> appStateObserverImp_ = nullptr;
};

const bool REGISTER_RESULT = SystemAbility::MakeAndRegisterAbility(FusionRangingServer::GetInstance().GetRefPtr());

FusionRangingServer::impl::impl()
{
    HILOGI("FusionRangingServer impl create");
    processDeathManager_ = std::make_shared<ProcessDeathManager>();
}

FusionRangingServer::impl::~impl()
{
    HILOGI("FusionRangingServer ~impl");
}

sptr<AppExecFwk::IAppMgr> FusionRangingServer::impl::AppStateObserver::GetAppMgrProxy()
{
    sptr<ISystemAbilityManager> systemAbilityManager =
        SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    FCM_CHECK_RETURN_RET(systemAbilityManager != nullptr, nullptr, "systemAbilityManager is nullptr");

    sptr<IRemoteObject> remoteObject = systemAbilityManager->GetSystemAbility(APP_MGR_SERVICE_ID);
    FCM_CHECK_RETURN_RET(remoteObject != nullptr, nullptr, "remote object is nullptr");
    sptr<AppExecFwk::IAppMgr> appMgrProxy = iface_cast<AppExecFwk::IAppMgr>(remoteObject);
    if (!appMgrProxy || !appMgrProxy->AsObject()) {
        HILOGE("get appMgrProxy failed!");
        return nullptr;
    }
    return appMgrProxy;
}

bool FusionRangingServer::impl::AppStateObserver::SubscribeAppState()
{
    std::lock_guard<std::mutex> lock(mutex_);
    FCM_CHECK_RETURN_RET(appStateAwareObserver_ == nullptr, false, "appStateAwareObserver_ has register");
    sptr<AppExecFwk::IAppMgr> appMgrProxy = GetAppMgrProxy();
    FCM_CHECK_RETURN_RET(appMgrProxy != nullptr, false, "appMgrProxy is nullptr");

    appStateAwareObserver_ = new (std::nothrow) AppStateAwareObserver();
    FCM_CHECK_RETURN_RET(appStateAwareObserver_ != nullptr, false, "appStateAwareObserver_ is nullptr");
    auto err = appMgrProxy->RegisterApplicationStateObserver(appStateAwareObserver_);
    if (err != 0) {
        HILOGE("error, code = %{public}d", err);
        appStateAwareObserver_ = nullptr;
        return false;
    }
    return true;
}

bool FusionRangingServer::impl::AppStateObserver::UnSubscribeAppState()
{
    std::lock_guard<std::mutex> lock(mutex_);
    FCM_CHECK_RETURN_RET(appStateAwareObserver_ != nullptr, false, "appStateAwareObserver_ is nullptr");
    sptr<AppExecFwk::IAppMgr> appMgrProxy = GetAppMgrProxy();
    if (appMgrProxy) {
        appMgrProxy->UnregisterApplicationStateObserver(appStateAwareObserver_);
        appMgrProxy = nullptr;
        appStateAwareObserver_ = nullptr;
    }
    return true;
}

void FusionRangingServer::impl::AppStateObserver::AppStateAwareObserver::OnForegroundApplicationChanged(
    const AppExecFwk::AppStateData &appStateData)
{
    if (!ValidateAppStateData(appStateData)) {
        HILOGI("validate app state data failed");
        return;
    }
    FusionConnectivity::DoInRangingThread(
        [appStateData]() { FusionRangingServer::GetInstance()->OnForegroundApplicationChanged(appStateData); });
}

inline bool FusionRangingServer::impl::AppStateObserver::AppStateAwareObserver::ValidateAppStateData(
    const AppExecFwk::AppStateData &appStateData)
{
    return appStateData.uid > 0 && appStateData.bundleName.length() > 0;
}

void FusionRangingServer::impl::RegisterAppObserver()
{
    appStateObserverImp_ = std::make_unique<FusionRangingServer::impl::AppStateObserver>();
    if (appStateObserverImp_ != nullptr) {
        auto ret = appStateObserverImp_->SubscribeAppState();
        HILOGI("FusionRangingServer SubscribeAppState ret:%{public}d", ret);
    }
}

void FusionRangingServer::impl::DeregisterAppObserver()
{
    if (appStateObserverImp_ == nullptr) {
        return;
    }
    HILOGI("FusionRangingServer UnSubscribeAppState");
    appStateObserverImp_->UnSubscribeAppState();
}

sptr<FusionRangingServer> FusionRangingServer::GetInstance()
{
    std::lock_guard<std::mutex> lock(instanceLock_);
    if (instance_ == nullptr) {
        sptr<FusionRangingServer> temp = new (std::nothrow) FusionRangingServer();
        instance_ = temp;
    }
    return instance_;
}

FusionRangingServer::FusionRangingServer() : SystemAbility(FUSION_RANGING_SYS_ABILITY_ID, true), isStopping_(false)
{
    HILOGI("FusionRangingServer create");
    pimpl = std::make_unique<impl>();
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
         FusionConnectivity::PermissionItem(FusionConnectivity::PUBLIC_API, PERMISSION_ACCESS_NEARLINK)},
        {static_cast<int>(IFusionRangingIpcCode::COMMAND_START_RANGING),
         FusionConnectivity::PermissionItem(FusionConnectivity::PUBLIC_API, PERMISSION_ACCESS_NEARLINK)},
        {static_cast<int>(IFusionRangingIpcCode::COMMAND_STOP_RANGING),
         FusionConnectivity::PermissionItem(FusionConnectivity::PUBLIC_API, PERMISSION_ACCESS_NEARLINK)},
        {static_cast<int>(IFusionRangingIpcCode::COMMAND_START_PASSIVE_RANGING),
         FusionConnectivity::PermissionItem(FusionConnectivity::PUBLIC_API, PERMISSION_ACCESS_NEARLINK)},
        {static_cast<int>(IFusionRangingIpcCode::COMMAND_STOP_PASSIVE_RANGING),
         FusionConnectivity::PermissionItem(FusionConnectivity::PUBLIC_API, PERMISSION_ACCESS_NEARLINK)},
        {static_cast<int>(IFusionRangingIpcCode::COMMAND_REGISTER_OBSERVER),
         FusionConnectivity::PermissionItem(FusionConnectivity::PUBLIC_API, PERMISSION_ACCESS_NEARLINK)},
        {static_cast<int>(IFusionRangingIpcCode::COMMAND_DEREGISTER_OBSERVER),
         FusionConnectivity::PermissionItem(FusionConnectivity::PUBLIC_API, PERMISSION_ACCESS_NEARLINK)},
    };
}

int32_t FusionRangingServer::CallbackEnter(uint32_t code)
{
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
    FusionConnectivity::DoInRangingThread([this]() { pimpl->RegisterAppObserver(); });
}

void FusionRangingServer::OnStop()
{
    if (isStopping_.load()) {
        HILOGW("OnStop: already stopping, skip");
        return;
    }
    isStopping_ = true;
    FCM_CHECK_RETURN(pimpl != nullptr, "pimpl is nullptr");
    HILOGI("FusionRangingServer stopping service.");
    if (pimpl->processDeathManager_) {
        pimpl->processDeathManager_->ClearAll();
    }
    pimpl->DeregisterAppObserver();
}

ErrCode FusionRangingServer::GetRangingCapability(RangingCapabilitySupported &capability)
{
    auto isSupport = FusionRangingService::GetInstance()->IsRangingSupported(RangingTypes::NEARLINK_HADM);
    HILOGI("GetRangingCapability isSupport:%{public}d", isSupport);
    capability.SetNearlinkHadm(isSupport);
    FusionConnectivity::DoInRangingThread([this]() { CheckAndUnloadIfIdle(); },
                                          FUSION_RANGING_DELAY_CHECK_SA_UNLOAD_MS);
    return static_cast<int32_t>(RangingErrCode::RANGING_NO_ERROR);
}

ErrCode FusionRangingServer::StartRanging(const RangingParams &params)
{
    HILOGI("StartRanging server.");
    sptr<IRangingObserver> observer = nullptr;
    int32_t callerUid = IPCSkeleton::GetCallingUid();
    FCM_CHECK_RETURN_RET(pimpl != nullptr, RANGING_ERR_OPERATION_FAILED, "pimpl is nullptr");
    auto findRet = pimpl->observers_.Find(callerUid, observer);
    FCM_CHECK_RETURN_RET(findRet && observer != nullptr, RANGING_ERR_OPERATION_FAILED, "observer not found");

    int32_t ret = FusionRangingService::GetInstance()->StartRanging(params, observer, callerUid);
    HILOGI("StartRanging: ret=%{public}d", ret);
    if (ret == RANGING_ERR_DEVICE_ALREADY_INITIATED) {
        return ret;
    }
    if (ret != 0) {
        FusionRangingService::GetInstance()->StopRanging(params.GetDeviceId(), callerUid);
        return ret;
    }
    return RANGING_NO_ERROR;
}

ErrCode FusionRangingServer::StopRanging(const RangingParams &params)
{
    HILOGI("StopRanging server, deviceId=%{public}s", GET_ENCRYPT_ADDR(params.GetDeviceId()));
    FCM_CHECK_RETURN_RET(IsValidAddress(params.GetDeviceId()), RANGING_ERR_PARAM_NOT_MEET_SPECIFICATIONS,
                         "device invalid");
    int32_t ret = FusionRangingService::GetInstance()->StopRanging(params.GetDeviceId(), IPCSkeleton::GetCallingUid());
    HILOGI("StopRanging: ret=%{public}d", ret);
    FusionConnectivity::DoInRangingThread([this]() { CheckAndUnloadIfIdle(); },
                                          FUSION_RANGING_DELAY_CHECK_SA_UNLOAD_MS);
    return ret;
}

ErrCode FusionRangingServer::StartPassiveRanging(int32_t capabilityType, int32_t &handle)
{
    sptr<IRangingObserver> observer = nullptr;
    int32_t callerUid = IPCSkeleton::GetCallingUid();
    FCM_CHECK_RETURN_RET(pimpl != nullptr, RANGING_ERR_OPERATION_FAILED, "pimpl is nullptr");
    auto findRet = pimpl->observers_.Find(callerUid, observer);
    FCM_CHECK_RETURN_RET(findRet && observer != nullptr, RANGING_ERR_OPERATION_FAILED, "observer not found");
    int32_t ret = FusionRangingService::GetInstance()->StartPassiveRanging(static_cast<RangingTypes>(capabilityType),
                                                                           handle, observer, callerUid);
    HILOGI("StartPassiveRanging handle:%{public}d, uid:%{public}d", handle, callerUid);
    return ret;
}

ErrCode FusionRangingServer::StopPassiveRanging(int32_t capabilityType, int32_t handle)
{
    FCM_CHECK_RETURN_RET(handle >= 0, RANGING_ERR_PARAM_NOT_MEET_SPECIFICATIONS, "handle invalid");
    int32_t ret = FusionRangingService::GetInstance()->StopPassiveRanging(static_cast<RangingTypes>(capabilityType),
                                                                          handle, IPCSkeleton::GetCallingUid());
    HILOGI("StopPassiveRanging ret=%{public}d", ret);
    return ret;
}

ErrCode FusionRangingServer::RegisterObserver(const sptr<IRangingObserver> &observer)
{
    int32_t callerUid = IPCSkeleton::GetCallingUid();
    bool isExist = false;
    FCM_CHECK_RETURN_RET(pimpl != nullptr, RANGING_ERR_OPERATION_FAILED, "pimpl is nullptr");
    pimpl->observers_.Iterate([&](int32_t uid, const sptr<IRangingObserver> &value) {
        if (observer == value && uid == callerUid) {
            isExist = true;
        }
    });
    FCM_CHECK_RETURN_RET(!isExist, RANGING_ERR_OPERATION_FAILED, "observer is already exist");
    pimpl->observers_.EnsureInsert(callerUid, observer);
    sptr<IRemoteObject> remoteObj = observer->AsObject();
    if (remoteObj != nullptr && pimpl->processDeathManager_ != nullptr) {
        pimpl->processDeathManager_->RegisterProcessDeath(callerUid, remoteObj);
        HILOGI("RegisterObserver: registered process death handler for uid=%{public}d", callerUid);
    }
    return RANGING_NO_ERROR;
}

ErrCode FusionRangingServer::DeregisterObserver(const sptr<IRangingObserver> &observer)
{
    int32_t callerUid = IPCSkeleton::GetCallingUid();
    bool isExist = false;
    FCM_CHECK_RETURN_RET(pimpl != nullptr, RANGING_ERR_OPERATION_FAILED, "pimpl is nullptr");
    pimpl->observers_.Iterate([&](int32_t uid, const sptr<IRangingObserver> &value) {
        if (observer == value && uid == callerUid) {
            isExist = true;
        }
    });
    FCM_CHECK_RETURN_RET(isExist, RANGING_ERR_OPERATION_FAILED, "observer not exist");
    FCM_CHECK_RETURN_RET(pimpl->processDeathManager_ != nullptr, RANGING_ERR_OPERATION_FAILED,
                         "processDeathManager_ is nullptr");
    pimpl->processDeathManager_->DeregisterProcessDeath(callerUid);
    pimpl->observers_.Erase(callerUid);
    FusionConnectivity::DoInRangingThread([this]() { CheckAndUnloadIfIdle(); },
                                          FUSION_RANGING_DELAY_CHECK_SA_UNLOAD_MS);
    return RANGING_NO_ERROR;
}

void FusionRangingServer::HandleProcessDeath(int32_t uid)
{
    HILOGI("HandleProcessDeath: uid=%{public}d", uid);
    FCM_CHECK_RETURN(pimpl != nullptr, "pimpl is nullptr");
    if (pimpl->processDeathManager_) {
        pimpl->processDeathManager_->DeregisterProcessDeath(uid);
    }
    pimpl->observers_.Erase(uid);
    FusionRangingService::GetInstance()->HandleProcessDeath(uid);
    FusionConnectivity::DoInRangingThread([this]() { CheckAndUnloadIfIdle(); },
                                          FUSION_RANGING_DELAY_CHECK_SA_UNLOAD_MS);
}

void FusionRangingServer::OnForegroundApplicationChanged(const AppExecFwk::AppStateData &appStateData)
{
    int32_t uid = appStateData.uid;
    HILOGI("OnForegroundApplicationChanged uid:%{public}d, state:%{public}d", uid, appStateData.state);
    if (appStateData.state == static_cast<int32_t>(AppExecFwk::ApplicationState::APP_STATE_FOREGROUND)) {
        FusionRangingService::GetInstance()->ResumeRangingByUid(uid);
    } else if (appStateData.state == static_cast<int32_t>(AppExecFwk::ApplicationState::APP_STATE_BACKGROUND)) {
        FusionRangingService::GetInstance()->PauseRangingByUid(uid);
    } else if (appStateData.state == static_cast<int32_t>(AppExecFwk::ApplicationState::APP_STATE_TERMINATED)) {
        HandleProcessDeath(uid);
        HILOGI("APP_STATE_TERMINATED: completed for uid=%{public}d", uid);
    }
}

void FusionRangingServer::CheckAndUnloadIfIdle()
{
    auto ret = FusionRangingService::GetInstance()->IsRangingEmpty();
    FCM_CHECK_RETURN(pimpl != nullptr, "pimpl is nullptr");
    if (ret && pimpl->observers_.IsEmpty()) {
        HILOGI("CheckAndUnloadIfIdle: no active sessions or state observers, stopping SA");
        if (pimpl->processDeathManager_) {
            pimpl->processDeathManager_->ClearAll();
        }
        FCM_CHECK_RETURN(pimpl->appStateObserverImp_ != nullptr, "appStateObserverImp_ is nullptr");
        pimpl->appStateObserverImp_->UnSubscribeAppState();
        CheckAndUnloadSA();
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