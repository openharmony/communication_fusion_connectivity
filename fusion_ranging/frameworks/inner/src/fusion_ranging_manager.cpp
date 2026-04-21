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
#include "log_util.h"

#ifndef LOG_TAG
#define LOG_TAG "FusionRangingManagerFwk"
#endif

namespace OHOS {
namespace FusionRanging {
namespace {
constexpr int32_t LOADSA_TIMEOUT_4S = 4;
constexpr size_t MAX_OBSERVERS_SIZE = 100;
constexpr int32_t FUSION_RANGING_SYS_ABILITY_ID = 8631;

static sptr<IFusionRanging> g_cachedProxy = nullptr;
static std::mutex g_proxyMutex;
}

static sptr<IFusionRanging> GetRemoteProxy()
{
    {
        std::lock_guard<std::mutex> lock(g_proxyMutex);
        if (g_cachedProxy != nullptr) {
            return g_cachedProxy;
        }
    }

    auto samgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (!samgrProxy) {
        HILOGE("samgrProxy is nullptr.");
        return nullptr;
    }

    sptr<IRemoteObject> remote = samgrProxy->LoadSystemAbility(FUSION_RANGING_SYS_ABILITY_ID, LOADSA_TIMEOUT_4S);
    HILOGE("remote object isValid: %{public}d", remote ? remote->IsProxyObject() : 0);
    if (remote == nullptr) {
        HILOGE("remote is nullptr.");
        return nullptr;
    }

    std::u16string localDesc = IFusionRanging::GetDescriptor();
    HILOGE("local descriptor: %{public}s", Str16ToStr8(localDesc).c_str());

    sptr<IFusionRanging> proxy = iface_cast<IFusionRanging>(remote);
    HILOGE("iface_cast result: %{public}s", proxy != nullptr ? "valid" : "null");
    if (proxy == nullptr) {
        HILOGE("failed: no proxy");
        return nullptr;
    }

    {
        std::lock_guard<std::mutex> lock(g_proxyMutex);
        g_cachedProxy = proxy;
    }
    return proxy;
}

struct FusionRangingManager::impl {
    impl();
    ~impl();

    void SubscribeSaStatus();
    void UnsubscribeSaStatus();
    bool CreateRangingObserverIfNeeded();
    void DestroyRangingObserver();
    bool CreateStateObserverIfNeeded(const std::function<void(const RangingStateChangeInfo &)> &callback);
    void DestroyStateObserver();

    sptr<RangingResultObserverImpl> rangingObserver_;
    sptr<RangingStateObserverImpl> stateObserver_;
    sptr<FusionRangingSaStatusChange> saStatusChange_;

