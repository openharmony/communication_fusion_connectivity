/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributd under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FUSION_RANGING_MEASUREMENT_H
#define FUSION_RANGING_MEASUREMENT_H

#include "parcel.h"
#include "fusion_ranging_types.h"

namespace OHOS {
namespace FusionRanging {

class RangingMeasurement : public Parcelable {
public:
    RangingMeasurement(bool isVaild, int32_t value, RangingConfidence confidence)
        : isVaild_(isVaild),
          value_(value),
          confidence_(confidence)
    {
    }
    RangingMeasurement() = default;
    ~RangingMeasurement() = default;

    bool Marshalling(Parcel &parcel) const override;
    static RangingMeasurement *Unmarshalling(Parcel &parcel);

    bool GetIsValid() const
    {
        return isVaild_;
    }

    void SetIsValid(bool isVaild)
    {
        isVaild_ = isVaild;
    }

    int32_t GetValue() const
    {
        return value_;
    }

    void SetValue(int32_t value)
    {
        value_ = value;
    }

    RangingConfidence GetConfidence() const
    {
        return confidence_;
    }

    void SetConfidence(RangingConfidence confidence)
    {
        confidence_ = confidence;
    }

private:
    bool isVaild_ = false;
    int value_ = 0;
    RangingConfidence confidence_ = RangingConfidence::MEDIUM;
};
} // namespace FusionRanging
} // namespace OHOS

#endif // FUSION_RANGING_MEASUREMENT_H