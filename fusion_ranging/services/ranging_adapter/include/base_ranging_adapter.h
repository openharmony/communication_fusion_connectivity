/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributd under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef BASE_RANGING_ADAPTER_H
#define BASE_RANGING_ADAPTER_H

#include <cstdint>
#include <memory>
#include <string>
#include "fusion_ranging_types.h"

namespace OHOS {
namespace FusionRanging {

class AdapterRangingData {
public:
    AdapterRangingData() = default;
    AdapterRangingData(const std::string &deviceId, int32_t distance, int32_t rssi, int32_t angle)
        : deviceId_(deviceId),
          distance_(distance),
          angle_(angle),
          rssi_(rssi)
    {
    }

    ~AdapterRangingData() = default;

    std::string GetDeviceId() const
    {
        return deviceId_;
    }

    int32_t GetDistance() const
    {
        return distance_;
    }

    int32_t GetRssi() const
    {
        return rssi_;
    }

    int32_t GetAngle() const
    {
        return angle_;
    }

    int32_t GetConfidence() const
    {
        return confidence_;
    }

    void SetConfidence(int32_t confidence)
    {
        confidence_ = confidence;
    }

    bool IsValid() const
    {
        return isValid_;
    }

private:
    std::string deviceId_;
    int32_t distance_;
    int32_t angle_;
    int32_t rssi_;
    int32_t confidence_;
    bool isValid_{true};
};

class BaseRangingAdapterCallback {
public:
    virtual ~BaseRangingAdapterCallback() = default;
    virtual void OnRangingStateChange(int32_t state) = 0;
    virtual void OnRangingResult(const AdapterRangingData &data) = 0;
};

class BaseRangingAdapter {
public:
    BaseRangingAdapter() = default;
    virtual ~BaseRangingAdapter() = default;

    virtual int Init() = 0;
    virtual int DeInit() = 0;
    virtual int StartRanging(const std::string &deviceId, RangingRole role) = 0;
    virtual int StopRanging(const std::string &deviceId) = 0;
    virtual int PauseRanging(const std::string &deviceId) = 0;
    virtual int ResumeRanging(const std::string &deviceId) = 0;
    virtual int SetCallback(const std::shared_ptr<BaseRangingAdapterCallback> &callback) = 0;
};
} // namespace FusionRanging
} // namespace OHOS
#endif // BASE_RANGING_ADAPTER_H