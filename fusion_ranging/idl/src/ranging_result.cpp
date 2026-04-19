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

#include "ranging_result.h"

namespace OHOS {
namespace FusionRanging {

bool RangingResult::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteString(deviceId_)) {
        return false;
    }
    if (!distance_.Marshalling(parcel)) {
        return false;
    }
    if (!angle_.Marshalling(parcel)) {
        return false;
    }
    if (!parcel.WriteInt32(rssi_)) {
        return false;
    }
    return true;
}

RangingResult *RangingResult::Unmarshalling(Parcel &parcel)
{
    auto *result = new (std::nothrow) RangingResult();
    if (result == nullptr) {
        return nullptr;
    }
    std::string deviceId = "";
    if (!parcel.ReadString(deviceId)) {
        delete result;
        return nullptr;
    }
    result->SetDeviceId(deviceId);
    auto *distance = RangingMeasurement::Unmarshalling(parcel);
    if (distance == nullptr) {
        delete result;
        return nullptr;
    }
    result->SetDistance(*distance);
    delete distance;
    auto *angle = RangingMeasurement::Unmarshalling(parcel);
    if (angle == nullptr) {
        delete result;
        return nullptr;
    }
    result->SetAngle(*angle);
    delete angle;
    int32_t rssi = 0;
    if (!parcel.ReadInt32(rssi)) {
        delete result;
        return nullptr;
    }
    result->SetRssi(rssi);
    return result;
}
}  // namespace FusionRanging
}  // namespace OHOS