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
#define LOG_TAG "PartnerDeviceAddress"
#endif

#include "partner_device_address.h"

#include "hilog/log.h"

using OHOS::HiviewDFX::HiLog;
namespace {
constexpr size_t ADDRESS_LENGTH = 17;
constexpr size_t ADDRESS_COLON_INDEX = 2;
constexpr size_t ADDRESS_SEPARATOR_UNIT = 3;
bool IsValidAddress(const std::string &addr)
{
    if (addr.empty() || addr.length() != ADDRESS_LENGTH) {
        return false;
    }
    for (size_t i = 0; i < ADDRESS_LENGTH; i++) {
        char c = addr[i];
        switch (i % ADDRESS_SEPARATOR_UNIT) {
            case 0:
            case 1:
                if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
                    break;
                }
                return false;
            case ADDRESS_COLON_INDEX:
            default:
                if (c == ':') {
                    break;
                }
                return false;
        }
    }
    return true;
}
}  // namespace



namespace OHOS {
namespace FusionConnectivity {
bool PartnerDeviceAddress::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteBool(isSupportBt_)) {
        return false;
    }
    if (isSupportBt_) {
        if (!parcel.WriteString(address_)) {
            return false;
        }
        if (!parcel.WriteInt32(static_cast<int32_t>(addressType_))) {
            return false;
        }
        if (!parcel.WriteBool(hasRawAddressType_)) {
            return false;
        }
        if (hasRawAddressType_) {
            if (!parcel.WriteInt32(static_cast<int32_t>(rawAddressType_))) {
                return false;
            }
        }
    }
    return true;
}

PartnerDeviceAddress *PartnerDeviceAddress::Unmarshalling(Parcel &parcel)
{
    PartnerDeviceAddress *deviceAddress = nullptr;
    bool isSupportBt = false;
    if (!parcel.ReadBool(isSupportBt)) {
        return nullptr;
    }
    if (isSupportBt) {
        std::string address = "";
        if (!parcel.ReadString(address)) {
            return nullptr;
        }
        if (!IsValidAddress(address)) {
            HILOG_ERROR(LOG_CORE, "Invalid address");
            return nullptr;
        }

        int32_t addressType;
        if (!parcel.ReadInt32(addressType)) {
            return nullptr;
        }
        if (addressType < static_cast<int32_t>(BluetoothAddressType::VIRTUAL) ||
            addressType > static_cast<int32_t>(BluetoothAddressType::REAL)) {
            HILOG_ERROR(LOG_CORE, "invalid addressType: %{public}d", addressType);
            return nullptr;
        }

        bool hasRawAddressType = false;
        if (!parcel.ReadBool(hasRawAddressType)) {
            return nullptr;
        }
        if (hasRawAddressType) {
            int32_t rawAddressType;
            if (!parcel.ReadInt32(rawAddressType)) {
                return nullptr;
            }
            if (rawAddressType < static_cast<int32_t>(BluetoothRawAddressType::PUBLIC) ||
                rawAddressType > static_cast<int32_t>(BluetoothRawAddressType::RANDOM)) {
                HILOG_ERROR(LOG_CORE, "invalid rawAddressType: %{public}d", rawAddressType);
                return nullptr;
            }
            deviceAddress = new PartnerDeviceAddress(address,
                static_cast<BluetoothAddressType>(addressType), static_cast<BluetoothRawAddressType>(rawAddressType));
        } else {
            deviceAddress = new PartnerDeviceAddress(address, static_cast<BluetoothAddressType>(addressType));
        }
    }
    return deviceAddress;
}
}  // namespace FusionConnectivity
}  // namespace OHOS
