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
#include "common_utils.h"
#include "log_utils.h"

#ifndef LOG_TAG
#define LOG_TAG "FusionRangingManagerFwk"
#endif

namespace OHOS {
namespace FusionRanging {
namespace {
constexpr int32_t LOADSA_TIMEOUT_4S = 4;
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
    class FusionRangingSystemAbility : public SystemAbilityStatusChangeStub {
    public:
        explicit FusionRangingSystemAbility(impl *outer) : outer_(outer){};
        void OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override;
        void OnRemoveSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override;

    private:
        impl *outer_ = nullptr;
        std::atomic<bool> isSaRemoved_ = false;
    };

    class RangingObserverHandlerImpl : public RangingObserverHandler {
    public:
        explicit RangingObserverHandlerImpl(FusionRangingManager::impl *outer) : outer_(outer) {}

        void OnRangingStateChanged(const RangingStateChangeInfo &info) override
        {
            if (!outer_) {
                return;
            }
            std::lock_guard<std::mutex> lock(outer_->mutex_);
            for (auto it = outer_->observers_.begin(); it != outer_->observers_.end(); ++it) {
                (*it)->OnRangingStateChanged(info);
            }
        }

        void OnRangingResult(const RangingResult &result) override
        {
            if (!outer_) {
                return;
            }
            std::lock_guard<std::mutex> lock(outer_->mutex_);
            for (auto it = outer_->observers_.begin(); it != outer_->observers_.end(); ++it) {
                (*it)->OnRangingResult(result);
            }
        }

    private:
        FusionRangingManager::impl *outer_;
    };

    void InitObserverHandler();

    sptr<RangingObserverHandlerImpl> observerHandler_;
    sptr<RangingObserverImpl> rangObserver_;
    sptr<FusionRangingSystemAbility> systemAbility_;

