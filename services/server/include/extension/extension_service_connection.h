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

#ifndef EXTENSION_SERVICE_CONNECTION_H
#define EXTENSION_SERVICE_CONNECTION_H

#include "ffrt.h"

#include "ability_connect_callback_stub.h"
#include "extension_service_common.h"
#include "extension_service_connection_notifier.h"
#include "fusion_conn_load_utils.h"
#include "remote_death_recipient.h"
#include "partner_agent_extension_proxy.h"
#include "partner_device_address.h"

namespace OHOS {
namespace FusionConnectivity {

class ExtensionServiceConnectionService;

class ExtensionServiceConnection : public AAFwk::AbilityConnectionStub {
public:
    ExtensionServiceConnection(const ExtensionSubscriberInfo& subscriberInfo,
        std::function<void(const ExtensionSubscriberInfo& subscriberInfo)> onDisconnected);
    virtual ~ExtensionServiceConnection();
    void Close();
    void NotifyOnDeviceDiscovered(const PartnerDeviceAddress& deviceAddress);
    void NotifyOnDestroyWithReason(const int32_t reason, const std::shared_ptr<ExtensionSubscriberInfo> subscriberInfo);
    void SetNotificationType(const NotificationType& notificationType);

    /**
     * OnAbilityConnectDone, Ability Manager Service notify caller ability the result of connect.
     *
     * @param element, Indicates elementName of service ability.
     * @param remoteObject, Indicates the session proxy of service ability.
     * @param resultCode, Returns ERR_OK on success, others on failure.
     */
    void OnAbilityConnectDone(
        const AppExecFwk::ElementName &element, const sptr<IRemoteObject> &remoteObject, int resultCode) override;

    /**
     * OnAbilityDisconnectDone, Ability Manager Service notify caller ability the result of disconnect.
     *
     * @param element, Indicates elementName of service ability.
     * @param resultCode, Returns ERR_OK on success, others on failure.
     */
    void OnAbilityDisconnectDone(const AppExecFwk::ElementName &element, int resultCode) override;

public:
    std::weak_ptr<ExtensionServiceConnectionService> connectionService_;

private:
    void HandleDisconnectedState();
    void OnRemoteDied(const wptr<IRemoteObject> &remote);
    static void AppendMessage(std::string& message, ErrCode callResult, int32_t retResult);
    void SetNotificationId(int32_t notificationId);

    enum class NotifyType {
        OnDeviceDiscovered,
        OnDestroyWithReason
    };
    struct NotifyParam {
        PartnerDeviceAddress deviceAddress;
        int32_t reason = 0;
        std::shared_ptr<ExtensionSubscriberInfo> subscriberInfo = nullptr;
    };
    sptr<PartnerAgentExtensionProxy> proxy_ = nullptr;
    ffrt::recursive_mutex mutex_;
    std::vector<std::pair<NotifyType, NotifyParam>> messages_;
    std::shared_ptr<ffrt::queue> messageQueue_ = nullptr;
    ExtensionServiceConnectionState state_ = ExtensionServiceConnectionState::CREATED;
    std::string connectionKey_ = "";
    uint64_t timerIdFreeze_ = 0L;
    uint64_t timerIdDisconnect_ = 0L;
    ExtensionSubscriberInfo subscriberInfo_;
    int32_t pid_ = -1;
    sptr<IRemoteObject> remoteObject_ = nullptr;
    sptr<RemoteDeathRecipient> deathRecipient_ = nullptr;
    PartnerDeviceAddress deviceAddress_;
    std::atomic<int32_t> notificationId_ {0};
    NotificationType notificationType_ = NotificationType::MEDIA_AND_TELEPHONY_CONTROL;
    std::function<void(const ExtensionSubscriberInfo& subscriberInfo)> onDisconnected_;
};
}
}
#endif // EXTENSION_SERVICE_CONNECTION_H
