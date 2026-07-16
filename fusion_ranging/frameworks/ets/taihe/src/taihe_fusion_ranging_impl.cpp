/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of, use or distribute this file except in compliance with
 * the License. You may obtain a copy of the License at
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
#define LOG_TAG "TaiheFusionRangingImpl"
#endif

#include <map>
#include <memory>
#include <mutex>

#include "taihe_fusion_ranging_utils.h"
#include "taihe_fusion_ranging_error.h"
#include "taihe_fusion_ranging_observer.h"

#include "fusion_ranging_errorcode.h"
#include "fusion_ranging_manager.h"
#include "common_utils.h"
#include "log_utils.h"

namespace OHOS {
namespace FusionRanging {
std::shared_ptr<TaiheFusionRangingObserver> observer_ = nullptr;
std::shared_mutex lock_{};

void TaiheInitFusionRangingObserver(ani_vm *vm)
{
    std::unique_lock<std::shared_mutex> guard(lock_);
    if (observer_ != nullptr) {
        return;
    }
    observer_ = std::make_shared<TaiheFusionRangingObserver>(vm);
}

bool IsRangingSupported()
{
    HILOGI("IsRangingSupported enter");
    auto isSupported = FusionRangingManager::GetInstance()->IsRangingSupported();
    return isSupported;
}

ohos::FusionConnectivity::ranging::RangingCapabilitySupported GetRangingCapability()
{
    HILOGI("enter");
    RangingCapabilitySupported cap;
    int32_t capabilityRet = FusionRangingManager::GetInstance()->GetRangingCapability(cap);
    HILOGI("GetRangingCapability from fwk, capabilityRet: %{public}d, nearlinkHadm: %{public}d", capabilityRet,
           cap.GetNearlinkHadm());
    ohos::FusionConnectivity::ranging::RangingCapabilitySupported resCap{};
    resCap.nearlinkHadm = cap.GetNearlinkHadm();
    return resCap;
}

void StartRanging(
    ::ohos::FusionConnectivity::ranging::RangingParams const &params,
    ::taihe::callback_view<void(::ohos::FusionConnectivity::ranging::RangingResult const &result)> callback)
{
    {
        std::unique_lock<std::shared_mutex> guard(lock_);
        TAIHE_FC_ASSERT_RETURN_VOID(observer_, RANGING_ERR_OPERATION_FAILED);
    }
    TAIHE_FC_ASSERT_RETURN_VOID(IsValidAddress(params.deviceId.c_str()), RANGING_ERR_PARAM_NOT_MEET_SPECIFICATIONS);
    auto isValidCap = (params.capabilityType > 0) &&
                      (params.capabilityType < static_cast<int>(RangingTypes::RANGING_TYPE_MAX));
    TAIHE_FC_ASSERT_RETURN_VOID(isValidCap, RANGING_ERR_RANGING_TYPE_NOT_SUPPORT);

    auto resultCb =
        ::taihe::optional<::taihe::callback<void(::ohos::FusionConnectivity::ranging::RangingResult const &result)>>{
            std::in_place_t{}, callback};
    RangingParams param(params.deviceId.c_str(), static_cast<RangingTypes>(params.capabilityType.get_value()));
    auto ret = FusionRangingManager::GetInstance()->StartRanging(param);
    if (ret != RANGING_NO_ERROR) {
        TAIHE_FC_ASSERT_RETURN_VOID(false, ret);
    }
    std::unique_lock<std::shared_mutex> guard(lock_);
    FusionRangingManager::GetInstance()->RegisterFusionRangingObserver(observer_);
    observer_->RegisterRangingResultCallback(param, resultCb);
}

void StopRanging(
    ::taihe::callback_view<void(::ohos::FusionConnectivity::ranging::RangingResult const &result)> callback,
    ::taihe::optional_view<::ohos::FusionConnectivity::ranging::RangingParams> params)
{
    std::unique_lock<std::shared_mutex> guard(lock_);
    TAIHE_FC_ASSERT_RETURN_VOID(observer_, RANGING_ERR_OPERATION_FAILED);
    auto resultCb =
        ::taihe::optional<::taihe::callback<void(::ohos::FusionConnectivity::ranging::RangingResult const &result)>>{
            std::in_place_t{}, callback};
    if (params.has_value()) {
        TAIHE_FC_ASSERT_RETURN_VOID(IsValidAddress(params->deviceId.c_str()),
                                    RANGING_ERR_PARAM_NOT_MEET_SPECIFICATIONS);
        auto isValidCap = (params->capabilityType > 0) &&
                          (params->capabilityType < static_cast<int>(RangingTypes::RANGING_TYPE_MAX));
        TAIHE_FC_ASSERT_RETURN_VOID(isValidCap, RANGING_ERR_RANGING_TYPE_NOT_SUPPORT);

        RangingParams param(params->deviceId.c_str(), static_cast<RangingTypes>(params->capabilityType.get_value()));
        FusionRangingManager::GetInstance()->StopRanging(param);
        observer_->DeregisterRangingResultCallbackWithDeviceId(resultCb, param.GetDeviceId());
    } else {
        auto stopDevices = observer_->GetRangParamsByCallback(resultCb);
        for (auto it = stopDevices.begin(); it != stopDevices.end(); it++) {
            FusionRangingManager::GetInstance()->StopRanging(*it);
            observer_->DeregisterRangingResultCallbackWithDeviceId(resultCb, it->GetDeviceId());
        }
    }
    if (observer_->IsRangingResultCallbackEmpty()) {
        FusionRangingManager::GetInstance()->DeregisterFusionRangingObserver(observer_);
    }
}

int32_t StartPassiveRanging(int32_t capabilityType)
{
    int32_t handle = -1;  // -1 as invalid ranging handle.
    HILOGI("enter capabilityType:%{public}d", capabilityType);
    {
        std::unique_lock<std::shared_mutex> guard(lock_);
        TAIHE_FC_ASSERT_RETURN(observer_, RANGING_ERR_OPERATION_FAILED, handle);
    }
    auto isValidCap = (capabilityType > 0) && (capabilityType < static_cast<int>(RangingTypes::RANGING_TYPE_MAX));
    TAIHE_FC_ASSERT_RETURN(isValidCap, RANGING_ERR_RANGING_TYPE_NOT_SUPPORT, handle);
    auto ret =
        FusionRangingManager::GetInstance()->StartPassiveRanging(static_cast<RangingTypes>(capabilityType), handle);
    TAIHE_FC_ASSERT_RETURN(ret == RANGING_NO_ERROR, RANGING_ERR_OPERATION_FAILED, handle);
    std::unique_lock<std::shared_mutex> guard(lock_);
    FusionRangingManager::GetInstance()->RegisterFusionRangingObserver(observer_);
    return handle;
}

void StopPassiveRanging(int32_t handle, int32_t capabilityType)
{
    HILOGI("enter capabilityType:%{public}d, handle:%{public}d", capabilityType, handle);
    TAIHE_FC_ASSERT_RETURN_VOID(handle >= 0, RANGING_ERR_RANGING_TYPE_NOT_SUPPORT);

    auto isValidCap = (capabilityType > 0) && (capabilityType < static_cast<int>(RangingTypes::RANGING_TYPE_MAX));
    TAIHE_FC_ASSERT_RETURN_VOID(isValidCap, RANGING_ERR_RANGING_TYPE_NOT_SUPPORT);

    auto ret =
        FusionRangingManager::GetInstance()->StopPassiveRanging(static_cast<RangingTypes>(capabilityType), handle);
    TAIHE_FC_ASSERT_RETURN_VOID(ret == RANGING_NO_ERROR, ret);
    std::unique_lock<std::shared_mutex> guard(lock_);
    TAIHE_FC_ASSERT_RETURN_VOID(observer_, RANGING_ERR_OPERATION_FAILED);
    if (observer_->IsRangingResultCallbackEmpty()) {
        FusionRangingManager::GetInstance()->DeregisterFusionRangingObserver(observer_);
    }
}

void OnRangingStateChange(
    ::taihe::callback_view<void(::ohos::FusionConnectivity::ranging::RangingStateChangeInfo const &info)> callback)
{
    std::unique_lock<std::shared_mutex> guard(lock_);
    TAIHE_FC_ASSERT_RETURN_VOID(observer_, RANGING_ERR_OPERATION_FAILED);
    if (observer_) {
        observer_->eventSubscribe_.RegisterEvent(callback);
    }
}

void OffRangingStateChange(
    ::taihe::optional_view<
        ::taihe::callback<void(::ohos::FusionConnectivity::ranging::RangingStateChangeInfo const &info)>>
        callback)
{
    std::unique_lock<std::shared_mutex> guard(lock_);
    TAIHE_FC_ASSERT_RETURN_VOID(observer_, RANGING_ERR_OPERATION_FAILED);
    if (observer_) {
        observer_->eventSubscribe_.DeregisterEvent(callback);
    }
}
}  // namespace FusionRanging
}  // namespace OHOS
TH_EXPORT_CPP_API_IsRangingSupported(OHOS::FusionRanging::IsRangingSupported);
TH_EXPORT_CPP_API_GetRangingCapability(OHOS::FusionRanging::GetRangingCapability);
TH_EXPORT_CPP_API_StartRanging(OHOS::FusionRanging::StartRanging);
TH_EXPORT_CPP_API_StopRanging(OHOS::FusionRanging::StopRanging);
TH_EXPORT_CPP_API_StartPassiveRanging(OHOS::FusionRanging::StartPassiveRanging);
TH_EXPORT_CPP_API_StopPassiveRanging(OHOS::FusionRanging::StopPassiveRanging);
TH_EXPORT_CPP_API_OnRangingStateChange(OHOS::FusionRanging::OnRangingStateChange);
TH_EXPORT_CPP_API_OffRangingStateChange(OHOS::FusionRanging::OffRangingStateChange);