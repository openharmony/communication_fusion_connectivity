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

#ifndef LOG_TAG
#define LOG_TAG "NearlinkRangingCallback"
#endif

#include "nearlink_ranging_callback.h"
#include "ranging_manager.h"
#include "fcm_thread_util.h"
#include "log_utils.h"

namespace OHOS {
namespace FusionRanging {

NearlinkRangingCallback::NearlinkRangingCallback() {}

NearlinkRangingCallback::~NearlinkRangingCallback() {}

void NearlinkRangingCallback::OnSleRangingResult(const Nearlink::RangingResult &result)
{
    auto task = [result]() {
        RangingManager::GetInstance()->OnRangingResult(result);
    };
    FusionConnectivity::DoInRangingThread(task);
}

void NearlinkRangingCallback::OnSleRangingStateChange(const Nearlink::RangingState &state)
{
    HILOGI("OnSleRangingStateChange state: %{public}d", state.GetNewState());
    std::string deviceId = state.GetAddress();
    int32_t newState = state.GetNewState();
    auto task = [deviceId, newState]() {
        RangingManager::GetInstance()->OnRangingStateChange(deviceId, newState);
    };
    FusionConnectivity::DoInRangingThread(task);
}
}  // namespace FusionRanging
}  // namespace OHOS