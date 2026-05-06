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
#include <string>
#include <future>

#include "fusion_ranging_types.h"
#include "ranging_params.h"
#include "ranging_result.h"
#include "base_ranging_adapter.h"
#include "safe_map.h"

namespace OHOS {
namespace FusionRanging {

using RangingResultCallback = std::function<void(const RangingResult &)>;
struct RangingDeviceInfo {
    RangingDeviceInfo(int32_t callerUid, const RangingParams &p, RangingResultCallback cb, RangingState st)
        : callerUid_(callerUid),
          params_(p.GetDeviceId(), p.GetCapabilityType()),
          callback_(cb),
          state_(st)
    {
    }
    int32_t callerUid_;
    RangingParams params_;
    RangingResultCallback callback_;
    RangingState state_;
};

struct PassiveRangingHandle {
    PassiveRangingHandle(int32_t uid, RangingTypes cap, int32_t advHandle)
        : callerUid_(uid),
          cap_(cap),
          advHandle_(advHandle)
    {
    }
    int32_t callerUid_;
    RangingTypes cap_;
    int32_t advHandle_;
};

class FusionRangingService {
public:
    static FusionRangingService *GetInstance();
    FusionRangingService();
    virtual ~FusionRangingService();

    bool IsRangingSupported(RangingTypes capabilityType);
    int StartRanging(const RangingParams &params, const std::function<void(const RangingResult &)> &callback,
                     int32_t callerUid);
    int StopRanging(const std::string &deviceId, int32_t callerUid);
    int StartPassiveRanging(RangingTypes capabilityType, int32_t &handle, int32_t callerUid);
    int StopPassiveRanging(RangingTypes capabilityType, int32_t handle, int32_t callerUid);
    void OnAdapterRangingStateChanged(int32_t state);
    void OnRangingResult(const RangingResult &result);

    void HandleProcessDeath(int32_t uid);
    void PauseRangingByUid(int32_t uid);
    void ResumeRangingByUid(int32_t uid);
    bool IsRangingEmpty();

private:
    void InitConfiguration();
    void DeInitConfiguration();
    int CreateRangingAdapter(RangingTypes capabilityType);
    int HandleStartRanging(const RangingParams &params, const std::function<void(const RangingResult &)> &callback,
                           int32_t callerUid);
    int HandleStartPassiveRanging(RangingTypes capabilityType, int32_t &advHandle, int32_t callerUid);
    int PauseRanging(const std::string &deviceId);
    int ResumeRanging(const std::string &deviceId);

    std::shared_ptr<BaseRangingAdapter> GetAdapter(RangingTypes capabilityType);
    std::shared_ptr<RangingDeviceInfo> GetRangingDevice(const std::string &deviceId);
    std::shared_ptr<PassiveRangingHandle> GetPassiveRangingHandle(int32_t handle);

    SafeMap<RangingTypes, std::shared_ptr<BaseRangingAdapter>> adapters_;
    SafeMap<std::string, std::shared_ptr<RangingDeviceInfo>> devicesInfo_;
    SafeMap<int32_t, std::shared_ptr<PassiveRangingHandle>> advHandles_;
};
}  // namespace FusionRanging
}  // namespace OHOS
#endif  // FUSION_RANGING_SERVICE_H