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
#define LOG_TAG "AdvertiserManager"
#endif

#include "advertiser_manager.h"
#include "ranging_manager.h"
#include "nearlink_sle_advertiser.h"
#include "nearlink_uuid.h"
#include "nearlink_ssap_service.h"
#include "nearlink_ssap_server.h"
#include "nearlink_remote_device.h"
#include "fusion_ranging_errorcode.h"
#include "fcm_thread_util.h"
#include "safe_map.h"
#include "log_utils.h"

namespace OHOS {
namespace FusionRanging {
using namespace Nearlink;
using namespace FusionConnectivity;

namespace {
constexpr int32_t WAIT_ADVERTISE_TIMEOUT_MS = 300;
constexpr int32_t INVALID_ADV_HANDLE = -1; /* default -1 as invalid handle */
}  // anonymous namespace

class SsapServerCallbackImpl : public Nearlink::SsapServerCallback {
public:
    explicit SsapServerCallbackImpl(int32_t advHandle) : advHandle_(advHandle){};
    void OnConnectionStateUpdate(const Nearlink::NearlinkRemoteDevice &device, int state, int reason) override;

private:
    int32_t advHandle_{INVALID_ADV_HANDLE};
};

enum AdvState {
    ADV_STATE_STOPPED,
    ADV_STATE_STARTED,
};

struct AdvInfos {
    AdvInfos(int32_t advHandle, const std::shared_ptr<Nearlink::SleAdvertiseCallback> &callback)
        : advHandle_(advHandle), sleAdvCallback_(callback)
    {
    }

    void SetSsapServer(const std::shared_ptr<Nearlink::SsapServer> ssapServer)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ssapServer_ = ssapServer;
    }

    std::shared_ptr<Nearlink::SsapServer> GetSsapServer() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return ssapServer_;
    }

    void SetSsapServerCallback(const std::shared_ptr<SsapServerCallbackImpl> serverCallback)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        serverCallback_ = serverCallback;
    }

    void SetSsapService(const std::shared_ptr<Nearlink::SsapService> ssapService)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ssapService_ = ssapService;
    }

    std::shared_ptr<Nearlink::SsapService> GetSsapService() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return ssapService_;
    }

    int32_t advHandle_{INVALID_ADV_HANDLE};
    std::shared_ptr<Nearlink::SleAdvertiseCallback> sleAdvCallback_;

    mutable std::mutex mutex_{};
    std::shared_ptr<SsapServerCallbackImpl> serverCallback_;
    std::shared_ptr<Nearlink::SsapServer> ssapServer_;
    std::shared_ptr<Nearlink::SsapService> ssapService_;
};

struct AdvertiserManager::impl {
    impl();
    ~impl();

    std::shared_ptr<Nearlink::SsapService> GetRangingSsapService();
    std::shared_ptr<AdvInfos> GetAdvInfo(int32_t advHandle);

    std::shared_ptr<Nearlink::SleAdvertiser> advertiser_{nullptr};
    SafeMap<int32_t, std::shared_ptr<AdvInfos>> advInfos_{};

    std::atomic<int32_t> advHandle_{INVALID_ADV_HANDLE};
    std::mutex advertisingLock_;
    std::condition_variable startAdvertising_;
};

class AdvertiserCallback : public SleAdvertiseCallback {
public:
    AdvertiserCallback() = default;
    ~AdvertiserCallback() override = default;

    void OnStartResultEvent(int result, int advHandle) override
    {
        if (result != 0) {
            return;
        }
        AdvertiserManager::GetInstance()->OnAdvStateChanged(advHandle, ADV_STATE_STARTED);
    }

    void OnEnableResultEvent(int result, int advHandle) override {}
    void OnDisableResultEvent(int result, int advHandle) override {}
    void OnStopResultEvent(int result, int advHandle) override
    {
        if (result != 0) {
            return;
        }
        AdvertiserManager::GetInstance()->OnAdvStateChanged(advHandle, ADV_STATE_STOPPED);
    }

    void OnGetAdvHandleEvent(int result, int advHandle) override {}
    void OnSetAdvDataEvent(int result) override {}
};

