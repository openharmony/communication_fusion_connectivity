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

#ifndef RANGING_MANAGER_H
#define RANGING_MANAGER_H

#include <string>
#include <mutex>
#include <atomic>
#include "nearlink_ranging_adapter.h"
#include "nearlink_sle_ranging.h"
#include "safe_map.h"

namespace OHOS {
namespace FusionRanging {

class NearlinkRangingCallback;
class NearlinkConnectionCallback;

constexpr int32_t SLE_RANGING_STOPPED = 0;
constexpr int32_t SLE_RANGING_STARTED = 1;

constexpr const char *RANGING_SERVICE_UUID = "00090000-0001-0005-0000-000000000000";
constexpr const char *RANGING_PROPERTY_UUID = "00090000-0001-0005-0001-000000000000";

enum class RangingAdapterState : int32_t { IDLE, CONNECTING, CONNECTED, RANGING };

class RangingDevice {
public:
    RangingDevice(const std::string &deviceId) : deviceId_(deviceId) {}

    void SetRangingState(RangingAdapterState state)
    {
        state_.store(state, std::memory_order_relaxed);
    }

    RangingAdapterState GetRangingState() const
    {
        return state_.load(std::memory_order_relaxed);
    }

    bool TryTransitionState(RangingAdapterState expected, RangingAdapterState desired)
    {
        return state_.compare_exchange_strong(expected, desired, std::memory_order_relaxed, std::memory_order_relaxed);
    }

private:
    std::string deviceId_ = "";
    std::atomic<RangingAdapterState> state_{RangingAdapterState::IDLE};
};

class RangingManager {
public:
    static RangingManager *GetInstance();

    int Init();
    int DeInit();
    int StartRanging(const std::string &deviceId);
    int StopRanging(const std::string &deviceId);
    int StartPassiveRanging(int32_t &handle);
    int StopPassiveRanging(int32_t handle);
    int PauseRanging(const std::string &deviceId);
    int ResumeRanging(const std::string &deviceId);
    int SetCallback(const std::shared_ptr<FusionRanging::BaseRangingAdapterCallback> &callback);

    void OnRangingStateChange(const std::string &deviceId, int32_t state);
    void OnRangingResult(const Nearlink::RangingResult &result);
    void OnConnectionStateChange(const std::string &deviceId, int32_t state);
    void NotifyPassiveRangingStateChanged(const std::string &deviceId, int32_t handle, int32_t state,
                                          int32_t cause = 0);

private:
    RangingManager();
    ~RangingManager();
    int StartRangingWithConnection(const std::string &deviceId);
    int StartSleRanging(const std::string &deviceId);
    int DoStartSleRanging(const std::string &deviceId);
    void NotifyRangingStateChanged(const std::string &deviceId, int32_t state, int32_t cause = 0);
    std::shared_ptr<RangingDevice> GetRangingDevice(const std::string &deviceId);
    void HandleConnectedState(std::shared_ptr<RangingDevice> &rangDev);
    void HandleDisconnectedState(const std::string &deviceId, std::shared_ptr<RangingDevice> &rangDev);

    std::shared_ptr<NearlinkRangingCallback> rangingCallback_{nullptr};
    std::shared_ptr<NearlinkConnectionCallback> connectionCallback_{nullptr};
    std::shared_ptr<Nearlink::NearlinkSleRanging> nearlinkRanging_;

    std::atomic<bool> isInitialized_{false};
    SafeMap<std::string, std::shared_ptr<RangingDevice>> rangingDev_{};
    mutable std::mutex mutex_{};
    std::shared_ptr<BaseRangingAdapterCallback> adapterCallback_{nullptr};
};
}  // namespace FusionRanging
}  // namespace OHOS
#endif  // RANGING_MANAGER_H