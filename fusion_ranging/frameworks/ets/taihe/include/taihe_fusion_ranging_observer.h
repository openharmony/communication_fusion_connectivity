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

#ifndef TAIHE_FUSION_RANGING_OBSERVER_H
#define TAIHE_FUSION_RANGING_OBSERVER_H

#include <vector>
#include "taihe/runtime.hpp"
#include "taihe_event_module.h"
#include "fusion_ranging_manager.h"
#include "ohos.FusionConnectivity.ranging.proj.hpp"
#include "ohos.FusionConnectivity.ranging.impl.hpp"
#include "stdexcept"
#include "safe_map.h"

namespace OHOS {
namespace FusionRanging {
struct TaiheRangingResultCallback {
    TaiheRangingResultCallback(
        const RangingParams &params,
        const ::taihe::optional<
            ::taihe::callback<void(::ohos::FusionConnectivity::ranging::RangingResult const &result)>> &callback)
        : params_(params),
          callback_(callback)
    {
    }
    ~TaiheRangingResultCallback() = default;

    RangingParams params_;
    ::taihe::optional<::taihe::callback<void(::ohos::FusionConnectivity::ranging::RangingResult const &result)>>
        callback_;
};

class TaiheFusionRangingObserver : public FusionRangingObserver {
public:
    TaiheFusionRangingObserver(ani_vm *vm) : vm_(vm){};
    ~TaiheFusionRangingObserver() override;

    void OnRangingStateChanged(const RangingStateChangeInfo &statechange) override;
    void OnRangingResult(const RangingResult &rangingResult) override;

    void RegisterRangingResultCallback(
        const RangingParams &params,
        const ::taihe::optional<
            ::taihe::callback<void(::ohos::FusionConnectivity::ranging::RangingResult const &result)>> &callback);
    void DeregisterRangingResultCallbackWithDeviceId(
        const ::taihe::optional<
            ::taihe::callback<void(::ohos::FusionConnectivity::ranging::RangingResult const &result)>> &callback,
        const std::string &deviceId);
    std::vector<RangingParams> GetRangParamsByCallback(
        const ::taihe::optional<
            ::taihe::callback<void(::ohos::FusionConnectivity::ranging::RangingResult const &result)>> &callback);
    bool IsRangingResultCallbackEmpty();

    EventModule<void(::ohos::FusionConnectivity::ranging::RangingStateChangeInfo const &info)> eventSubscribe_;

private:
    ani_env *GetEnv();

    SafeMap<std::string, std::shared_ptr<TaiheRangingResultCallback>> resultCallback_{};
    ani_vm *vm_{};
    std::mutex lock_{};
    ani_env *env_{};
    bool isAttached_{};
};
ani_status TaiheInitFusionRangingObserver(ani_vm *vm);
}  // namespace FusionRanging
}  // namespace OHOS
#endif /* TAIHE_FUSION_RANGING_OBSERVER_H */