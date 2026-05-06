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
#ifndef NAPI_FUSION_RANGING_CALLBACK_H
#define NAPI_FUSION_RANGING_CALLBACK_H

#include <shared_mutex>
#include "fusion_ranging_manager.h"
#include "napi_async_work.h"
#include "napi_event_subscribe_module.h"
#include "safe_map.h"

namespace OHOS {
namespace FusionRanging {
using namespace FusionConnectivity;
const char *const STR_FUSION_RANGING_CALLBACK_STATE_CHANGE = "FusionRangingStateChange";

struct NapiRangingResultCallback {
    NapiRangingResultCallback(const RangingParams &params, const std::shared_ptr<NapiCallback> &napiCallback,
                              const std::function<void(const RangingResult &)> &callback)
        : params_(params.GetDeviceId(), params.GetCapabilityType()),
          napiCallback_(napiCallback),
          callback_(callback)
    {
    }
    ~NapiRangingResultCallback() = default;

    RangingParams params_;
    std::shared_ptr<NapiCallback> napiCallback_;
    std::function<void(const RangingResult &)> callback_;
};

class NapiFusionRangingObserver : public FusionRangingObserver {
public:
    NapiFusionRangingObserver();
    ~NapiFusionRangingObserver() override = default;

    void OnRangingStateChanged(const RangingStateChangeInfo &info) override;
    void OnRangingResult(const RangingResult &result) override;

    void RegisterRangingResultCallback(const RangingParams &params, const std::shared_ptr<NapiCallback> &napiCallback,
                                       const std::function<void(const RangingResult &)> &callback);
    void DeregisterRangingResultCallbackWithDeviceId(const napi_env env, napi_value callback,
                                                     const std::string &deviceId);
    std::vector<RangingParams> GetRangParamsByNapiCallback(const napi_env env, napi_value callback);
    bool IsRangingResultCallbackEmpty();

    FusionConnectivity::NapiEventSubscribeModule eventSubscribe_;

private:
    SafeMap<std::string, std::shared_ptr<NapiRangingResultCallback>> rangingResultCallback_{};
};
}  // namespace Bluetooth
}  // namespace OHOS
#endif /* NAPI_FUSION_RANGING_CALLBACK_H */