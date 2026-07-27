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

#ifndef CONNECTION_MANAGER_H
#define CONNECTION_MANAGER_H

#include <string>
#include "base_def.h"
#include "nearlink_uuid.h"

namespace OHOS {
namespace FusionRanging {

class ConnectionManager {
public:
    static ConnectionManager *GetInstance();

    ConnectionManager();

    int Connect(const std::string &deviceId);
    int Disconnect(const std::string &deviceId);
    int StartPair(const std::string &deviceId);
    bool IsConnected(const std::string &deviceId);

    void OnPairStateChanged(const std::string &deviceId, int32_t state);
    void OnAcbStateChanged(const std::string &deviceId, int state);
    void OnSsapConnectionStateChanged(const std::string &deviceId, int connectionState, int ret);
    void OnSsapServiceDiscovery(const std::string &deviceId, int status, const Nearlink::UUID &uuid);

private:
    ~ConnectionManager();
    int ConnectWithSsap(const std::string &deviceId);
    void StartConnectTimeout(const std::string &deviceId);
    void StopConnectTimeout(const std::string &deviceId);

    DECLARE_IMPL();
};
}  // namespace FusionRanging
}  // namespace OHOS
#endif  // CONNECTION_MANAGER_H