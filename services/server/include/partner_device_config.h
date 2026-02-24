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

#ifndef PARTNER_DEVICE_AGENT_SERVER_H
#define PARTNER_DEVICE_AGENT_SERVER_H

#include "partner_device_agent_server.h"

namespace OHOS {
namespace FusionConnectivity {

#ifdef __cplusplus
extern "C" {
#endif

using CreatePartnerDeviceFunc = std::function<std::shared_ptr<PartnerDevice>(PartnerDevice::DeviceInfo &)>;

void UpdatePartnerDeviceConfig(PartnerDeviceMap &deviceMap);
void LoadPartnerDeviceConfig(PartnerDeviceMap &deviceMap, CreatePartnerDeviceFunc createPartnerDeviceFunc);
void ClearPartnerDeviceConfig();

#ifdef __cplusplus
}
#endif

}  // namespace FusionConnectivity
}  // namespace OHOS
#endif  // PARTNER_DEVICE_AGENT_SERVER_H
