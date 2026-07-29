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
#define LOG_TAG "NearlinkConnectionCallback"
#endif

#include "nearlink_connection_callback.h"
#include "connection_manager.h"
#include "nearlink_remote_device.h"
#include "fcm_thread_util.h"
#include "log_utils.h"

namespace OHOS {
namespace FusionRanging {
using namespace OHOS::Nearlink;

NearlinkConnectionCallback::NearlinkConnectionCallback() {}

NearlinkConnectionCallback::~NearlinkConnectionCallback() {}

void NearlinkConnectionCallback::OnConnectionStateChanged(const NearlinkRemoteDevice &device, int preState, int state,
                                                          int reason)
{
    /* SLE connection state is handled via OnAcbStateChanged (ACL encryption) and
       SsapClientCallbackImpl::OnConnectionStateChanged (SSAP layer), no action needed here */
}

void NearlinkConnectionCallback::OnPairStatusChanged(const NearlinkRemoteDevice &device, int preState, int state,
                                                     int reason)
{
    std::string deviceId = device.GetDeviceAddr();
    auto task = [deviceId, state]() {
        ConnectionManager::GetInstance()->OnPairStateChanged(deviceId, state);
    };
    FusionConnectivity::DoInRangingThread(task);
}

void NearlinkConnectionCallback::OnAcbStateChanged(const NearlinkRemoteDevice &device, int state, int reason)
{
    HILOGI("OnAcbStateChanged state:%{public}d, reason:%{public}d", state, reason);
    std::string deviceId = device.GetDeviceAddr();
    auto task = [deviceId, state]() {
        ConnectionManager::GetInstance()->OnAcbStateChanged(deviceId, state);
    };
    FusionConnectivity::DoInRangingThread(task);
}
void NearlinkConnectionCallback::OnPairingRequest(const Nearlink::NearlinkRemoteDevice &device,
                                                  const std::string &passkey, int type){};
void NearlinkConnectionCallback::OnRemoteUuidChanged(const Nearlink::NearlinkRemoteDevice &device,
                                                     const std::vector<Nearlink::UUID> &uuids){};
void NearlinkConnectionCallback::OnRemoteNameChanged(const Nearlink::NearlinkRemoteDevice &device,
                                                     const std::string &deviceName){};
void NearlinkConnectionCallback::OnRemoteAliasChanged(const Nearlink::NearlinkRemoteDevice &device,
                                                      const std::string &alias){};
void NearlinkConnectionCallback::OnReadRemoteRssiEvent(const Nearlink::NearlinkRemoteDevice &device, int rssi,
                                                       int status){};
}  // namespace FusionRanging
}  // namespace OHOS