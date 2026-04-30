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

#include "napi_fusion_ranging_callback.h"
#include <uv.h>
#include "napi_async_work.h"
#include "napi_event_subscribe_module.h"
#include "napi_fusion_ranging_utils.h"

namespace OHOS {
namespace FusionRanging {

NapiFusionRangingObserver::NapiFusionRangingObserver()
    : eventSubscribe_(STR_FUSION_RANGING_CALLBACK_STATE_CHANGE, FCM_MODULE_NAME)
{
}

void NapiFusionRangingObserver::OnRangingStateChanged(const RangingStateChangeInfo &stateInfo)
{
    auto nativeObject = std::make_shared<NapiNativeRangingStateChange>(stateInfo);
    eventSubscribe_.PublishEvent(STR_FUSION_RANGING_CALLBACK_STATE_CHANGE, nativeObject);
}

void NapiFusionRangingObserver::OnRangingResult(const RangingResult &result)
{
    std::shared_ptr<NapiRangingResutlCallback> napiResultCallback;
    bool found = rangingResultCallback_.Find(result.GetDeviceId(), napiResultCallback);
    if (found && napiResultCallback != nullptr && napiResultCallback->callback_) {
        napiResultCallback->callback_(result);
    }
}

void NapiFusionRangingObserver::RegisterRangingResultCallback(
    const RangingParams &params, const std::shared_ptr<NapiCallback> &napiCallack,
    const std::function<void(const RangingResult &)> &callback)
{
    if (napiCallack == nullptr || callback == nullptr) {
        return;
    }
    auto value = std::make_shared<NapiRangingResutlCallback>(params, napiCallack, callback);
    if (value != nullptr) {
        rangingResultCallback_.EnsureInsert(params.GetDeviceId(), value);
    }
}

void NapiFusionRangingObserver::DeregisterRangingResultCallbackWithDeviceId(const napi_env env, napi_value callback,
                                                                            const std::string &deviceId)
{
    if (callback == nullptr || deviceId.empty()) {
        return;
    }

    std::shared_ptr<NapiRangingResutlCallback> value = nullptr;
    auto ret = rangingResultCallback_.Find(deviceId, value);
    if (ret && value != nullptr && value->napiCallback_->Equal(env, callback)) {
        rangingResultCallback_.Erase(deviceId);
    }
}

std::vector<RangingParams> NapiFusionRangingObserver::GetRangParamsByNapiCallback(const napi_env env,
                                                                                  napi_value callback)
{
    std::vector<RangingParams> findParams;
    if (callback == nullptr) {
        return findParams;
    }

    rangingResultCallback_.Iterate(
        [&](const std::string &deviceId, const std::shared_ptr<NapiRangingResutlCallback> &value) {
            if (value != nullptr && value->napiCallback_ != nullptr && value->napiCallback_->Equal(env, callback)) {
                findParams.push_back(value->params_);
            }
        });
    return findParams;
}

bool NapiFusionRangingObserver::IsRangingResultCallbackEmpty()
{
    return rangingResultCallback_.IsEmpty();
}
}  // namespace FusionRanging
}  // namespace OHOS
