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

#ifndef EXTENSION_SERVICE_CONNECTION_SERVICE_H
#define EXTENSION_SERVICE_CONNECTION_SERVICE_H

#include "ffrt.h"

#include <mutex>
#include <refbase.h>
#include "extension_service_common.h"
#include "extension_service_connection.h"

namespace OHOS {
namespace FusionConnectivity {
class ExtensionServiceConnectionService : public std::enable_shared_from_this<ExtensionServiceConnectionService> {
public:
    static std::shared_ptr<ExtensionServiceConnectionService> GetInstance();
    void NotifyOnDeviceDiscovered(const std::shared_ptr<ExtensionSubscriberInfo> subscriberInfo,
        const PartnerDeviceAddress& deviceAddress, const NotificationType& type);
    void NotifyOnDestroyWithReason(const std::shared_ptr<ExtensionSubscriberInfo> subscriberInfo,
        const int32_t reason);
    void CloseConnection(const std::shared_ptr<ExtensionSubscriberInfo> subscriberInfo);
    void RemoveConnection(const ExtensionSubscriberInfo& subscriberInfo);
    sptr<ExtensionServiceConnection> GetConnection(
        const std::shared_ptr<ExtensionSubscriberInfo> subscriberInfo);
    int32_t Connect(const std::shared_ptr<ExtensionSubscriberInfo> subscriberInfo);
private:
    ffrt::recursive_mutex mapLock_;
    //map : ExtensionSubscriberInfo::Key
    std::map<std::string, sptr<ExtensionServiceConnection>> connectionMap_;
    static std::mutex instanceMutex_;
    static std::shared_ptr<ExtensionServiceConnectionService> instance_;
};
}
}
#endif // EXTENSION_SERVICE_CONNECTION_SERVICE_H
