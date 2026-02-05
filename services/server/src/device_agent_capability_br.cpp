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
#define LOG_TAG "DeviceAgentCapBr"
#endif

#include "device_agent_capability_br.h"
#include "log_util.h"
#include "bluetooth_host.h"
#include "partner_device.h"

namespace OHOS {
namespace FusionConnectivity {
using namespace OHOS::Bluetooth;

void DeviceAgentCapabilityBr::Init(const std::string &addr)
{
    HILOGI("Init br capability");
    isInit_ = true;
}

void DeviceAgentCapabilityBr::Close()
{
    HILOGI("Close br capability");
    isInit_ = false;
}

void DeviceAgentCapabilityBr::OnBluetoothDeviceAclConnected()
{
    if (!isInit_.load()) {
        return;
    }

    funcs_.startExtension();

    // ACL连接成功，停止该设备的连接超时定时器
    std::lock_guard<std::mutex> lock(timerMutex_);
    aclDisconnectTimer_ = nullptr;
}

void DeviceAgentCapabilityBr::OnBluetoothDeviceAclDisconnected()
{
    if (!isInit_.load()) {
        return;
    }

    // ACL断连3分钟后，销毁Extension，并重启开启BLE扫描
    auto func = [ptr = weak_from_this()]() {
        auto capabilitySptr = ptr.lock();
        if (capabilitySptr) {
            HILOGI("destroy extension");
            capabilitySptr->funcs_.destroyExtension(ABILITY_DESTROY_DEVICE_LOST);
        };
    };
    std::lock_guard<std::mutex> lock(timerMutex_);
    aclDisconnectTimer_ = std::make_unique<Timer>(func);
    aclDisconnectTimer_->Start(extensionKeepAliveTimeout_);
}
}  // namespace FusionConnectivity
}  // namespace OHOS
