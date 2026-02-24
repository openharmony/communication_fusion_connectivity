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

#ifndef PARTNER_DEVICE_ADDRESS_H
#define PARTNER_DEVICE_ADDRESS_H

#include "parcel.h"
#include "ifusion_connectivity_types.h"

namespace OHOS {
namespace FusionConnectivity {
class PartnerDeviceAddress : public Parcelable {
public:
    PartnerDeviceAddress(std::string address, BluetoothAddressType addressType)
        : isSupportBt_(true), address_(address), addressType_(addressType), hasRawAddressType_(false) {}
    PartnerDeviceAddress(std::string address, BluetoothAddressType addressType, BluetoothRawAddressType rawAddressType)
        : isSupportBt_(true), address_(address), addressType_(addressType),
        hasRawAddressType_(true), rawAddressType_(rawAddressType) {}
    PartnerDeviceAddress() = default;
    ~PartnerDeviceAddress() = default;

    bool Marshalling(Parcel &parcel) const override;
    static PartnerDeviceAddress *Unmarshalling(Parcel &parcel);

    std::string GetAddress() const
    {
        return address_;
    }
    BluetoothAddressType GetAddressType() const
    {
        return addressType_;
    }

    bool HasRawAddressType() const
    {
        return hasRawAddressType_;
    }
    BluetoothRawAddressType GetRawAddressType() const
    {
        return rawAddressType_;
    }

private:
    bool isSupportBt_ = false;
    std::string address_ = "";
    BluetoothAddressType addressType_ = BluetoothAddressType::VIRTUAL;
    bool hasRawAddressType_ = false;
    BluetoothRawAddressType rawAddressType_ = BluetoothRawAddressType::PUBLIC;
};

}  // namespace FusionConnectivity
}  // namespace OHOS
#endif  // PARTNER_DEVICE_ADDRESS_H
