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

#include "ranging_measurement.h"

namespace OHOS {
namespace FusionRanging {

bool RangingMeasurement::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteBool(isValid_)) {
        return false;
    }
    if (!parcel.WriteInt32(value_)) {
        return false;
    }
    if (!parcel.WriteInt32(static_cast<int32_t>(confidence_))) {
        return false;
    }
    return true;
}

RangingMeasurement *RangingMeasurement::Unmarshalling(Parcel &parcel)
{
    bool isValid = false;
    if (!parcel.ReadBool(isValid)) {
        return nullptr;
    }
    int32_t value = 0;
    if (!parcel.ReadInt32(value)) {
        return nullptr;
    }
    int32_t confidence = 0;
    if (!parcel.ReadInt32(confidence)) {
        return nullptr;
    }
    auto *measurement = new (std::nothrow)
        RangingMeasurement(isValid, value, static_cast<RangingConfidence>(confidence));
    return measurement;
}
}  // namespace FusionRanging
}  // namespace OHOS