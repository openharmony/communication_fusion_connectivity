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

#ifndef OHOS_PARTNER_DEVICE_AGENT_SERVER_H
#define OHOS_PARTNER_DEVICE_AGENT_SERVER_H

#include "base_def.h"
#include "iservice_registry.h"
#include "if_system_ability_manager.h"
#include "system_ability.h"
#include "partner_device_agent_stub.h"
#include "permission_manager.h"
#include "fusion_conn_load_utils.h"
#include "partner_device.h"
#include "safe_map.h"

namespace OHOS {
namespace FusionConnectivity {
using PartnerDeviceMapKey = std::pair<uint32_t, std::string>;
using PartnerDeviceMap = SafeMap<PartnerDeviceMapKey, std::shared_ptr<PartnerDevice>>;

class PartnerDeviceAgentServer : public SystemAbility, public PartnerDeviceAgentStub {
    DECLARE_SYSTEM_ABILITY(PartnerDeviceAgentServer);
public:
    ~PartnerDeviceAgentServer() override;
    static sptr<PartnerDeviceAgentServer> GetInstance();
    void OnStart() override;
    void OnStop() override;
    int32_t OnIdle(const SystemAbilityOnDemandReason& idleReason) override;
    // Before IPC OnRemoteRequestInner, Used for check permission
    int32_t CallbackEnter(uint32_t code) override;
    // After IPC OnRemoteRequestInner
    int32_t CallbackExit([[maybe_unused]] uint32_t code, [[maybe_unused]] int32_t result) override;

    ErrCode BindDevice(const PartnerDeviceAddress &deviceAddress, const DeviceCapability &capability,
        const BusinessCapability &businessCapability, const std::string &partnerAgentExtensionAbilityName) override;
    ErrCode UnbindDevice(const PartnerDeviceAddress &deviceAddress) override;
    ErrCode IsDeviceBound(const PartnerDeviceAddress &deviceAddress, bool &isBound) override;
    ErrCode GetBoundDevices(std::vector<PartnerDeviceAddress> &deviceAddressVec) override;
    ErrCode EnableDeviceControl(const PartnerDeviceAddress &deviceAddress) override;
    ErrCode DisableDeviceControl(const PartnerDeviceAddress &deviceAddress) override;
    ErrCode IsDeviceControlEnabled(const PartnerDeviceAddress &deviceAddress, bool &isEnabled) override;

private:
    PartnerDeviceAgentServer();
    int32_t ConnectExtensionService(const std::string &bundleName, const std::string &abilityName);
    int32_t OnDeviceDiscoveredExtensionService(
        const std::string &bundleName, const std::string &abilityName, PartnerDeviceAddress deviceAddress);
    int32_t OnDestroyWithReasonExtensionService(
        const std::string &bundleName, const std::string &abilityName, int destroyReason);

    bool IsDeviceBoundByCallingApp(const PartnerDeviceAddress &deviceAddress);
    bool IsDeviceBoundByAll(const PartnerDeviceAddress &deviceAddress);
    bool IsPairedDevice(const PartnerDeviceAddress &deviceAddress);

    void Init();
    std::shared_ptr<PartnerDevice> CreatePartnerDeviceInstance(PartnerDevice::DeviceInfo &deviceInfo);
    void AttemptUnloadPartnerAgent();
    int ChangeDeviceControlState(const std::string &addr, bool isEnabled);

    std::map<int, PermissionItem> permissionsMap_ {};

    static std::mutex instanceMutex_;
    static sptr<PartnerDeviceAgentServer> instance_;

    std::atomic<bool> partnerAgentExtensionLoaded_ = false;
    std::shared_ptr<FusionConnectivityLoadUtils> partnerAgentExtensionHandler_;
    PartnerDeviceMap partnerDeviceMap_;

    DECLARE_IMPL();
};

}  // namespace FusionConnectivity
}  // namespace OHOS
#endif // OHOS_PARTNER_DEVICE_AGENT_SERVER_H