    std::mutex mutex_;
    std::list<std::shared_ptr<FusionRangingObserver>> observers_;
};

FusionRangingManager::impl::impl()
{
    HILOGI("FusionRangingManager impl()");
    systemAbility_ = sptr<FusionRangingSystemAbility>::MakeSptr(this);
    sptr<ISystemAbilityManager> samgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    int ret = samgrProxy->SubscribeSystemAbility(FUSION_RANGING_SYS_ABILITY_ID, systemAbility_);
    if (ret != RANGING_NO_ERROR) {
        HILOGE("SubscribeSystemAbility failed");
    }
}

FusionRangingManager::impl::~impl()
{
    HILOGI("FusionRangingManager ~impl()");
    auto proxy = GetRemoteProxy();
    if (proxy && rangObserver_) {
        proxy->DeregisterObserver(rangObserver_);
    }
    sptr<ISystemAbilityManager> samgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgrProxy && systemAbility_) {
        samgrProxy->UnSubscribeSystemAbility(FUSION_RANGING_SYS_ABILITY_ID, systemAbility_);
    }
}

void FusionRangingManager::impl::InitObserverHandler()
{
    observerHandler_ = sptr<RangingObserverHandlerImpl>::MakeSptr(this);
    rangObserver_ = sptr<RangingObserverImpl>::MakeSptr(observerHandler_);
    HILOGI("InitObserverHandler: observerHandler_ and rangObserver_ created");
    auto proxy = GetRemoteProxy();
    if (proxy && rangObserver_) {
        proxy->RegisterObserver(rangObserver_);
    }
}

void FusionRangingManager::impl::FusionRangingSystemAbility::OnAddSystemAbility(int32_t systemAbilityId,
                                                                                const std::string &deviceId)
{
    HILOGI("systemAbilityId:%{public}d", systemAbilityId);
    if (systemAbilityId == FUSION_RANGING_SYS_ABILITY_ID) {
        auto proxy = GetRemoteProxy();
        if (!proxy) {
            HILOGE("Get proxy failed");
            return;
        }
        if (isSaRemoved_.load()) {
            isSaRemoved_ = false;
            if (outer_ && outer_->rangObserver_) {
                proxy->RegisterObserver(outer_->rangObserver_);
            }
        }
    }
}

void FusionRangingManager::impl::FusionRangingSystemAbility::OnRemoveSystemAbility(int32_t systemAbilityId,
                                                                                   const std::string &deviceId)
{
    HILOGI("systemAbilityId:%{public}d", systemAbilityId);
    isSaRemoved_ = true;
}

FusionRangingManager::FusionRangingManager()
{
    HILOGI("FusionRangingManager constructed.");
    pimpl = std::make_unique<impl>();
    pimpl->InitObserverHandler();
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
    FCM_CHECK_RETURN_RET(proxy != nullptr, RANGING_ERR_RANGING_SERVICE_DISABLED, "proxy null");
    return proxy->GetRangingCapability(capability);
}

int FusionRangingManager::StartRanging(const RangingParams &params)
{
    if (!IsRangingSupported()) {
        return static_cast<int32_t>(RangingErrCode::RANGING_ERR_API_NOT_SUPPORT);
    }
    if (!IsValidAddress(params.GetDeviceId())) {
        HILOGE("param not valid");
        return RANGING_ERR_PARAM_NOT_MEET_SPECIFICATIONS;
    }
    auto proxy = GetRemoteProxy();
    FCM_CHECK_RETURN_RET(proxy != nullptr, RANGING_ERR_RANGING_SERVICE_DISABLED, "proxy null");
    int ret = proxy->StartRanging(params);
    HILOGI("StartRanging ret:%{public}d", ret);
    return ret;
}

int FusionRangingManager::StopRanging(const RangingParams &params)
{
    if (!IsRangingSupported()) {
        return static_cast<int32_t>(RangingErrCode::RANGING_ERR_API_NOT_SUPPORT);
    }

    if (!IsValidAddress(params.GetDeviceId())) {
        HILOGE("param not valid");
        return RANGING_ERR_PARAM_NOT_MEET_SPECIFICATIONS;
    }
    auto proxy = GetRemoteProxy();
    FCM_CHECK_RETURN_RET(proxy != nullptr, RANGING_ERR_RANGING_SERVICE_DISABLED, "proxy null");
    int ret = proxy->StopRanging(params);
    HILOGI("StopRanging ret:%{public}d", ret);
    return ret;
}

int FusionRangingManager::StartPassiveRanging(RangingTypes capabilityType, int32_t &handle)
{
    if (!IsRangingSupported()) {
        return static_cast<int32_t>(RangingErrCode::RANGING_ERR_API_NOT_SUPPORT);
    }
    auto proxy = GetRemoteProxy();
    FCM_CHECK_RETURN_RET(proxy != nullptr, RANGING_ERR_RANGING_SERVICE_DISABLED, "proxy null");
    int ret = proxy->StartPassiveRanging(static_cast<int>(capabilityType), handle);
    HILOGI("StartPassiveRanging ret:%{public}d", ret);
    return ret;
}

int FusionRangingManager::StopPassiveRanging(RangingTypes capabilityType, int32_t handle)
{
    if (!IsRangingSupported()) {
        return static_cast<int32_t>(RangingErrCode::RANGING_ERR_API_NOT_SUPPORT);
    }
    FCM_CHECK_RETURN_RET(handle >= 0, RANGING_ERR_PARAM_NOT_MEET_SPECIFICATIONS, "invalid handle");
    auto proxy = GetRemoteProxy();
    FCM_CHECK_RETURN_RET(proxy != nullptr, RANGING_ERR_RANGING_SERVICE_DISABLED, "proxy null");
    int ret = proxy->StopPassiveRanging(static_cast<int>(capabilityType), handle);
    HILOGI("StopPassiveRanging ret:%{public}d", ret);
    return ret;
}

int FusionRangingManager::RegisterFusionRangingObserver(const std::shared_ptr<FusionRangingObserver> &observer)
{
    if (!IsRangingSupported()) {
        return static_cast<int32_t>(RangingErrCode::RANGING_ERR_API_NOT_SUPPORT);
    }
    std::lock_guard<std::mutex> lock(pimpl->mutex_);
    for (auto it = pimpl->observers_.begin(); it != pimpl->observers_.end(); ++it) {
        if (*it == observer) {
            return RANGING_NO_ERROR;
        }
    }
    pimpl->observers_.push_back(observer);
    return RANGING_NO_ERROR;
}

int FusionRangingManager::DeregisterFusionRangingObserver(const std::shared_ptr<FusionRangingObserver> &observer)
{
    if (!IsRangingSupported()) {
        return static_cast<int32_t>(RangingErrCode::RANGING_ERR_API_NOT_SUPPORT);
    }
    std::lock_guard<std::mutex> lock(pimpl->mutex_);
    for (auto it = pimpl->observers_.begin(); it != pimpl->observers_.end();) {
        if (*it == observer) {
            it = pimpl->observers_.erase(it);
            return RANGING_NO_ERROR;
        } else {
            ++it;
        }
    }
    return RANGING_ERR_OPERATION_FAILED;
}
}  // namespace FusionRanging
}  // namespace OHOS