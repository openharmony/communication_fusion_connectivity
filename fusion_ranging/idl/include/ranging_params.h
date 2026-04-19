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

#ifndef FUSION_RANGING_PARAMS_H
#define FUSION_RANGING_PARAMS_H

#include "parcel.h"
#include "fusion_ranging_types.h"

namespace OHOS {
namespace FusionRanging {

class RangingParams : public Parcelable {
public:
    RangingParams(std::string deviceId, RangingRole role, RangingTypes capabilityType)
        : deviceId_(deviceId),
          role_(role),
          capabilityType_(capabilityType)
    {
    }
    RangingParams() = default;
    ~RangingParams() = default;

    bool Marshalling(Parcel &parcel) const override;
    static RangingParams *Unmarshalling(Parcel &parcel);

    std::string GetDeviceId() const
    {
        return deviceId_;
    }

    void SetDeviceId(const std::string &deviceId)
    {
        deviceId_ = deviceId;
    }

    RangingRole GetRole() const
    {
        return role_;
    }

    void SetRole(RangingRole role)
    {
        role_ = role;
    }

    RangingTypes GetCapabilityType() const
    {
        return capabilityType_;
    }

    void SetCapabilityType(RangingTypes capabilityType)
    {
        capabilityType_ = capabilityType;
    }

private:
    std::string deviceId_ = "";
    RangingRole role_ = RangingRole::ROLE_INITIATOR;
    RangingTypes capabilityType_ = RangingTypes::NEARLINK_HADM;
};
}  // namespace FusionRanging
}  // namespace OHOS

#endif  // FUSION_RANGING_PARAMS_H