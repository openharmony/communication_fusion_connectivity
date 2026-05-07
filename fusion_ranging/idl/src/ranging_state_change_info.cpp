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

#include "ranging_state_change_info.h"

namespace OHOS {
namespace FusionRanging {

bool RangingStateChangeInfo::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteString(deviceId_)) {
        return false;
    }
    if (!parcel.WriteInt32(handle_)) {
        return false;
    }
    if (!parcel.WriteInt32(static_cast<int32_t>(state_))) {
        return false;
    }
    if (!parcel.WriteInt32(static_cast<int32_t>(cause_))) {
        return false;
    }
    return true;
}

RangingStateChangeInfo *RangingStateChangeInfo::Unmarshalling(Parcel &parcel)
{
    std::string deviceId = "";
    if (!parcel.ReadString(deviceId)) { /* deviceId empty for passive ranging */
        return nullptr;
    }
    int32_t handle = -1; /* default -1 is invalid handle */
    if (!parcel.ReadInt32(handle)) {
        return nullptr;
    }
    int32_t state = 0;
    if (!parcel.ReadInt32(state)) {
        return nullptr;
    }
    if (state < 0 || state > static_cast<int32_t>(RangingState::STATE_STARTED)) {
        return nullptr;
    }
    int32_t cause = 0;
    if (!parcel.ReadInt32(cause)) {
        return nullptr;
    }
    if (cause < 0 || cause > static_cast<int32_t>(RangingStoppedCause::BACKGROUND_PAUSED)) {
        return nullptr;
    }
    auto *info = new (std::nothrow) RangingStateChangeInfo(deviceId, handle, static_cast<RangingState>(state),
                                                           static_cast<RangingStoppedCause>(cause));
    return info;
}
}  // namespace FusionRanging
}  // namespace OHOS