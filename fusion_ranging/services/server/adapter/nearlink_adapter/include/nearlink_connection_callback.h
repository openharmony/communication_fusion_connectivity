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

#ifndef NEARLINK_CONNECTION_CALLBACK_H
#define NEARLINK_CONNECTION_CALLBACK_H

#include <string>
#include "nearlink_host.h"

namespace OHOS {
namespace FusionRanging {

class NearlinkConnectionCallback : public Nearlink::NearlinkRemoteDeviceObserver {
public:
    NearlinkConnectionCallback();
    ~NearlinkConnectionCallback();

    void OnConnectionStateChanged(const Nearlink::NearlinkRemoteDevice &device, int preState, int state,
                                  int reason) override;
    void OnPairStatusChanged(const Nearlink::NearlinkRemoteDevice &device, int preState, int state,
                             int reason) override;

    // Required pure virtual methods from NearlinkRemoteDeviceObserver
    void OnAcbStateChanged(const Nearlink::NearlinkRemoteDevice &device, int state, int reason) override;
    void OnPairingRequest(const Nearlink::NearlinkRemoteDevice &device, const std::string &passkey, int type) override;
    void OnRemoteUuidChanged(const Nearlink::NearlinkRemoteDevice &device,
                             const std::vector<Nearlink::UUID> &uuids) override;
    void OnRemoteNameChanged(const Nearlink::NearlinkRemoteDevice &device, const std::string &deviceName) override;
    void OnRemoteAliasChanged(const Nearlink::NearlinkRemoteDevice &device, const std::string &alias) override;
    void OnReadRemoteRssiEvent(const Nearlink::NearlinkRemoteDevice &device, int rssi, int status) override;
};
}  // namespace FusionRanging
}  // namespace OHOS
#endif  // NEARLINK_CONNECTION_CALLBACK_H