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

#ifndef TAIHE_FUSION_CONNECTIVITY_UTILS_H
#define TAIHE_FUSION_CONNECTIVITY_UTILS_H

#include "ohos.FusionConnectivity.partnerAgent.impl.hpp"
#include "ohos.FusionConnectivity.partnerAgent.proj.hpp"
#include "taihe/runtime.hpp"
#include "stdexcept"
#include "partner_device_address.h"

namespace OHOS {
namespace FusionConnectivity {
bool IsValidAddr(const std::string &addr);
bool PartnerDeviceAddressToNative(
    const ::ohos::FusionConnectivity::partnerAgent::PartnerDeviceAddress &deviceAddress,
    PartnerDeviceAddress &outAddr);
::ohos::FusionConnectivity::partnerAgent::PartnerDeviceAddress PartnerDeviceAddressToTaihe(
    const PartnerDeviceAddress &address);
DeviceCapability DeviceCapabilityToNative(
    const ::ohos::FusionConnectivity::partnerAgent::DeviceCapability &capability);
BusinessCapability BusinessCapabilityToNative(
    const ::ohos::FusionConnectivity::partnerAgent::BusinessCapability &businessCapability);
}  // namespace FusionConnectivity
}  // namespace OHOS

#endif  // TAIHE_FUSION_CONNECTIVITY_UTILS_H