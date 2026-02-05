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

#ifndef EXTENSION_SERVICE_CONNECTION_NOTIFIER_H
#define EXTENSION_SERVICE_CONNECTION_NOTIFIER_H

#include "extension_service_common.h"
#include "pixel_map.h"
#include "notification_helper.h"
#include "notification_content.h"
#include "notification_request.h"
#include "fusion_conn_load_utils.h"

namespace OHOS {
namespace FusionConnectivity {

class ExtensionServiceConnectionNotifier {
public:
    static ExtensionServiceConnectionNotifier& GetInstance();
    
    void SendNotification(ExtensionSubscriberInfo &subscriberInfo, const NotificationType &notificationType,
        const std::string &notificationAddress, int32_t &notificationId);
    void CancelNotification(const int32_t notificationId);

private:
    ExtensionServiceConnectionNotifier() = default;
    void SetWantAgent(OHOS::Notification::NotificationRequest &request,
        const std::string &notificationAddress);
    OHOS::Notification::NotificationRequest request_;
    std::mutex requestMutex_;
    std::optional<std::shared_ptr<Media::PixelMap>> icon_;
    std::atomic<int32_t> notificationId_ {1};
};
} // namespace FusionConnectivity
} // namespace OHOS
#endif // EXTENSION_SERVICE_CONNECTION_NOTIFIER_H