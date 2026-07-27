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
#define LOG_TAG "ConnectionManager"
#endif

#include "connection_manager.h"
#include "nearlink_connection_callback.h"
#include "nearlink_host.h"
#include "nearlink_remote_device.h"
#include "nearlink_ssap_client.h"
#include "nearlink_def.h"
#include "ranging_manager.h"
#include "fcm_thread_util.h"
#include "fusion_ranging_errorcode.h"
#include "safe_map.h"
#include "log_utils.h"

namespace OHOS {
namespace FusionRanging {
using namespace OHOS::Nearlink;
using namespace OHOS::FusionConnectivity;

namespace {
constexpr int32_t CONNECTION_TIMEOUT_TASK_DELAY_MS = 10000;
constexpr int32_t CONN_MGR_POINTER_NULL = -1;
static constexpr const char *RANGING_SERVICE_UUID = "00090000-0001-0005-0000-000000000000";
static constexpr const char *RANGING_PROPERTY_UUID = "00090000-0001-0005-0001-000000000000";
}  // anonymous namespace

class SsapClientCallbackImpl : public Nearlink::SsapClientCallback {
public:
    explicit SsapClientCallbackImpl(const std::string &deviceId) : deviceId_(deviceId){};
    ~SsapClientCallbackImpl() = default;

    void OnConnectionStateChanged(int connectionState, int ret) override;
    void OnServicesDiscoveredByUuid(int status, const Nearlink::UUID &uuid) override;

private:
    std::string deviceId_ = "";
};

struct SsapClientInfo {
    SsapClientInfo(const std::shared_ptr<Nearlink::SsapClient> &client,
                   const std::shared_ptr<SsapClientCallbackImpl> &callback)
        : client(client), callback(callback)
    {
    }

    void SetConnected(bool connect)
    {
        std::lock_guard<std::mutex> lock(mutex);
        isConnected = connect;
    }

    bool IsConnected() const
    {
        std::lock_guard<std::mutex> lock(mutex);
        return isConnected;
    }

    void SetPaired(bool paired)
    {
        std::lock_guard<std::mutex> lock(mutex);
        isPaired = paired;
    }

    bool IsPaired() const
    {
        std::lock_guard<std::mutex> lock(mutex);
        return isPaired;
    }

    void SetSelfInitiated(bool initiated)
    {
        std::lock_guard<std::mutex> lock(mutex);
        isSelfInitiated = initiated;
    }

    bool IsSelfInitiated() const
    {
        std::lock_guard<std::mutex> lock(mutex);
        return isSelfInitiated;
    }

    std::shared_ptr<Nearlink::SsapClient> client{nullptr};
    std::shared_ptr<SsapClientCallbackImpl> callback{nullptr};

    mutable std::mutex mutex{};
    bool isConnected{false};
    bool isPaired{false};
    bool isSelfInitiated{false};
};

struct ConnectionManager::impl {
    impl();
    ~impl();