static NlErrCode DoStartAndGetHandle(const std::shared_ptr<Nearlink::SleAdvertiser> &advertiser,
    const std::shared_ptr<AdvertiserCallback> &callback, uint8_t &sleAdvHandle)
{
    SleAdvertiserSettings settings;
    settings.SetConnectable(true);
    settings.SetPrimaryPhy(static_cast<uint8_t>(SlePhyType::PHY_LE_CODED));
    SleAdvertiserData advData;
    std::string serviceData = "FusionRanging";
    UUID uuid = UUID::FromString(RANGING_SERVICE_UUID);
    advData.AddServiceData(uuid, serviceData);
    SleAdvertiserData scanResponse;
    scanResponse.SetIncludeDeviceName(true);
    scanResponse.AddManufacturerData(0x027d, RANGING_SERVICE_UUID);
    NlErrCode ret = advertiser->StartAdvertising(settings, advData, scanResponse, 0, callback);
    HILOGI("DoStartAndGetHandle ret:%{public}d", static_cast<int>(ret));
    if (ret != NlErrCode::NL_NO_ERROR) {
        return ret;
    }
    ret = advertiser->GetAdvHandle(callback, sleAdvHandle);
    if (ret != NlErrCode::NL_NO_ERROR) {
        HILOGE("GetAdvHandle fail, stop advertising, ret:%{public}d", static_cast<int>(ret));
        advertiser->StopAdvertising(callback);
        return ret;
    }
    HILOGI("GetAdvHandle ret:%{public}d, advHandle:%{public}d", static_cast<int>(ret), sleAdvHandle);
    return NlErrCode::NL_NO_ERROR;
}

void SsapServerCallbackImpl::OnConnectionStateUpdate(const Nearlink::NearlinkRemoteDevice &device, int state,
                                                     int reason)
{
    HILOGI("OnConnectionStateUpdate state:%{public}d, reason:%{public}d", state, reason);
    auto deviceId = device.GetDeviceAddr();
    auto handle = advHandle_;
    DoInRangingThread([deviceId, handle, state, reason]() {
        AdvertiserManager::GetInstance()->OnSsapConnectionStateUpdate(deviceId, handle, state, reason);
    });
}

AdvertiserManager *AdvertiserManager::GetInstance()
{
    static AdvertiserManager instance;
    return &instance;
}

AdvertiserManager::AdvertiserManager()
{
    pimpl = std::make_unique<impl>();
}

AdvertiserManager::~AdvertiserManager()
{
    pimpl->advInfos_.Clear();
    HILOGI("~AdvertiserManager: destroyed");
}

AdvertiserManager::impl::impl()
{
    advertiser_ = SleAdvertiser::CreateSleAdvertiser();
}

AdvertiserManager::impl::~impl() {}

std::shared_ptr<Nearlink::SsapService> AdvertiserManager::impl::GetRangingSsapService()
{
    Nearlink::UUID uuid = Nearlink::UUID::FromString(RANGING_SERVICE_UUID);
    Nearlink::UUID uuidProperty = Nearlink::UUID::FromString(RANGING_PROPERTY_UUID);
    std::shared_ptr<Nearlink::SsapService> svs =
        std::make_shared<Nearlink::SsapService>(uuid, Nearlink::SsapServiceType::VENDOR_PROMARY);
    std::shared_ptr<Nearlink::SsapProperty> np = std::make_shared<Nearlink::SsapProperty>(
        Nearlink::SsapProperty::PropertyType::ENTRY_TYPE_PROPERTY, uuidProperty,
        Nearlink::SsapProperty::OperationIndication::OPERATION_READ |
            Nearlink::SsapProperty::OperationIndication::OPERATION_WRITE_NO_RESPONSE |
            Nearlink::SsapProperty::OperationIndication::OPERATION_WRITE_WITH_RESPONSE |
            Nearlink::SsapProperty::OperationIndication::OPERATION_NOTIFY |
            Nearlink::SsapProperty::OperationIndication::OPERATION_INDICATION |
            Nearlink::SsapProperty::OperationIndication::OPERATION_BROADCAST |
            Nearlink::SsapProperty::OperationIndication::OPERATION_WRITE_CLIENT_CONFIG |
            Nearlink::SsapProperty::OperationIndication::OPERATION_WRITE_SERVER_CONFIG,
        0);
    std::shared_ptr<Nearlink::SsapDescriptor> nd = std::make_shared<Nearlink::SsapDescriptor>(
        Nearlink::SsapDescriptor::PropertyDescriptorType::DESCRIPTOR_TYPE_CLIENT_PROPERTY_CONFIG, 0);
    np->AddDescriptor(*nd);
    svs->AddProperty(*np);
    return svs;
}

