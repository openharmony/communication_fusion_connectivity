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

#ifndef FUSION_RANGING_RESULT_H
#define FUSION_RANGING_RESULT_H

#include "parcel.h"
#include "ranging_measurement.h"

namespace OHOS {
namespace FusionRanging {

class RangingResult : public Parcelable {
public:
    RangingResult() = default;
    ~RangingResult() = default;

    bool Marshalling(Parcel &parcel) const override;
    static RangingResult *Unmarshalling(Parcel &parcel);

    std::string GetDeviceId() const
    {
        return deviceId_;
    }

    void SetDeviceId(const std::string &deviceId)
    {
        deviceId_ = deviceId;
    }

    const RangingMeasurement &GetDistance() const
    {
        return distance_;
    }

    void SetDistance(const RangingMeasurement &distance)
    {
        distance_ = distance;
    }

    const RangingMeasurement &GetAngle() const
    {
        return angle_;
    }

    void SetAngle(const RangingMeasurement &angle)
    {
        angle_ = angle;
    }

    int32_t GetRssi() const
    {
        return rssi_;
    }

    void SetRssi(int32_t rssi)
    {
        rssi_ = rssi;
    }

private:
    std::string deviceId_ = "";
    RangingMeasurement distance_;
    RangingMeasurement angle_;
    int32_t rssi_ = 0;
};
}  // namespace FusionRanging
}  // namespace OHOS
#endif  // FUSION_RANGING_RESULT_H