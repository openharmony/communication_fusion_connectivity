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

#ifndef FUSION_RANGING_SERVICE_H
#define FUSION_RANGING_SERVICE_H

#include <memory>
#include <mutex>
#include <string>
#include <map>

#include "fusion_ranging_types.h"
#include "ranging_params.h"
#include "ranging_result.h"
#include "base_ranging_adapter.h"

namespace OHOS {
namespace FusionRanging {

struct RangingDeviceInfo {
    RangingParams params;
    std::function<void(const RangingResult &)> callback;
    RangingState state;
};

class FusionRangingService : public std::enable_shared_from_this<FusionRangingService> {
public:
    static std::shared_ptr<FusionRangingService> GetInstance();
    FusionRangingService();
    virtual ~FusionRangingService();

    int StartRanging(const RangingParams &params, const std::function<void(const RangingResult &)> &callback);
    int StopRanging(const std::string &deviceId);
    int PauseRanging(const std::string &deviceId);
    int ResumeRanging(const std::string &deviceId);
    void OnAdapterRangingStateChanged(int32_t state);
    void OnRangingResult(const RangingResult &result);

private:
    void InitConfiguration();
    void DeInitConfiguration();
    int CreateRangingAdapter(const RangingParams &params);
    std::shared_ptr<BaseRangingAdapter> GetAdapter(RangingTypes capabilityType);

    std::mutex rangingMutex_;
    std::map<std::string, RangingDeviceInfo> rangingData_;
    std::map<RangingTypes, std::shared_ptr<BaseRangingAdapter>> rangingAdapters_;
};

}  // namespace FusionRanging
}  // namespace OHOS
#endif  // FUSION_RANGING_SERVICE_H