    std::mutex rangingMutex_;
    std::map<std::string, std::function<void(const RangingResult &)>> rangingResultCallbacks_;
    std::mutex stateMutex_;
    std::vector<std::function<void(const RangingStateChangeInfo &)>> stateChangeCallbacks_;
};

FusionRangingManager::impl::impl()
{
    HILOGI("FusionRangingManager impl()");
    saStatusChange_ = new (std::nothrow) FusionRangingSaStatusChange();

    saStatusChange_->SetRemoveCallback([]() {
        std::lock_guard<std::mutex> lock(g_proxyMutex);
        g_cachedProxy = nullptr;
    });

    SubscribeSaStatus();
}

FusionRangingManager::impl::~impl()
{
    HILOGI("FusionRangingManager ~impl()");
    UnsubscribeSaStatus();
    std::lock_guard<std::mutex> lock(g_proxyMutex);
    g_cachedProxy = nullptr;
}

void FusionRangingManager::impl::SubscribeSaStatus()
{
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (!samgr) {
        HILOGE("SubscribeSaStatus: samgr is nullptr");
        return;
    }

    if (!saStatusChange_) {
        HILOGE("SubscribeSaStatus: saStatusChange_ is nullptr");
        return;
    }

    auto ret = samgr->SubscribeSystemAbility(FUSION_RANGING_SYS_ABILITY_ID, saStatusChange_);
    if (ret != ERR_OK) {
        HILOGE("SubscribeSystemAbility failed, ret:%{public}d", ret);
        saStatusChange_ = nullptr;
        return;
    }
    HILOGI("SubscribeSystemAbility success");
}

void FusionRangingManager::impl::UnsubscribeSaStatus()
{
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (!samgr || !saStatusChange_) {
        return;
    }

    samgr->UnSubscribeSystemAbility(FUSION_RANGING_SYS_ABILITY_ID, saStatusChange_);
    saStatusChange_ = nullptr;
    HILOGI("UnsubscribeSystemAbility success");
}

bool FusionRangingManager::impl::CreateRangingObserverIfNeeded()
{
    if (!rangingResultCallbacks_.empty()) {
        return true;
    }

    rangingObserver_ = new (std::nothrow) RangingResultObserverImpl();
    if (rangingObserver_ == nullptr) {
        HILOGE("CreateRangingObserverIfNeeded: failed to create rangingObserver_");
        return false;
    }

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
    if (!rangingResultCallbacks_.empty() || rangingObserver_ == nullptr) {
        return;
    }

    rangingObserver_ = nullptr;
    HILOGI("DestroyRangingObserver: destroyed rangingObserver_ for last device");
}

bool FusionRangingManager::impl::CreateStateObserverIfNeeded(
    const std::function<void(const RangingStateChangeInfo &)> &callback)
{
    if (!stateChangeCallbacks_.empty()) {
        stateChangeCallbacks_.push_back(callback);
        return true;
    }

    stateObserver_ = new (std::nothrow) RangingStateObserverImpl();
    if (stateObserver_ == nullptr) {
        HILOGE("CreateStateObserverIfNeeded: failed to create stateObserver_");
        return false;
    }

    impl *self = this;
    stateObserver_->SetStateCallback([self](const RangingStateChangeInfo &info) {
        std::lock_guard<std::mutex> lock(self->stateMutex_);
        for (const auto &cb : self->stateChangeCallbacks_) {
            if (cb) {
                cb(info);
            }
        }
    });

    stateChangeCallbacks_.push_back(callback);

    auto proxy = GetRemoteProxy();
    if (proxy != nullptr) {
        proxy->RegisterStateObserver(stateObserver_);
        HILOGI("CreateStateObserverIfNeeded: registered stateObserver_");
    }

    HILOGI("CreateStateObserverIfNeeded: created stateObserver_ for first callback");
    return true;
}

void FusionRangingManager::impl::DestroyStateObserver()
{
    if (stateObserver_ == nullptr) {
        return;
    }

    auto proxy = GetRemoteProxy();
    if (proxy != nullptr) {
        proxy->UnregisterStateObserver(stateObserver_);
    }

    stateObserver_ = nullptr;
    stateChangeCallbacks_.clear();
    HILOGI("DestroyStateObserver: unregistered and destroyed stateObserver_");
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
    if (proxy == nullptr) {
        HILOGE("GetRangingCapability proxy nullptr");
        return static_cast<int32_t>(RangingErrCode::RANGING_ERR_INTERNAL_ERROR);
    }

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
    HILOGE("StartRanging proxy nullptr:%{public}d", proxy == nullptr);

    if (proxy == nullptr) {
        return static_cast<int32_t>(RangingErrCode::RANGING_ERR_INTERNAL_ERROR);
    }

    std::string deviceId = params.GetDeviceId();

    if (pimpl->rangingResultCallbacks_.size() >= MAX_OBSERVERS_SIZE) {
        HILOGE("Too many observers registered");
        return static_cast<int32_t>(RangingErrCode::RANGING_ERR_INTERNAL_ERROR);
    }

    bool shouldCreateObserver = pimpl->rangingResultCallbacks_.empty();
    {
        std::lock_guard<std::mutex> lock(pimpl->rangingMutex_);
        if (!pimpl->CreateRangingObserverIfNeeded()) {
            return static_cast<int32_t>(RangingErrCode::RANGING_ERR_INTERNAL_ERROR);
        }
        pimpl->rangingResultCallbacks_[deviceId] = callback;
    }

    int ret = proxy->StartRanging(params, pimpl->rangingObserver_);
    HILOGI("StartRanging ret:%{public}d", ret);
    if (ret == 0) {
        return 0;
    }

    std::lock_guard<std::mutex> lock(pimpl->rangingMutex_);
    pimpl->rangingResultCallbacks_.erase(deviceId);

    if (shouldCreateObserver) {
        pimpl->DestroyRangingObserver();
        HILOGI("StartRanging: rollback rangingObserver_ due to failure");
    }

    return ret;
}

int FusionRangingManager::StopRanging(const std::string &deviceId)
{
    if (!IsRangingSupported()) {
        return static_cast<int32_t>(RangingErrCode::RANGING_ERR_API_NOT_SUPPORT);
    }

    auto proxy = GetRemoteProxy();
    if (proxy == nullptr) {
        return static_cast<int32_t>(RangingErrCode::RANGING_ERR_INTERNAL_ERROR);
    }

    bool shouldDestroyObserver = false;
    sptr<RangingResultObserverImpl> observerToStop = nullptr;
    {
        std::lock_guard<std::mutex> lock(pimpl->rangingMutex_);
        pimpl->rangingResultCallbacks_.erase(deviceId);
        shouldDestroyObserver = (pimpl->rangingResultCallbacks_.empty() && pimpl->rangingObserver_ != nullptr);
        if (shouldDestroyObserver) {
            observerToStop = pimpl->rangingObserver_;
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

    if (pimpl->stateChangeCallbacks_.size() >= MAX_OBSERVERS_SIZE) {
        HILOGE("Too many state observers registered");
        return static_cast<int32_t>(RangingErrCode::RANGING_ERR_INTERNAL_ERROR);
    }

    if (!pimpl->CreateStateObserverIfNeeded(callback)) {
        return static_cast<int32_t>(RangingErrCode::RANGING_ERR_INTERNAL_ERROR);
    }

    return 0;
}

int FusionRangingManager::OffRangingStateChange()
{
    if (!IsRangingSupported()) {
        return static_cast<int32_t>(RangingErrCode::RANGING_ERR_API_NOT_SUPPORT);
    }

    auto proxy = GetRemoteProxy();
    if (proxy == nullptr) {
        return static_cast<int32_t>(RangingErrCode::RANGING_ERR_INTERNAL_ERROR);
    }

    pimpl->DestroyStateObserver();
    return 0;
}

}  // namespace FusionRanging
}  // namespace OHOS