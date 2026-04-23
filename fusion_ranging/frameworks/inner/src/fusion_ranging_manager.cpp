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

#include "fusion_ranging_manager.h"
#include "fusion_ranging_errorcode.h"
#include "fusion_ranging_observer.h"
#include "fusion_ranging_proxy.h"
#include "iservice_registry.h"
#include "if_system_ability_manager.h"
#include "parameter.h"
#include "log_utils.h"

#ifndef LOG_TAG
#define LOG_TAG "FusionRangingManagerFwk"
#endif

namespace OHOS {
namespace FusionRanging {
namespace {
constexpr int32_t LOADSA_TIMEOUT_4S = 4;
constexpr size_t MAX_OBSERVERS_SIZE = 100;
constexpr int32_t FUSION_RANGING_SYS_ABILITY_ID = 8631;
}

static sptr<IFusionRanging> GetRemoteProxy()
{
    auto samgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (!samgrProxy) {
        HILOGE("samgrProxy is nullptr.");
        return nullptr;
    }
    sptr<IRemoteObject> remote = samgrProxy->LoadSystemAbility(FUSION_RANGING_SYS_ABILITY_ID, LOADSA_TIMEOUT_4S);
    if (remote == nullptr) {
        HILOGE("remote is nullptr.");
        return nullptr;
    }

    sptr<IFusionRanging> proxy = iface_cast<IFusionRanging>(remote);
    if (proxy == nullptr) {
        HILOGE("failed: no proxy");
        return nullptr;
    }
    return proxy;
}

struct FusionRangingManager::impl {
    impl();
    ~impl();

    bool CreateRangingObserverIfNeeded();
    void DestroyRangingObserver();

    std::mutex observerMutex_;
    sptr<RangingResultObserverImpl> rangingObserver_;
    sptr<RangingStateObserverImpl> stateObserver_;

