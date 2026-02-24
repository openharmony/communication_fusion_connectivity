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

#ifndef PARTNER_DEVICE_AGENT_H
#define PARTNER_DEVICE_AGENT_H

#include <set>
#include <string>
#include "fusion_connectivity_def.h"
#include "fusion_connectivity_errorcode.h"
#include "partner_device_address.h"

namespace OHOS {
namespace FusionConnectivity {

class PartnerDeviceAgent {
public:
    static PartnerDeviceAgent *GetInstance();

    bool IsPartnerAgentSupported(void);
    int BindDevice(const PartnerDeviceAddress &deviceAddress, const DeviceCapability &capability,
        const BusinessCapability &businessCapability, const std::string &abilityName);
    int UnbindDevice(const PartnerDeviceAddress &deviceAddress);
    int IsDeviceBound(const PartnerDeviceAddress &deviceAddress, bool &isBound);
    int EnableDeviceControl(const PartnerDeviceAddress &deviceAddress);
    int DisableDeviceControl(const PartnerDeviceAddress &deviceAddress);
    int IsDeviceControlEnabled(const PartnerDeviceAddress &deviceAddress, bool &isEnabled);
    int GetBoundDevices(std::vector<PartnerDeviceAddress> &deviceAddressVec);

private:
    PartnerDeviceAgent();
    ~PartnerDeviceAgent();

    struct impl;
    std::unique_ptr<impl> pimpl;

    PartnerDeviceAgent(const PartnerDeviceAgent &) = delete;
    PartnerDeviceAgent &operator=(const PartnerDeviceAgent &) = delete;
};

}  // namespace PartnerDeviceAgent
}  // namespace OHOS
#endif // PARTNER_DEVICE_AGENT_H