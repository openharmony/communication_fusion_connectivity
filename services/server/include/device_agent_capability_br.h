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

#ifndef DEVICE_AGENT_CAPABILITY_BR_H
#define DEVICE_AGENT_CAPABILITY_BR_H

#include "i_device_agent_capability.h"
#include <mutex>
#include "timer_manager.h"

namespace OHOS {
namespace FusionConnectivity {
class DeviceAgentCapabilityBr : public IDeviceAgentCapability,
                                public std::enable_shared_from_this<DeviceAgentCapabilityBr> {
public:
    DeviceAgentCapabilityBr(DependencyFuncs funcs, std::weak_ptr<PartnerDevice> ownerWptr)
        : IDeviceAgentCapability(funcs, ownerWptr) {}
    ~DeviceAgentCapabilityBr() override = default;

    void Init(const std::string &addr) override;
    void Close() override;
    void OnBluetoothDeviceAclConnected() override;
    void OnBluetoothDeviceAclDisconnected() override;
    void OnScreenOn() override {}
    void OnScreenOff() override {}
    void OnExtensionDestroy() override {}

private:
    std::atomic_bool isInit_ { false };

    std::mutex timerMutex_;
    int extensionKeepAliveTimeout_ = 3 * 60 * 1000;  // 3min
    std::unique_ptr<Timer> aclDisconnectTimer_ { nullptr };
};
}  // namespace FusionConnectivity
}  // namespace OHOS
#endif  // DEVICE_AGENT_CAPABILITY_BR_H