    std::mutex rangingMutex_;
    std::map<std::string, std::function<void(const RangingResult &)>> rangingResultCallbacks_;
    std::mutex stateMutex_;
    std::function<void(const RangingStateChangeInfo &)> stateChangeCallback_;
};

FusionRangingManager::impl::impl()
{
    HILOGI("FusionRangingManager impl()");
}

FusionRangingManager::impl::~impl()
{
    HILOGI("FusionRangingManager ~impl()");
}

bool FusionRangingManager::impl::CreateRangingObserverIfNeeded()
{
    if (!rangingResultCallbacks_.empty()) {
        return true;
    }

    auto observer = sptr<RangingResultObserverImpl>::MakeSptr();
    if (observer == nullptr) {
        HILOGE("CreateRangingObserverIfNeeded: failed to create rangingObserver_");
        return false;
    }
    std::lock_guard<std::mutex> lock(observerMutex_);
    rangingObserver_ = observer;
    impl *self = this;
    rangingObserver_->SetResultCallback([self](const RangingResult &result) {
        std::lock_guard<std::mutex> lock(self->rangingMutex_);
        auto it = self->rangingResultCallbacks_.find(result.GetDeviceId());
        if (it != self->rangingResultCallbacks_.end() && it->second) {
            HILOGI("OnRangingResult callback deviceId: %{public}s", GET_ENCRYPT_ADDR(result.GetDeviceId()));
            it->second(result);
        }
    });

    HILOGI("CreateRangingObserverIfNeeded: created rangingObserver_ for first device");
    return true;
}

void FusionRangingManager::impl::DestroyRangingObserver()
{
    std::lock_guard<std::mutex> lock(observerMutex_);
    rangingObserver_ = nullptr;
    HILOGI("DestroyRangingObserver: destroyed rangingObserver_ for last device");
}

FusionRangingManager::FusionRangingManager()
{
    HILOGI("FusionRangingManager constructed.");
    pimpl = std::make_unique<impl>();
}

FusionRangingManager::~FusionRangingManager()
{
    HILOGI("~FusionRangingManager destroyed.");
}

FusionRangingManager *FusionRangingManager::GetInstance()
{
    static FusionRangingManager instance;
    return &instance;
}

bool FusionRangingManager::IsRangingSupported() const
{
    bool isSupport = false;
#ifdef SUPPORT_FUSION_RANGING
    isSupport = true;
#endif
    HILOGI("IsRangingSupported isSupport:%{public}d", isSupport);
    return isSupport;
}

int32_t FusionRangingManager::GetRangingCapability(RangingCapabilitySupported &capability)
{
    if (!IsRangingSupported()) {
        capability.SetNearlinkHadm(false);
        return static_cast<int32_t>(RangingErrCode::RANGING_ERR_API_NOT_SUPPORT);
    }
    auto proxy = GetRemoteProxy();
    FCM_CHECK_RETURN_RET(proxy != nullptr, RANGING_ERR_SERVICE_NOT_PROVIDED, "proxy null");
    HILOGE("GetRangingCapability proxy call");
    return proxy->GetRangingCapability(capability);
}

int FusionRangingManager::StartRanging(const RangingParams &params,
                                       const std::function<void(const RangingResult &)> &callback)
{
    if (!IsRangingSupported()) {
        return static_cast<int32_t>(RangingErrCode::RANGING_ERR_API_NOT_SUPPORT);
    }
    auto proxy = GetRemoteProxy();
    FCM_CHECK_RETURN_RET(proxy != nullptr, RANGING_ERR_SERVICE_NOT_PROVIDED, "proxy null");
    std::string deviceId = params.GetDeviceId();
    {
        std::lock_guard<std::mutex> lock(pimpl->rangingMutex_);
        if (pimpl->rangingResultCallbacks_.size() >= MAX_OBSERVERS_SIZE) {
            HILOGE("Too many observers registered");
            return static_cast<int32_t>(RangingErrCode::RANGING_ERR_INTERNAL_ERROR);
        }
        FCM_CHECK_RETURN_RET(pimpl->rangingResultCallbacks_.size() < MAX_OBSERVERS_SIZE, RANGING_ERR_INTERNAL_ERROR,
                             "proxy null");
        if (!pimpl->CreateRangingObserverIfNeeded()) {
            return static_cast<int32_t>(RangingErrCode::RANGING_ERR_INTERNAL_ERROR);
        }
        pimpl->rangingResultCallbacks_[deviceId] = callback;
    }

    std::lock_guard<std::mutex> lock(pimpl->observerMutex_);
    int ret = proxy->StartRanging(params, pimpl->rangingObserver_);
    HILOGI("StartRanging ret:%{public}d", ret);
    if (ret != 0) {
        std::lock_guard<std::mutex> lock(pimpl->rangingMutex_);
        pimpl->rangingResultCallbacks_.erase(deviceId);
        if (pimpl->rangingResultCallbacks_.empty()) {
            pimpl->DestroyRangingObserver();
            HILOGI("StartRanging: rollback rangingObserver_ due to failure");
        }
    }
    return ret;
}

int FusionRangingManager::StopRanging(const std::string &deviceId)
{
    if (!IsRangingSupported()) {
        return static_cast<int32_t>(RangingErrCode::RANGING_ERR_API_NOT_SUPPORT);
    }

    auto proxy = GetRemoteProxy();
    FCM_CHECK_RETURN_RET(proxy != nullptr, RANGING_ERR_SERVICE_NOT_PROVIDED, "proxy null");
    bool shouldDestroyObserver = false;
    sptr<RangingResultObserverImpl> observerToStop = nullptr;
    {
        std::lock_guard<std::mutex> lock(pimpl->rangingMutex_);
        pimpl->rangingResultCallbacks_.erase(deviceId);
        observerToStop = pimpl->rangingObserver_;
        shouldDestroyObserver = (pimpl->rangingResultCallbacks_.empty() && pimpl->rangingObserver_ != nullptr);
        if (shouldDestroyObserver) {
            pimpl->rangingObserver_ = nullptr;
        }
    }

    int ret = proxy->StopRanging(deviceId, observerToStop);
    if (shouldDestroyObserver) {
        pimpl->DestroyRangingObserver();
    }
    return ret;
}

int FusionRangingManager::OnRangingStateChange(const std::function<void(const RangingStateChangeInfo &)> &callback)
{
    if (!IsRangingSupported()) {
        return static_cast<int32_t>(RangingErrCode::RANGING_ERR_API_NOT_SUPPORT);
    }
    {
        std::lock_guard<std::mutex> lock(pimpl->stateMutex_);
        FCM_CHECK_RETURN_RET(pimpl->stateChangeCallback_ == nullptr, RANGING_ERR_PARAM_IS_OCCUPIED, "proxy null");
        pimpl->stateChangeCallback_ = callback;
    }
    auto observer = sptr<RangingStateObserverImpl>::MakeSptr();
    FCM_CHECK_RETURN_RET(observer != nullptr, RANGING_ERR_INTERNAL_ERROR, "proxy null");
    auto proxy = GetRemoteProxy();
    FCM_CHECK_RETURN_RET(proxy != nullptr, RANGING_ERR_SERVICE_NOT_PROVIDED, "proxy null");
    std::lock_guard<std::mutex> lock(pimpl->observerMutex_);
    pimpl->stateObserver_ = observer;
    auto self = this;
    pimpl->stateObserver_->SetStateCallback([self](const RangingStateChangeInfo &info) {
        std::lock_guard<std::mutex> lock(self->pimpl->stateMutex_);
        if (self->pimpl->stateChangeCallback_) {
            self->pimpl->stateChangeCallback_(info);
        }
    });
    auto ret = proxy->RegisterStateObserver(pimpl->stateObserver_);
    HILOGI("OnRangingStateChange ret:%{public}d", ret);
    return ret;
}

int FusionRangingManager::OffRangingStateChange()
{
    if (!IsRangingSupported()) {
        return static_cast<int32_t>(RangingErrCode::RANGING_ERR_API_NOT_SUPPORT);
    }

    auto proxy = GetRemoteProxy();
    FCM_CHECK_RETURN_RET(proxy != nullptr, RANGING_ERR_SERVICE_NOT_PROVIDED, "proxy null");
    std::lock_guard<std::mutex> lock(pimpl->observerMutex_);
    FCM_CHECK_RETURN_RET(pimpl->stateObserver_ != nullptr, RANGING_ERR_INTERNAL_ERROR, "observer null");
    auto ret = proxy->UnregisterStateObserver(pimpl->stateObserver_);
    if (ret == 0) {
        pimpl->stateObserver_ = nullptr;
        std::lock_guard<std::mutex> lock(pimpl->stateMutex_);
        pimpl->stateChangeCallback_ = nullptr;
    }
    HILOGI("OffRangingStateChange ret:%{public}d", ret);
    return ret;
}
}  // namespace FusionRanging
}  // namespace OHOS