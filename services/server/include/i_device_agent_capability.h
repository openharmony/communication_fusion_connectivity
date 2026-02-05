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

#ifndef I_DEVICE_AGENT_CAPABILITY_H
#define I_DEVICE_AGENT_CAPABILITY_H

#include <memory>
#include <functional>

namespace OHOS {
namespace FusionConnectivity {
class PartnerDevice;

class IDeviceAgentCapability {
public:
    struct DependencyFuncs {
        std::function<void(void)> startExtension;
        std::function<void(int)> destroyExtension;
    };

    IDeviceAgentCapability(DependencyFuncs funcs, std::weak_ptr<PartnerDevice> ownerWptr)
        : funcs_(funcs), ownerWptr_(ownerWptr) {}
    virtual ~IDeviceAgentCapability() = default;

    virtual void Init(const std::string &addr) = 0;
    virtual void Close() = 0;
    virtual void OnBluetoothDeviceAclConnected() = 0;
    virtual void OnBluetoothDeviceAclDisconnected() = 0;
    virtual void OnScreenOn() = 0;
    virtual void OnScreenOff() = 0;
    virtual void OnExtensionDestroy() = 0;

protected:
    DependencyFuncs funcs_;
    std::weak_ptr<PartnerDevice> ownerWptr_;
};
}
}
#endif  // I_DEVICE_AGENT_CAPABILITY_H