std::shared_ptr<AdvInfos> AdvertiserManager::impl::GetAdvInfo(int32_t advHandle)
{
    std::shared_ptr<AdvInfos> advInfo = nullptr;
    auto ret = advInfos_.Find(advHandle, advInfo);
    if (ret && advInfo != nullptr) {
        return advInfo;
    }
    return nullptr;
}

int AdvertiserManager::StartPassiveRanging(int32_t &advHandle)
{
    FCM_CHECK_RETURN_RET(!IsPassiveRangingActive(), RANGING_ERR_OPERATION_FAILED, "already active");
    auto ret = StartAdvertisingInternal(advHandle);
    FCM_CHECK_RETURN_RET(ret == RANGING_NO_ERROR, RANGING_ERR_OPERATION_FAILED, "adv internal err");
    ret = StartSsapServerInternal(advHandle);
    std::unique_lock<std::mutex> advertisingLock(pimpl->advertisingLock_);
    auto isNotify =
        pimpl->startAdvertising_.wait_for(advertisingLock, std::chrono::milliseconds(WAIT_ADVERTISE_TIMEOUT_MS),
                                          [this]() { return (pimpl->advHandle_.load() != INVALID_ADV_HANDLE); });
    HILOGI("StartPassiveRanging isNotify:%{public}d", isNotify);
    if (!isNotify) {
        StopPassiveRanging(advHandle);
        return RANGING_ERR_OPERATION_FAILED;
    }
    return ret;
}

int AdvertiserManager::StopPassiveRanging(int32_t advHandle)
{
    auto retAdv = StopAdvertising(advHandle);
    if (retAdv != RANGING_NO_ERROR) {
        HILOGW("adv auto stopped after connect ret:%{public}d", static_cast<int>(retAdv));
    }
    auto retSsap = StopSsapServer(advHandle);
    HILOGI("StopSsapServer advHandle:%{public}d, ret:%{public}d", advHandle, static_cast<int>(retSsap));
    pimpl->advInfos_.Erase(advHandle);
    return (retSsap == RANGING_NO_ERROR) ? RANGING_NO_ERROR : RANGING_ERR_OPERATION_FAILED;
}

void AdvertiserManager::OnAdvStateChanged(int advHandle, int state)
{
    HILOGI("OnAdvStateChanged advHandle:%{public}d, state:%{public}d", advHandle, state);
    if (state == ADV_STATE_STARTED) {
        pimpl->advHandle_ = advHandle;
        pimpl->startAdvertising_.notify_one();
    }
}

int AdvertiserManager::StopSsapServer(int32_t advHandle)
{
    HILOGI("StopSsapServer advHandle:%{public}d", advHandle);
    auto advInfo = pimpl->GetAdvInfo(advHandle);
    FCM_CHECK_RETURN_RET(advInfo != nullptr, RANGING_ERR_OPERATION_FAILED, "not found adv");
    auto ssapServer = advInfo->GetSsapServer();
    FCM_CHECK_RETURN_RET(ssapServer != nullptr, RANGING_ERR_OPERATION_FAILED, "server nullptr");
    auto service = advInfo->GetSsapService();
    FCM_CHECK_RETURN_RET(service != nullptr, RANGING_ERR_OPERATION_FAILED, "service nullptr");
    ssapServer->RemoveSsapService(*service);
    return RANGING_NO_ERROR;
}

bool AdvertiserManager::IsPassiveRangingActive()
{
    return pimpl->advInfos_.Size() > 0;
}

