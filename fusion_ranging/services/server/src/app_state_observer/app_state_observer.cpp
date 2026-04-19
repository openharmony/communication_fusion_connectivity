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
#define LOG_TAG "RangingAppStateObserver"
#endif

#include "app_state_observer.h"
#include "fusion_ranging_server.h"

#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "app_mgr_constants.h"
#include "log.h"

#include <vector>

namespace OHOS {
namespace FusionRanging {

void RangingAppStateObserver::SetServer(const sptr<FusionRangingServer> &server)
{
    std::lock_guard<std::mutex> lock(mutex_);
    server_ = server;
}

sptr<FusionRangingServer> RangingAppStateObserver::GetServer() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return server_;
}

sptr<AppExecFwk::IAppMgr> RangingAppStateObserver::GetAppMgrProxy()
{
    auto registry = OHOS::SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (registry == nullptr) {
        HILOGE("GetAppMgrProxy: registry is nullptr");
        return nullptr;
    }
    return iface_cast<AppExecFwk::IAppMgr>(registry->GetSystemAbility(APP_MGR_SERVICE_ID));
}

bool RangingAppStateObserver::SubscribeAppState()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (appStateAwareObserver_) {
        HILOGW("SubscribeAppState: already subscribed");
        return true;
    }

    sptr<AppExecFwk::IAppMgr> appMgrProxy = GetAppMgrProxy();
    if (appMgrProxy == nullptr) {
        HILOGE("SubscribeAppState: appMgrProxy is nullptr");
        return false;
    }

    appStateAwareObserver_ = new (std::nothrow) AppStateAwareObserver(this);
    if (appStateAwareObserver_ == nullptr) {
        HILOGE("SubscribeAppState: appStateAwareObserver_ is nullptr");
        return false;
    }

    auto err = appMgrProxy->RegisterApplicationStateObserver(appStateAwareObserver_);
    if (err != 0) {
        HILOGE("SubscribeAppState: failed, err=%{public}d", err);
        appStateAwareObserver_ = nullptr;
        return false;
    }

    HILOGI("SubscribeAppState: success");
    return true;
}

bool RangingAppStateObserver::UnSubscribeAppState()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!appStateAwareObserver_) {
        HILOGW("UnSubscribeAppState: not subscribed");
        return true;
    }

    sptr<AppExecFwk::IAppMgr> appMgrProxy = GetAppMgrProxy();
    if (appMgrProxy) {
        appMgrProxy->UnregisterApplicationStateObserver(appStateAwareObserver_);
    }
    appStateAwareObserver_ = nullptr;
    HILOGI("UnSubscribeAppState: success");
    return true;
}

void RangingAppStateObserver::HandleAppForeground(int32_t uid, const std::string &bundleName)
{
    HILOGI("HandleAppForeground: uid=%{public}d, bundleName=%{public}s", uid, bundleName.c_str());
    auto server = GetServer();
    if (server) {
        server->ResumeRangingByUid(uid);
    }
}

void RangingAppStateObserver::HandleAppBackground(int32_t uid, const std::string &bundleName)
{
    HILOGI("HandleAppBackground: uid=%{public}d, bundleName=%{public}s", uid, bundleName.c_str());
    auto server = GetServer();
    if (server) {
        server->PauseRangingByUid(uid);
    }
}

void RangingAppStateObserver::HandleAppTerminate(int32_t uid, const std::string &bundleName)
{
    HILOGI("HandleAppTerminate: uid=%{public}d, bundleName=%{public}s", uid, bundleName.c_str());
    auto server = GetServer();
    if (server) {
        server->StopRangingByUid(uid);
        server->CheckAndUnloadSA();
    }
}

int32_t RangingAppStateObserver::GetUidByBundleName(const std::string &bundleName)
{
    return -1;
}

void RangingAppStateObserver::AppStateAwareObserver::OnForegroundApplicationChanged(
    const AppExecFwk::AppStateData &appStateData)
{
    HILOGI("OnForegroundApplicationChanged: uid=%{public}d, state:%{public}d", appStateData.uid, appStateData.state);
    if (appStateData.uid <= 0 || appStateData.bundleName.empty()) {
        HILOGW("OnForegroundApplicationChanged: invalid data, uid=%{public}d, bundleName=%{public}s", appStateData.uid,
               appStateData.bundleName.c_str());
        return;
    }

    int32_t uid = appStateData.uid;
    std::string bundleName = appStateData.bundleName;

    {
        std::lock_guard<std::mutex> lock(observer_->mutex_);
        observer_->uidToBundleName_[uid] = bundleName;
    }

    if (appStateData.state == static_cast<int32_t>(AppExecFwk::ApplicationState::APP_STATE_FOREGROUND)) {
        observer_->HandleAppForeground(uid, bundleName);
    } else if (appStateData.state == static_cast<int32_t>(AppExecFwk::ApplicationState::APP_STATE_BACKGROUND)) {
        observer_->HandleAppBackground(uid, bundleName);
    } else if (appStateData.state == static_cast<int32_t>(AppExecFwk::ApplicationState::APP_STATE_TERMINATED)) {
        observer_->HandleAppTerminate(uid, bundleName);
    }
}

}  // namespace FusionRanging
}  // namespace OHOS