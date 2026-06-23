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

#ifndef BASE_RANGING_ADAPTER_H
#define BASE_RANGING_ADAPTER_H

#include <cstdint>
#include <memory>
#include <string>
#include "fusion_ranging_types.h"

namespace OHOS {
namespace FusionRanging {

enum AdapterRangingType {
    ADAPTER_RANGING,
    ADAPTER_PASSIVE_RANGING,
};

class AdapterRangingStateInfo {
public:
    AdapterRangingStateInfo() = default;
    AdapterRangingStateInfo(AdapterRangingType type, const std::string &deviceId, int32_t handle, RangingState state)
        : type_(type),
          deviceId_(deviceId),
          handle_(handle),
          state_(state)
    {
    }

    AdapterRangingType GetType() const
    {
        return type_;
    }

    std::string GetDeviceId() const
    {
        return deviceId_;
    }

    int32_t GetHandle() const
    {
        return handle_;
    }

    RangingState GetState() const
    {
        return state_;
    }

    void SetStoppedCause(RangingStoppedCause cause)
    {
        cause_ = cause;
    }

    RangingStoppedCause GetStoppedCause() const
    {
        return cause_;
    }

private:
    AdapterRangingType type_;
    std::string deviceId_ = "";
    int32_t handle_ = -1; /* -1 as invalid handle */
    RangingState state_ = RangingState::STATE_STOPPED;
    RangingStoppedCause cause_ = RangingStoppedCause::NO_ERROR;
};

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

private:
    std::string deviceId_;
    int32_t distance_;
    int32_t angle_;
    int32_t rssi_;
    int32_t confidence_ = 0;
};

class BaseRangingAdapterCallback {
public:
    virtual ~BaseRangingAdapterCallback() = default;
    virtual void OnRangingStateChange(const AdapterRangingStateInfo &Info) = 0;
    virtual void OnRangingResult(const AdapterRangingData &data) = 0;
};

class BaseRangingAdapter {
public:
    BaseRangingAdapter() = default;
    virtual ~BaseRangingAdapter() = default;

    virtual int Init() = 0;
    virtual int DeInit() = 0;
    virtual int StartRanging(const std::string &deviceId) = 0;
    virtual int StopRanging(const std::string &deviceId) = 0;
    virtual int StartPassiveRanging(int32_t &handle) = 0;
    virtual int StopPassiveRanging(int32_t handle) = 0;
    virtual int PauseRanging(const std::string &deviceId) = 0;
    virtual int ResumeRanging(const std::string &deviceId) = 0;
    virtual int SetCallback(const std::shared_ptr<BaseRangingAdapterCallback> &callback) = 0;
};
}  // namespace FusionRanging
}  // namespace OHOS
#endif  // BASE_RANGING_ADAPTER_H