    std::shared_ptr<SsapClientInfo> GetClientInfo(const std::string &deviceId);
    bool IsRemoteSupportRangingService(std::shared_ptr<Nearlink::SsapClient> client);
    SafeMap<std::string, std::shared_ptr<SsapClientInfo>> deviceClients_{};
};

void SsapClientCallbackImpl::OnConnectionStateChanged(int connectionState, int ret)
{
    HILOGI("OnConnectionStateChanged state:%{public}d, ret:%{public}d", connectionState, ret);
    auto deviceId = deviceId_;
    FusionConnectivity::DoInRangingThread([deviceId, connectionState, ret]() {
        ConnectionManager::GetInstance()->OnSsapConnectionStateChanged(deviceId, connectionState, ret);
    });
}

void SsapClientCallbackImpl::OnServicesDiscoveredByUuid(int status, const Nearlink::UUID &uuid)
{
    HILOGI("OnServicesDiscoveredByUuid status:%{public}d", status);
    auto deviceId = deviceId_;
    FusionConnectivity::DoInRangingThread([deviceId, status, uuid]() {
        ConnectionManager::GetInstance()->OnSsapServiceDiscovery(deviceId, status, uuid);
    });
}

ConnectionManager *ConnectionManager::GetInstance()
{
    static ConnectionManager instance_;
    return &instance_;
}

ConnectionManager::ConnectionManager()
{
    pimpl = std::make_unique<impl>();
}
ConnectionManager::~ConnectionManager()
{
    pimpl->deviceClients_.Clear();
}

ConnectionManager::impl::impl() {}

ConnectionManager::impl::~impl() {}

std::shared_ptr<SsapClientInfo> ConnectionManager::impl::GetClientInfo(const std::string &deviceId)
{
    std::shared_ptr<SsapClientInfo> clientInfo = nullptr;
    auto ret = deviceClients_.Find(deviceId, clientInfo);
    if (ret) {
        return clientInfo;
    }
    return nullptr;
}

int ConnectionManager::Connect(const std::string &deviceId)
{
    HILOGI("Connect deviceId: %{public}s", GET_ENCRYPT_ADDR(deviceId));
    NearlinkRemoteDevice remoteDevice(deviceId, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
    bool isAcbConnected = false;
    NlErrCode ret = remoteDevice.IsAcbConnected(isAcbConnected);
    HILOGI("Connect deviceId:%{public}s, ret:%{public}d, acl connected:%{public}d", GET_ENCRYPT_ADDR(deviceId), ret,
           isAcbConnected);
    if (ret == NL_NO_ERROR && isAcbConnected) {
        return RANGING_NO_ERROR;
    }
    return ConnectWithSsap(deviceId);
}

int ConnectionManager::ConnectWithSsap(const std::string &deviceId)
{
    HILOGI("ConnectWithSsap deviceId:%{public}s", GET_ENCRYPT_ADDR(deviceId));
    auto device = std::make_shared<NearlinkRemoteDevice>(deviceId, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
    auto ssapClient = Nearlink::SsapClient::CreateSsapClient(device);
    FCM_CHECK_RETURN_RET(ssapClient != nullptr, CONN_MGR_POINTER_NULL, "CreateSsapClient fail");
    auto callback = std::make_shared<SsapClientCallbackImpl>(deviceId);
    auto clientInfo = std::make_shared<SsapClientInfo>(ssapClient, callback);
    clientInfo->SetSelfInitiated(true);

    NlErrCode connectRet = ssapClient->Connect(callback);
    if (connectRet != NL_NO_ERROR) {
        HILOGE("SsapClient Connect failed, ret:%{public}d", static_cast<int>(connectRet));
        return static_cast<int>(connectRet);
    }
    StartConnectTimeout(deviceId);
    pimpl->deviceClients_.EnsureInsert(deviceId, clientInfo);
    HILOGI("SsapClient Connect started size:%{public}d", pimpl->deviceClients_.Size());
    return RANGING_NO_ERROR;
}

bool ConnectionManager::impl::IsRemoteSupportRangingService(std::shared_ptr<Nearlink::SsapClient> client)
{
    FCM_CHECK_RETURN_RET(client != nullptr, false, "client nullptr");
    auto service = client->GetService(UUID::FromString(RANGING_SERVICE_UUID));
    FCM_CHECK_RETURN_RET(service != nullptr, false, "GetService fail");
    Nearlink::UUID propertyUuid = Nearlink::UUID::FromString(RANGING_PROPERTY_UUID);
    std::vector<Nearlink::SsapProperty> properties = service->GetProperty();
    for (size_t i = 0; i < properties.size(); i++) {
        if (properties.at(i).GetUuid().Equals(propertyUuid)) {
            return true;
        }
    }
    return false;
}

void ConnectionManager::OnSsapConnectionStateChanged(const std::string &deviceId, int connectionState, int ret)
{
    HILOGI("deviceId:%{public}s, state:%{public}d, ret:%{public}d", GET_ENCRYPT_ADDR(deviceId), connectionState, ret);
    if (connectionState == static_cast<int>(SleConnectState::CONNECTED)) {
        std::shared_ptr<SsapClientInfo> clientInfo = nullptr;
        auto found = pimpl->deviceClients_.Find(deviceId, clientInfo);
        FCM_CHECK_RETURN((found && clientInfo != nullptr), "not found device client");
        clientInfo->SetConnected(true);

        FCM_CHECK_RETURN(clientInfo->client, "client nullptr");
        clientInfo->client->FindStructureByUuid(UUID::FromString(RANGING_SERVICE_UUID));
    } else if (connectionState == static_cast<int>(SleConnectState::DISCONNECTED)) {
        StopConnectTimeout(deviceId);
        pimpl->deviceClients_.Erase(deviceId);
    }
}

void ConnectionManager::OnSsapServiceDiscovery(const std::string &deviceId, int status, const Nearlink::UUID &uuid)
{
    HILOGI("deviceId:%{public}s, status:%{public}d, uuid:%{public}s", GET_ENCRYPT_ADDR(deviceId), status,
           uuid.GetEncryptUuid().c_str());
    if (uuid.Equals(Nearlink::UUID::FromString(RANGING_SERVICE_UUID))) {
        std::shared_ptr<SsapClientInfo> clientInfo = nullptr;
        auto ret = pimpl->deviceClients_.Find(deviceId, clientInfo);
        FCM_CHECK_RETURN((ret && clientInfo != nullptr), "not found device client");
        FCM_CHECK_RETURN(clientInfo->client != nullptr, "not found ssap client");

        auto findServiceRet = pimpl->IsRemoteSupportRangingService(clientInfo->client);
        HILOGI("find ranging service ret:%{public}d", findServiceRet);
        if (!findServiceRet) {
            clientInfo->client->Disconnect();
            HILOGI("Remote not support ranging disconnect");
            return;
        }
        StopConnectTimeout(deviceId);
        if (clientInfo->IsPaired()) {
            RangingManager::GetInstance()->OnConnectionStateChange(
                deviceId, static_cast<int>(SleConnState::SLE_CONNECTION_STATE_ENCRYPTED));
        } else {
            StartPair(deviceId);
        }
    }
}

int ConnectionManager::Disconnect(const std::string &deviceId)
{
    auto clientInfo = pimpl->GetClientInfo(deviceId);
    FCM_CHECK_RETURN_RET(clientInfo != nullptr, RANGING_ERR_DEVICE_NOT_INITIATED, "not found device");
    FCM_CHECK_RETURN_RET(clientInfo->IsSelfInitiated(), RANGING_NO_ERROR, "not connect by FusionRanging");
    FCM_CHECK_RETURN_RET(clientInfo->client, CONN_MGR_POINTER_NULL, "client nullptr");
    auto ret = clientInfo->client->Disconnect();
    HILOGI("Disconnect deviceId:%{public}s, ret:%{public}d", GET_ENCRYPT_ADDR(deviceId), static_cast<int>(ret));
    return static_cast<int>(ret);
}

int ConnectionManager::StartPair(const std::string &deviceId)
{
    int pairState = static_cast<int>(SlePairState::SLE_PAIR_NONE);
    NearlinkRemoteDevice remoteDevice(deviceId, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
    NlErrCode ret = remoteDevice.GetPairState(pairState);
    HILOGI("IsAcbEncrypted ret:%{public}d, pairState:%{public}d", static_cast<int>(ret), pairState);
    if (ret != NlErrCode::NL_NO_ERROR) {
        return static_cast<int>(ret);
    }
    if (pairState != static_cast<int>(SlePairState::SLE_PAIR_PAIRED)) {
        HILOGI("ACB not encrypted, start Pair");
        ret = remoteDevice.StartPair();
    }
    HILOGI("StartPair deviceId:%{public}s, ret:%{public}d", GET_ENCRYPT_ADDR(deviceId), static_cast<int>(ret));
    return static_cast<int>(ret);
}

bool ConnectionManager::IsConnected(const std::string &deviceId)
{
    bool isConnected = false;
    auto clientInfo = pimpl->GetClientInfo(deviceId);
    isConnected = (clientInfo != nullptr && clientInfo->IsConnected());
    if (!isConnected) {
        NearlinkRemoteDevice remoteDevice(deviceId, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
        remoteDevice.IsAcbConnected(isConnected);
    }
    return isConnected;
}

void ConnectionManager::OnPairStateChanged(const std::string &deviceId, int32_t state)
{
    HILOGI("OnPairStateChanged state:%{public}d", state);
    auto devicesInfo = pimpl->GetClientInfo(deviceId);
    FCM_CHECK_RETURN(devicesInfo != nullptr, "not found device:%{public}s", GET_ENCRYPT_ADDR(deviceId));
    devicesInfo->SetPaired(state == static_cast<int32_t>(SlePairState::SLE_PAIR_PAIRED));
}

void ConnectionManager::OnAcbStateChanged(const std::string &deviceId, int state)
{
    HILOGI("OnAcbStateChanged state:%{public}d, device:%{public}s", state, GET_ENCRYPT_ADDR(deviceId));
    if (state == static_cast<int>(SleConnState::SLE_CONNECTION_STATE_ENCRYPTED)) {
        auto clientInfo = pimpl->GetClientInfo(deviceId);
        FCM_CHECK_RETURN(clientInfo != nullptr, "client info nullptr");
        clientInfo->SetConnected(true);
        clientInfo->SetPaired(state == static_cast<int>(SleConnState::SLE_CONNECTION_STATE_ENCRYPTED));
        FCM_CHECK_RETURN(clientInfo->client != nullptr, "client nullptr");
        auto findServiceRet = pimpl->IsRemoteSupportRangingService(clientInfo->client);
        if (findServiceRet) {
            RangingManager::GetInstance()->OnConnectionStateChange(deviceId, state);
        }
    } else {
        RangingManager::GetInstance()->OnConnectionStateChange(deviceId, state);
    }
}

void ConnectionManager::StartConnectTimeout(const std::string &deviceId)
{
    std::string taskName = "CONNECTTIMEOUT_" + GetEncryptAddr(deviceId);
    FusionConnectivity::FcmThreadUtil::GetInstance().PostTask(
        FusionConnectivity::THREAD_ID_RANGING,
        [this, deviceId]() {
            HILOGI("ConnectionTimeout");
            auto clientInfo = pimpl->GetClientInfo(deviceId);
            FCM_CHECK_RETURN(clientInfo != nullptr, "clientInfo nullptr");
            if (!clientInfo->IsConnected()) {
                ConnectionManager::GetInstance()->OnAcbStateChanged(
                    deviceId, static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));
            }
            FCM_CHECK_RETURN(clientInfo->client, "client nullptr");
            clientInfo->client->Disconnect();
        },
        CONNECTION_TIMEOUT_TASK_DELAY_MS, taskName);
}

void ConnectionManager::StopConnectTimeout(const std::string &deviceId)
{
    std::string taskName = "CONNECTTIMEOUT_" + GetEncryptAddr(deviceId);
    FusionConnectivity::FcmThreadUtil::GetInstance().RemoveTask(FusionConnectivity::THREAD_ID_RANGING, taskName);
}
}  // namespace FusionRanging
}  // namespace OHOS