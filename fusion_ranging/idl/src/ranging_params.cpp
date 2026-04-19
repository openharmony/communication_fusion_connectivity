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

#include "ranging_params.h"

namespace OHOS {
namespace FusionRanging {

bool RangingParams::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteString(deviceId_)) {
        return false;
    }
    if (!parcel.WriteInt32(static_cast<int32_t>(role_))) {
        return false;
    }
    if (!parcel.WriteInt32(static_cast<int32_t>(capabilityType_))) {
        return false;
    }
    return true;
}

RangingParams *RangingParams::Unmarshalling(Parcel &parcel)
{
    std::string deviceId = "";
    if (!parcel.ReadString(deviceId)) {
        return nullptr;
    }
    int32_t role = 0;
    if (!parcel.ReadInt32(role)) {
        return nullptr;
    }
    int32_t capabilityType = 0;
    if (!parcel.ReadInt32(capabilityType)) {
        return nullptr;
    }
    auto *params = new (std::nothrow)
        RangingParams(deviceId, static_cast<RangingRole>(role), static_cast<RangingTypes>(capabilityType));
    return params;
}
}  // namespace FusionRanging
}  // namespace OHOS