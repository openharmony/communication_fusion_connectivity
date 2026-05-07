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

#ifndef FUSION_RANGING_STATE_CHANGE_INFO_H
#define FUSION_RANGING_STATE_CHANGE_INFO_H

#include "parcel.h"
#include "fusion_ranging_types.h"

namespace OHOS {
namespace FusionRanging {

class RangingStateChangeInfo : public Parcelable {
public:
    RangingStateChangeInfo(const std::string &deviceId, int32_t handle, RangingState state, RangingStoppedCause cause)
        : deviceId_(deviceId),
          handle_(handle),
          state_(state),
          cause_(cause)
    {
    }
    RangingStateChangeInfo() = default;
    ~RangingStateChangeInfo() = default;

    bool Marshalling(Parcel &parcel) const override;
    static RangingStateChangeInfo *Unmarshalling(Parcel &parcel);

    std::string GetDeviceId() const
    {
        return deviceId_;
    }

    void SetDeviceId(const std::string &deviceId)
    {
        deviceId_ = deviceId;
    }

    int32_t GetHandle() const
    {
        return handle_;
    }

    void SetHandle(int32_t handle)
    {
        handle_ = handle;
    }

    RangingState GetState() const
    {
        return state_;
    }

    void SetState(RangingState state)
    {
        state_ = state;
    }

    RangingStoppedCause GetCause() const
    {
        return cause_;
    }

    void SetCause(RangingStoppedCause cause)
    {
        cause_ = cause;
    }

private:
    std::string deviceId_ = "";
    int32_t handle_ = -1; /* default -1 invalid handle */
    RangingState state_ = RangingState::STATE_STOPPED;
    RangingStoppedCause cause_ = RangingStoppedCause::NO_ERROR;
};
}  // namespace FusionRanging
}  // namespace OHOS
#endif  // FUSION_RANGING_STATE_CHANGE_INFO_H