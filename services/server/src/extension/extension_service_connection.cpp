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
#define LOG_TAG "ExtensionServiceConnection"
#endif

#include "ability_manager_client.h"
#include "extension_service_connection.h"
#include "extension_service_connection_service.h"

namespace OHOS {
namespace FusionConnectivity {

ExtensionServiceConnection::ExtensionServiceConnection(const ExtensionSubscriberInfo& subscriberInfo,
    std::function<void(const ExtensionSubscriberInfo& subscriberInfo)> onDisconnected)
    : subscriberInfo_(subscriberInfo), onDisconnected_(onDisconnected)
{
    messageQueue_ = std::make_shared<ffrt::queue>("ExtensionServiceConnection");

    deathRecipient_ = new (std::nothrow)
        RemoteDeathRecipient(std::bind(&ExtensionServiceConnection::OnRemoteDied, this, std::placeholders::_1));
    if (deathRecipient_ == nullptr) {
        HILOGE("Failed to create RemoteDeathRecipient instance");
    }
}

ExtensionServiceConnection::~ExtensionServiceConnection()
{
    HILOGI("~ExtensionServiceConnection call");
}

void ExtensionServiceConnection::Close()
{
    std::lock_guard<ffrt::recursive_mutex> lock(mutex_);
    if (state_ == ExtensionServiceConnectionState::CREATED ||
        state_ == ExtensionServiceConnectionState::DISCONNECTED) {
        HandleDisconnectedState();
    } else {
        AAFwk::AbilityManagerClient::GetInstance()->DisconnectAbility(this);
    }
}

void ExtensionServiceConnection::NotifyOnDeviceDiscovered(const PartnerDeviceAddress& deviceAddress)
{
    HILOGI("NotifyOnDeviceDiscovered call");
    wptr<ExtensionServiceConnection> wThis = this;
    messageQueue_->submit([wThis, deviceAddress]() {
        sptr<ExtensionServiceConnection> sThis = wThis.promote();
        if (!sThis) {
            HILOGE("null sThis");
            return;
        }
        if (deviceAddress.GetAddress().empty()) {
            HILOGE("deviceAddress is null");
            return;
        }
        std::lock_guard<ffrt::recursive_mutex> lock(sThis->mutex_);
        sThis->deviceAddress_ = deviceAddress;
        if (sThis->state_ == ExtensionServiceConnectionState::CREATED ||
            sThis->state_ == ExtensionServiceConnectionState::CONNECTING ||
            sThis->state_ == ExtensionServiceConnectionState::DISCONNECTED) {
            NotifyParam param = { .deviceAddress = deviceAddress};
            sThis->messages_.emplace_back(NotifyType::OnDeviceDiscovered, param);
            HILOGI("Cache OnDeviceDiscovered state_ is %{public}d", sThis->state_);
            if (sThis->state_ == ExtensionServiceConnectionState::CREATED ||
                sThis->state_ == ExtensionServiceConnectionState::DISCONNECTED) {
                HILOGI("Connect ability");
                sThis->state_ = ExtensionServiceConnectionState::CONNECTING;
                AAFwk::Want want;
                want.SetElementName(sThis->subscriberInfo_.bundleName, sThis->subscriberInfo_.extensionName);
                int32_t result = AAFwk::AbilityManagerClient::GetInstance()->ConnectAbility(want,
                    sThis, sThis->subscriberInfo_.userId);
                //ability failed, target ability not extension service   result:2097170    less param 2097152
                HILOGE("ConnectAbility result:%{public}d", result);
            }
            return;
        }
        if (sThis->proxy_ == nullptr) {
            HILOGE("null proxy_");
        } else {
            HILOGE("OnDeviceDiscovered begin call");
            int32_t retResult = 0;
            ErrCode callResult = sThis->proxy_->OnDeviceDiscovered(deviceAddress, retResult);
            HILOGI("Notify OnDeviceDiscovered callResult %{public}d retResult %{public}d", callResult, retResult);
        }
    });
}

void ExtensionServiceConnection::NotifyOnDestroyWithReason(const int32_t reason,
    const std::shared_ptr<ExtensionSubscriberInfo> subscriberInfo)
{
    wptr<ExtensionServiceConnection> wThis = this;
    messageQueue_->submit([wThis, reason, subscriberInfo]() {
        sptr<ExtensionServiceConnection> sThis = wThis.promote();
        if (!sThis || !subscriberInfo) {
            HILOGE("null param");
            return;
        }
        HILOGI("NotifyOnDestroyWithReason %{public}d", reason);
        std::lock_guard<ffrt::recursive_mutex> lock(sThis->mutex_);
        if (sThis->state_ == ExtensionServiceConnectionState::CREATED ||
            sThis->state_ == ExtensionServiceConnectionState::CONNECTING||
            sThis->state_ == ExtensionServiceConnectionState::DISCONNECTED) {
            NotifyParam param = { .reason = reason, .subscriberInfo = subscriberInfo};
            sThis->messages_.emplace_back(NotifyType::OnDestroyWithReason, param);
            HILOGI("Cache NotifyOnDestroyWithReason state_ is %{public}d", sThis->state_);
            if (sThis->state_ == ExtensionServiceConnectionState::CREATED||
                sThis->state_ == ExtensionServiceConnectionState::DISCONNECTED) {
                HILOGI("Connect ability");
                sThis->state_ = ExtensionServiceConnectionState::CONNECTING;
                AAFwk::Want want;
                want.SetElementName(sThis->subscriberInfo_.bundleName, sThis->subscriberInfo_.extensionName);
                int32_t result = AAFwk::AbilityManagerClient::GetInstance()->ConnectAbility(want, sThis,
                    sThis->subscriberInfo_.userId);
                //ability failed, target ability not extension service   result:2097170    less param 2097152
                HILOGE("ConnectAbility result:%{public}d", result);
            }
            return;
        }
        if (sThis->proxy_ == nullptr) {
            HILOGE("null proxy_");
        } else {
            int32_t retResult = 0;
            ErrCode callResult = sThis->proxy_->OnDestroyWithReason(reason, retResult);
            HILOGI("Notify NotifyOnDestroyWithReason callResult %{public}d retResult %{public}d",
                callResult, retResult);
            auto connectServiceSptr = sThis->connectionService_.lock(); 
            if (connectServiceSptr == nullptr) {
                HILOGE("connectionService_ is expired.");
            } else {
                connectServiceSptr->CloseConnection(subscriberInfo);
            }
        }
    });
}

void ExtensionServiceConnection::SetNotificationType(const NotificationType& notificationType)
{
    HILOGI("notificationType %{public}d", static_cast<uint8_t>(notificationType));
    notificationType_ = notificationType;
}

void ExtensionServiceConnection::OnAbilityConnectDone(
    const AppExecFwk::ElementName &element, const sptr<IRemoteObject> &remoteObject, int resultCode)
{
    std::lock_guard<ffrt::recursive_mutex> lock(mutex_);
    state_ = ExtensionServiceConnectionState::CONNECTED;
    int32_t notificationId = 0;
    ExtensionServiceConnectionNotifier::GetInstance().SendNotification(subscriberInfo_,
        notificationType_, deviceAddress_.GetAddress(), notificationId);
    SetNotificationId(notificationId);
    remoteObject_ = remoteObject;
    remoteObject->AddDeathRecipient(deathRecipient_);
    proxy_ = new (std::nothrow) PartnerAgentExtensionProxy(remoteObject);
    if (proxy_ == nullptr) {
        HILOGE("failed to create PartnerAgentExtensionProxy!");
        return;
    }
    for (auto& message : messages_) {
        switch (message.first) {
            case NotifyType::OnDeviceDiscovered:
                NotifyOnDeviceDiscovered(message.second.deviceAddress);
                break;
            case NotifyType::OnDestroyWithReason:
                NotifyOnDestroyWithReason(message.second.reason, message.second.subscriberInfo);
                break;
            default:
                HILOGW("incorrect type");
                break;
        }
    }
    messages_.clear();
}

void ExtensionServiceConnection::OnAbilityDisconnectDone(const AppExecFwk::ElementName &element, int resultCode)
{
    HILOGD("OnAbilityDisconnectDone %{public}s", subscriberInfo_.GetKey().c_str());
    std::lock_guard<ffrt::recursive_mutex> lock(mutex_);
    state_ = ExtensionServiceConnectionState::DISCONNECTED;
    ExtensionServiceConnectionNotifier::GetInstance().CancelNotification(notificationId_.load());
    pid_ = -1;
    HandleDisconnectedState();
}

void ExtensionServiceConnection::SetNotificationId(int32_t notificationId)
{
    HILOGI("SetNotificationId is %{public}d", notificationId);
    notificationId_.store(notificationId);
}

void ExtensionServiceConnection::HandleDisconnectedState()
{
    if (remoteObject_ != nullptr) {
        remoteObject_->RemoveDeathRecipient(deathRecipient_);
        remoteObject_ = nullptr;
    }
    deathRecipient_ = nullptr;
    proxy_ = nullptr;
    if (onDisconnected_) {
        HILOGD("call onDisconnected %{public}s", subscriberInfo_.GetKey().c_str());
        onDisconnected_(subscriberInfo_);
        onDisconnected_ = nullptr;
    }
}

void ExtensionServiceConnection::OnRemoteDied(const wptr<IRemoteObject> &remote)
{
    HILOGD("OnRemoteDied %{public}s", subscriberInfo_.GetKey().c_str());
    wptr<ExtensionServiceConnection> wThis = this;
    sptr<ExtensionServiceConnection> sThis = wThis.promote();
    if (sThis) {
        std::lock_guard<ffrt::recursive_mutex> lock(sThis->mutex_);
        sThis->state_ = ExtensionServiceConnectionState::DISCONNECTED;
        sThis->Close();
    }
}

void ExtensionServiceConnection::AppendMessage(std::string& message, ErrCode callResult, int32_t retResult)
{
    if (callResult != ERR_OK) {
        message += " failed with callResult " + std::to_string(callResult);
    } else if (retResult != ERR_OK) {
        message += " failed with retResult " + std::to_string(retResult);
    } else {
        message += " OK";
    }
}
}
}