int AdvertiserManager::StartAdvertisingInternal(int32_t &advHandle)
{
    FCM_CHECK_RETURN_RET(pimpl->advertiser_ != nullptr, RANGING_ERR_OPERATION_FAILED, "null advertiser");
    auto callback = std::make_shared<AdvertiserCallback>();
    pimpl->advHandle_ = INVALID_ADV_HANDLE;
    uint8_t sleAdvHandle = 0xFF; /* 0xFF is invalid adv handle */
    NlErrCode ret = DoStartAndGetHandle(pimpl->advertiser_, callback, sleAdvHandle);
    if (ret != NlErrCode::NL_NO_ERROR) {
        return RANGING_ERR_OPERATION_FAILED;
    }
    advHandle = sleAdvHandle;
    auto advInfo = std::make_shared<AdvInfos>(sleAdvHandle, callback);
    pimpl->advInfos_.EnsureInsert(sleAdvHandle, advInfo);
    return RANGING_NO_ERROR;
}

int AdvertiserManager::StopAdvertising(int32_t advHandle)
{
    FCM_CHECK_RETURN_RET(pimpl->advertiser_ != nullptr, RANGING_ERR_OPERATION_FAILED, "null advertiser");
    auto advInfo = pimpl->GetAdvInfo(advHandle);
    FCM_CHECK_RETURN_RET(advInfo != nullptr, RANGING_ERR_OPERATION_FAILED, "not found adv");
    NlErrCode ret = pimpl->advertiser_->StopAdvertising(advInfo->sleAdvCallback_);
    HILOGI("StopAdvertising advHandle:%{public}d, ret:%{public}d", advHandle, static_cast<int>(ret));
    return (ret == NlErrCode::NL_NO_ERROR) ? RANGING_NO_ERROR : RANGING_ERR_OPERATION_FAILED;
}

void AdvertiserManager::OnSsapConnectionStateUpdate(const std::string &deviceId, int32_t advHandle, int state,
                                                    int reason)
{
    HILOGI("deviceId:%{public}s, state:%{public}d, reason:%{public}d, advHandle:%{public}d", GET_ENCRYPT_ADDR(deviceId),
           state, reason, advHandle);
    auto advInfo = pimpl->GetAdvInfo(advHandle);
    FCM_CHECK_RETURN(advInfo != nullptr, "null advInfo");
    if (state == static_cast<int>(SleConnectState::CONNECTED)) {
        RangingManager::GetInstance()->NotifyPassiveRangingStateChanged(deviceId, advHandle, SLE_RANGING_STARTED);
    } else if (state == static_cast<int>(SleConnectState::DISCONNECTED)) {
        RangingManager::GetInstance()->NotifyPassiveRangingStateChanged(deviceId, advHandle, SLE_RANGING_STOPPED);
    }
}

int AdvertiserManager::StartSsapServerInternal(int32_t advHandle)
{
    HILOGI("StartSsapServerInternal advHandle:%{public}d", advHandle);
    auto advInfo = pimpl->GetAdvInfo(advHandle);
    FCM_CHECK_RETURN_RET(advInfo != nullptr, RANGING_ERR_OPERATION_FAILED, "null advInfo");
    std::shared_ptr<SsapServerCallbackImpl> callback = std::make_shared<SsapServerCallbackImpl>(advHandle);
    auto ssapServer = SsapServer::CreateSsapServer(callback);
    FCM_CHECK_RETURN_RET(ssapServer != nullptr, RANGING_ERR_OPERATION_FAILED, "CreateSsapServer err");
    auto service = pimpl->GetRangingSsapService();
    NlErrCode ret = ssapServer->AddService(*service);
    FCM_CHECK_RETURN_RET((ret == NlErrCode::NL_NO_ERROR), RANGING_ERR_OPERATION_FAILED, "AddService err");
    advInfo->SetSsapServer(ssapServer);
    advInfo->SetSsapServerCallback(callback);
    advInfo->SetSsapService(service);
    return RANGING_NO_ERROR;
}
}  // namespace FusionRanging
}  // namespace OHOS