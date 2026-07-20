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
#define LOG_TAG "fc_taihe_fusion_ranging_observer"
#endif

#include "taihe_fusion_ranging_observer.h"
#include "taihe_fusion_connectivity_utils.h"
#include "log_utils.h"

namespace OHOS {
namespace FusionRanging {
using namespace FusionConnectivity;

TaiheFusionRangingObserver::~TaiheFusionRangingObserver()
{
    std::lock_guard<std::mutex> lock(lock_);
    if (isAttached_ && vm_ != nullptr) {
        vm_->DetachCurrentThread();
        isAttached_ = false;
    }
}

ani_env *TaiheFusionRangingObserver::GetEnv()
{
    if (vm_ == nullptr) {
        HILOGE("vm_ is nullptr in TaiheFusionRangingObserver");
        return nullptr;
    }
    std::lock_guard<std::mutex> lock(lock_);
    if (env_ != nullptr) {
        return env_;
    }

    env_ = GetCurrentEnv(vm_, isAttached_);
    if (env_ == nullptr) {
        HILOGE("GetCurrentEnv failed in TaiheFusionRangingObserver");
        return nullptr;
    }
    return env_;
}

void TaiheFusionRangingObserver::OnRangingStateChanged(const RangingStateChangeInfo &info)
{
    auto env = GetEnv();
    if (env == nullptr) {
        HILOGW("OnRangingStateChanged env nullptr");
        return;
    }

    std::lock_guard<std::mutex> guard(eventSubscribe_.lock_);
    ::ohos::FusionConnectivity::ranging::RangingStateChangeInfo stateInfo = {
        .state = ::ohos::FusionConnectivity::ranging::RangingState::from_value(static_cast<int32_t>(info.GetState())),
        .cause = ::ohos::FusionConnectivity::ranging::RangingStoppedCause::from_value(
            static_cast<int32_t>(info.GetCause())),
        .deviceId = info.GetDeviceId().empty() ?
                        ::taihe::optional<::taihe::string>(std::nullopt) :
                        ::taihe::optional<::taihe::string>(std::in_place, info.GetDeviceId().c_str()),
        .handle = info.GetHandle() < 0 ? ::taihe::optional<int32_t>(std::nullopt) :
                                         ::taihe::optional<int32_t>(std::in_place, info.GetHandle())};
    TaiheCreateLocalScope(env);
    for (auto callback : eventSubscribe_.callbackVec_) {
        if (callback) {
            (*callback)(stateInfo);
        }
    }
    TaiheDestroyLocalScope(env);
}

void TaiheFusionRangingObserver::OnRangingResult(const RangingResult &rangingResult)
{
    auto env = GetEnv();
    if (env == nullptr) {
        HILOGW("OnRangingResult env nullptr");
        return;
    }
    std::shared_ptr<TaiheRangingResultCallback> value = nullptr;
    auto found = resultCallback_.Find(rangingResult.GetDeviceId(), value);
    if (found && value != nullptr && value->callback_) {
        ::ohos::FusionConnectivity::ranging::RangingResult result = {
            .deviceId = ::taihe::string(rangingResult.GetDeviceId().c_str()),
            .distance =
                {
                    .value = rangingResult.GetDistance().GetValue(),
                    .confidence = ::ohos::FusionConnectivity::ranging::RangingConfidence::from_value(
                        static_cast<int32_t>(rangingResult.GetDistance().GetConfidence())),
                },
            .angle =
                {
                    .value = rangingResult.GetAngle().GetValue(),
                    .confidence = ::ohos::FusionConnectivity::ranging::RangingConfidence::from_value(
                        static_cast<int32_t>(rangingResult.GetAngle().GetConfidence())),
                },
            .rssi = rangingResult.GetRssi()};
        TaiheCreateLocalScope(env);
        (*value->callback_)(result);
        TaiheDestroyLocalScope(env);
    }
}

void TaiheFusionRangingObserver::RegisterRangingResultCallback(
    const RangingParams &params,
    const ::taihe::optional<::taihe::callback<void(::ohos::FusionConnectivity::ranging::RangingResult const &result)>>
        &callback)
{
    if (!callback) {
        HILOGW("RegisterRangingResultCallback callback is nullopt");
        return;
    }
    auto value = std::make_shared<TaiheRangingResultCallback>(params, callback);
    resultCallback_.EnsureInsert(params.GetDeviceId(), value);
}

void TaiheFusionRangingObserver::DeregisterRangingResultCallbackWithDeviceId(
    const ::taihe::optional<::taihe::callback<void(::ohos::FusionConnectivity::ranging::RangingResult const &result)>>
        &callback,
    const std::string &deviceId)
{
    std::lock_guard<std::mutex> lock(lock_);
    std::shared_ptr<TaiheRangingResultCallback> value = nullptr;
    auto ret = resultCallback_.Find(deviceId, value);
    if (ret && value != nullptr && value->callback_ == callback) {
        resultCallback_.Erase(deviceId);
    }
}

std::vector<RangingParams> TaiheFusionRangingObserver::GetRangParamsByCallback(
    const ::taihe::optional<::taihe::callback<void(::ohos::FusionConnectivity::ranging::RangingResult const &result)>>
        &callback)
{
    std::vector<RangingParams> findParams;
    resultCallback_.Iterate(
        [&findParams, &callback](const std::string &deviceId, const std::shared_ptr<TaiheRangingResultCallback> cb) {
            if (cb->callback_ == callback) {
                findParams.push_back(cb->params_);
            }
        });
    return findParams;
}

bool TaiheFusionRangingObserver::IsRangingResultCallbackEmpty()
{
    return resultCallback_.IsEmpty();
}
}  // namespace FusionRanging
}  // namespace OHOS