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
#define LOG_TAG "ExtensionServiceConnectionService"
#endif

#include "ability_manager_client.h"
#include "extension_service_connection_service.h"

namespace OHOS {
namespace FusionConnectivity {

std::mutex ExtensionServiceConnectionService::instanceMutex_;
std::shared_ptr<ExtensionServiceConnectionService> ExtensionServiceConnectionService::instance_ =
    std::make_shared<ExtensionServiceConnectionService>();

std::shared_ptr<ExtensionServiceConnectionService> ExtensionServiceConnectionService::GetInstance()
{
    std::lock_guard<std::mutex> lock(instanceMutex_);
    return ExtensionServiceConnectionService::instance_;
}

void ExtensionServiceConnectionService::NotifyOnDeviceDiscovered(
    const std::shared_ptr<ExtensionSubscriberInfo> subscriberInfo,
    const PartnerDeviceAddress& deviceAddress, const NotificationType& type)
{
    auto connection = GetConnection(subscriberInfo);
    if (connection == nullptr) {
        HILOGE("null connection");
        return;
    }
    connection->SetNotificationType(type);
    connection->NotifyOnDeviceDiscovered(deviceAddress);
}

void ExtensionServiceConnectionService::NotifyOnDestroyWithReason(
    const std::shared_ptr<ExtensionSubscriberInfo> subscriberInfo, const int32_t reason)
{
    auto connection = GetConnection(subscriberInfo);
    if (connection == nullptr) {
        HILOGE("null connection");
        return;
    }
    connection->connectionService_ = weak_from_this();
    connection->NotifyOnDestroyWithReason(reason, subscriberInfo);
}

void ExtensionServiceConnectionService::CloseConnection(const std::shared_ptr<ExtensionSubscriberInfo> subscriberInfo)
{
    std::string connectionKey = subscriberInfo->GetKey();
    HILOGI("close connection: %{public}s", connectionKey.c_str());
    do {
        std::lock_guard<ffrt::recursive_mutex> lock(mapLock_);
        auto iter = connectionMap_.find(connectionKey);
        if (iter == connectionMap_.end()) {
            HILOGE("connection not found");
            break;
        }
        if (iter->second == nullptr) {
            HILOGE("null connection");
            connectionMap_.erase(iter);
            break;
        }
        iter->second->Close();
    } while (false);
}

void ExtensionServiceConnectionService::RemoveConnection(const ExtensionSubscriberInfo& subscriberInfo)
{
    std::string connectionKey = subscriberInfo.GetKey();
    HILOGD("remove connection: %{public}s", connectionKey.c_str());
    {
        std::lock_guard<ffrt::recursive_mutex> lock(mapLock_);
        auto iter = connectionMap_.find(connectionKey);
        if (iter != connectionMap_.end()) {
            connectionMap_.erase(iter);
        }
    }
}

int32_t ExtensionServiceConnectionService::Connect(const std::shared_ptr<ExtensionSubscriberInfo> subscriberInfo)
{
    if (subscriberInfo == nullptr) {
        HILOGE("null subscriberInfo");
        return 0;
    }
    std::lock_guard<ffrt::recursive_mutex> lock(mapLock_);
    std::string connectionKey = subscriberInfo->GetKey();
    sptr<ExtensionServiceConnection> connection = nullptr;
    auto iter = connectionMap_.find(connectionKey);
    if (iter == connectionMap_.end()) {
        HILOGI("create connection: %{public}s", connectionKey.c_str());
        connection = new (std::nothrow) ExtensionServiceConnection(
            *subscriberInfo, [this](const ExtensionSubscriberInfo& info) { RemoveConnection(info); });
        if (connection == nullptr) {
            HILOGE("new connection failed: %{public}s", connectionKey.c_str());
        } else {
            connectionMap_[connectionKey] = connection;
        }
    } else {
        HILOGI("found connection: %{public}s", connectionKey.c_str());
        connection = iter->second;
    }
    AAFwk::Want want;
    want.SetElementName(subscriberInfo->bundleName, subscriberInfo->extensionName);
    int32_t result = AAFwk::AbilityManagerClient::GetInstance()->ConnectAbility(want,
        connection, subscriberInfo->userId);
    //ability failed, target ability not extension service   result:2097170    less param 2097152
    HILOGI("ConnectAbility result:%{public}d", result);
    return result;
}

sptr<ExtensionServiceConnection> ExtensionServiceConnectionService::GetConnection(
    const std::shared_ptr<ExtensionSubscriberInfo> subscriberInfo)
{
    if (subscriberInfo == nullptr) {
        HILOGE("null subscriberInfo");
        return nullptr;
    }
    std::lock_guard<ffrt::recursive_mutex> lock(mapLock_);
    std::string connectionKey = subscriberInfo->GetKey();
    sptr<ExtensionServiceConnection> connection = nullptr;
    auto iter = connectionMap_.find(connectionKey);
    if (iter == connectionMap_.end()) {
        HILOGI("create connection: %{public}s", connectionKey.c_str());
        connection = new (std::nothrow) ExtensionServiceConnection(
            *subscriberInfo, [this](const ExtensionSubscriberInfo& info) {});
        if (connection == nullptr) {
            HILOGE("new connection failed: %{public}s", connectionKey.c_str());
        } else {
            connectionMap_[connectionKey] = connection;
        }
    } else {
        HILOGI("found connection: %{public}s", connectionKey.c_str());
        connection = iter->second;
    }
    return connection;
}
}
}
