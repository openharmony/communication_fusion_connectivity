/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of, use or distribute this file except in compliance with
 * the License. You may obtain a copy of the License at
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
#define LOG_TAG "taihe_fusion_connectivity_utils"
#endif

#include "taihe_fusion_connectivity_utils.h"
#include "log_utils.h"

namespace OHOS {
namespace FusionConnectivity {
constexpr size_t ADDRESS_LENGTH = 17;
constexpr size_t ADDRESS_COLON_INDEX = 2;
constexpr size_t ADDRESS_SEPARATOR_UNIT = 3;

bool IsValidAddr(const std::string &addr)
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

bool PartnerDeviceAddressToNative(
    const ::ohos::FusionConnectivity::partnerAgent::PartnerDeviceAddress &deviceAddress,
    PartnerDeviceAddress &outAddr)
{
    if (!deviceAddress.bluetoothAddress.has_value()) {
        HILOGE("bluetoothAddress property is not exist");
        return false;
    }
    auto bluetoothAddress = deviceAddress.bluetoothAddress.value();
    std::string address(bluetoothAddress.address);
    int32_t addressType = bluetoothAddress.addressType.get_value();
    int32_t rawAddressType;
    if (bluetoothAddress.rawAddressType.has_value()) {
        rawAddressType = bluetoothAddress.rawAddressType.value().get_value();
        outAddr = PartnerDeviceAddress(address,
            static_cast<BluetoothAddressType>(addressType), static_cast<BluetoothRawAddressType>(rawAddressType));
    } else {
        outAddr = PartnerDeviceAddress(address, static_cast<BluetoothAddressType>(addressType));
    }
    return true;
}

::ohos::FusionConnectivity::partnerAgent::PartnerDeviceAddress PartnerDeviceAddressToTaihe(
    const PartnerDeviceAddress &address)
{
    int32_t type = static_cast<int32_t>(address.GetAddressType());
    ::ohos::FusionConnectivity::partnerAgent::PartnerDeviceAddress result {};
    ohos::bluetooth::common::BluetoothAddress btAddr {
        .address = ::taihe::string(address.GetAddress().c_str()),
        .addressType = ohos::bluetooth::common::BluetoothAddressType::from_value(type)
    };

    if (address.HasRawAddressType()) {
        int32_t rawAddressType = static_cast<int32_t>(address.GetRawAddressType());
        btAddr.rawAddressType = ::taihe::optional<ohos::bluetooth::common::BluetoothRawAddressType>(
            std::in_place_t{}, ohos::bluetooth::common::BluetoothRawAddressType::from_value(rawAddressType));
    }
    result.bluetoothAddress = taihe::optional<ohos::bluetooth::common::BluetoothAddress>(std::in_place_t{}, btAddr);
    return result;
}

DeviceCapability DeviceCapabilityToNative(
    const ::ohos::FusionConnectivity::partnerAgent::DeviceCapability &capability)
{
    DeviceCapability result;
    if (capability.supportBR.has_value()) {
        result.isSupportBR = capability.supportBR.value();
    }
    if (capability.supportBleAdvertiser.has_value()) {
        result.isSupportBleAdvertiser = capability.supportBleAdvertiser.value();
    }
    return result;
}

BusinessCapability BusinessCapabilityToNative(
    const ::ohos::FusionConnectivity::partnerAgent::BusinessCapability &businessCapability)
{
    BusinessCapability result;
    if (businessCapability.supportMediaControl.has_value()) {
        result.isSupportMediaControl = businessCapability.supportMediaControl.value();
    }
    if (businessCapability.supportTelephonyControl.has_value()) {
        result.isSupportTelephonyControl = businessCapability.supportTelephonyControl.value();
    }
    return result;
}
}  // namespace FusionConnectivity
}  // namespace OHOS