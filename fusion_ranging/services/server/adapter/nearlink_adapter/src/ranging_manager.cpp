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
#define LOG_TAG "RangingManager"
#endif

#include "ranging_manager.h"
#include "connection_manager.h"
#include "advertiser_manager.h"
#include "nearlink_ranging_callback.h"
#include "nearlink_connection_callback.h"
#include "nearlink_sle_ranging.h"
#include "nearlink_host.h"
#include "nearlink_def.h"
#include "fcm_thread_util.h"
#include "fusion_ranging_errorcode.h"
#include "common_utils.h"
#include "log_utils.h"

namespace OHOS {
namespace FusionRanging {
using namespace FusionConnectivity;

namespace {
constexpr int32_t CONVERT_M_TO_CM = 100; /* Conversion from meters to centimeters */
constexpr size_t ADDRESS_LENGTH = 17;
constexpr size_t ADDRESS_COLON_INDEX = 2;
constexpr size_t ADDRESS_SEPARATOR_UNIT = 3;
}  // anonymous namespace

RangingManager::RangingManager()
{
    HILOGI("RangingManager");
    rangingCallback_ = std::make_shared<NearlinkRangingCallback>();
    connectionCallback_ = std::make_shared<NearlinkConnectionCallback>();
    nearlinkRanging_ = Nearlink::NearlinkSleRanging::CreateNearlinkSleRanging(rangingCallback_);
}

RangingManager *RangingManager::GetInstance()
{
    static RangingManager instance_;
    return &instance_;
}

RangingManager::~RangingManager()
{
    rangingDev_.Clear();
}

int RangingManager::Init()
{
    bool expected = false;
    if (!isInitialized_.compare_exchange_strong(expected, true)) {
        HILOGI("RangingManager already initialized");
        return RANGING_NO_ERROR;
    }
    Nearlink::NearlinkHost::GetInstance().RegisterRemoteDeviceObserver(connectionCallback_);
    return RANGING_NO_ERROR;
}

int RangingManager::DeInit()
{
    bool expected = true;
    if (!isInitialized_.compare_exchange_strong(expected, false)) {
        HILOGI("RangingManager not initialized");
        return RANGING_NO_ERROR;
    }
    Nearlink::NearlinkHost::GetInstance().DeregisterRemoteDeviceObserver(connectionCallback_);
    HILOGI("RangingManager DeInit: advertiserManager reset done");
    return RANGING_NO_ERROR;
}

int RangingManager::StartRanging(const std::string &deviceId)
{
    FCM_CHECK_RETURN_RET(isInitialized_.load(), RANGING_ERR_OPERATION_FAILED, "StartRanging not init");
    FCM_CHECK_RETURN_RET(IsValidAddress(deviceId), RANGING_ERR_PARAM_NOT_MEET_SPECIFICATIONS, "Invalid deviceId");
    HILOGI("StartRanging deviceId: %{public}s", GET_ENCRYPT_ADDR(deviceId));
    return StartRangingWithConnection(deviceId);
}

int RangingManager::StopRanging(const std::string &deviceId)
{
    FCM_CHECK_RETURN_RET(isInitialized_.load(), RANGING_ERR_OPERATION_FAILED, "adapter not init");
    FCM_CHECK_RETURN_RET(IsValidAddress(deviceId), RANGING_ERR_PARAM_NOT_MEET_SPECIFICATIONS, "Invalid deviceId");
    std::shared_ptr<RangingDevice> rangDev = GetRangingDevice(deviceId);
    FCM_CHECK_RETURN_RET(rangDev != nullptr, RANGING_ERR_DEVICE_NOT_INITIATED, "not found device: %{public}s",
                         GET_ENCRYPT_ADDR(deviceId));

    rangDev->SetRangingState(RangingAdapterState::IDLE);
    FCM_CHECK_RETURN_RET(nearlinkRanging_ != nullptr, RANGING_ERR_OPERATION_FAILED, "ranging nullptr");
    Nearlink::NearlinkRemoteDevice remoteDevice(deviceId,
                                                static_cast<int>(Nearlink::NlTransportType::NL_TRANSPORT_SLE));
    nearlinkRanging_->StopSleRanging(remoteDevice);

    ConnectionManager::GetInstance()->Disconnect(deviceId);
    rangingDev_.Erase(deviceId);
    NotifyRangingStateChanged(deviceId, SLE_RANGING_STOPPED);
    HILOGI("StopRanging device: %{public}s", GET_ENCRYPT_ADDR(deviceId));
    return RANGING_NO_ERROR;
}

int RangingManager::StartPassiveRanging(int32_t &handle)
{
    int ret = AdvertiserManager::GetInstance()->StartPassiveRanging(handle);
    HILOGI("StartPassiveRanging ret:%{public}d", ret);
    return ret;
}

int RangingManager::StopPassiveRanging(int32_t handle)
{
    int ret = AdvertiserManager::GetInstance()->StopPassiveRanging(handle);
    HILOGI("StopPassiveRanging ret:%{public}d", ret);
    return ret;
}

int RangingManager::PauseRanging(const std::string &deviceId)
{
    FCM_CHECK_RETURN_RET(isInitialized_.load(), RANGING_ERR_OPERATION_FAILED, "adapter not init");
    FCM_CHECK_RETURN_RET(IsValidAddress(deviceId), RANGING_ERR_PARAM_NOT_MEET_SPECIFICATIONS, "Invalid deviceId");
    HILOGI("PauseRanging deviceId: %{public}s", GET_ENCRYPT_ADDR(deviceId));

    std::shared_ptr<RangingDevice> rangDev = GetRangingDevice(deviceId);
    FCM_CHECK_RETURN_RET(rangDev != nullptr, RANGING_ERR_DEVICE_NOT_INITIATED, "not found device: %{public}s",
                         GET_ENCRYPT_ADDR(deviceId));
    if (!rangDev->TryTransitionState(RangingAdapterState::RANGING, RangingAdapterState::CONNECTED)) {
        HILOGW("PauseRanging: not in RANGING state");
        return RANGING_ERR_OPERATION_FAILED;
    }
    FCM_CHECK_RETURN_RET(nearlinkRanging_ != nullptr, RANGING_ERR_OPERATION_FAILED, "nearlinkRanging_ nullptr");
    Nearlink::NearlinkRemoteDevice remoteDevice(deviceId,
                                                static_cast<int>(Nearlink::NlTransportType::NL_TRANSPORT_SLE));
    nearlinkRanging_->StopSleRanging(remoteDevice);
    return RANGING_NO_ERROR;
}

int RangingManager::ResumeRanging(const std::string &deviceId)
{
    FCM_CHECK_RETURN_RET(isInitialized_.load(), RANGING_ERR_OPERATION_FAILED, "adapter not init");
    FCM_CHECK_RETURN_RET(IsValidAddress(deviceId), RANGING_ERR_PARAM_NOT_MEET_SPECIFICATIONS, "Invalid deviceId");

    HILOGI("ResumeRanging deviceId: %{public}s", GET_ENCRYPT_ADDR(deviceId));
    std::shared_ptr<RangingDevice> rangDev = GetRangingDevice(deviceId);
    FCM_CHECK_RETURN_RET(rangDev != nullptr, RANGING_ERR_DEVICE_NOT_INITIATED, "not found device: %{public}s",
                         GET_ENCRYPT_ADDR(deviceId));
    if (!rangDev->TryTransitionState(RangingAdapterState::CONNECTED, RangingAdapterState::RANGING)) {
        HILOGW("ResumeRanging: not in CONNECTED state");
        return RANGING_ERR_OPERATION_FAILED;
    }

    bool isAclConn = ConnectionManager::GetInstance()->IsConnected(deviceId);
    if (!isAclConn) {
        HILOGE("ResumeRanging: device not connected");
        rangDev->SetRangingState(RangingAdapterState::IDLE);
        rangingDev_.Erase(deviceId);
        return RANGING_ERR_OPERATION_FAILED;
    }

    return DoStartSleRanging(deviceId);
}

int RangingManager::SetCallback(const std::shared_ptr<FusionRanging::BaseRangingAdapterCallback> &callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (adapterCallback_ != nullptr) {
        return RANGING_NO_ERROR;
    }
    adapterCallback_ = callback;
    return RANGING_NO_ERROR;
}

int RangingManager::StartRangingWithConnection(const std::string &deviceId)
{
    HILOGI("StartRangingWithConnection deviceId: %{public}s", GET_ENCRYPT_ADDR(deviceId));
    std::shared_ptr<RangingDevice> rangDev = GetRangingDevice(deviceId);
    FCM_CHECK_RETURN_RET(rangDev == nullptr, RANGING_ERR_DEVICE_ALREADY_INITIATED, "already exist device: %{public}s",
                         GET_ENCRYPT_ADDR(deviceId));
    rangDev = std::make_shared<RangingDevice>(deviceId);
    if (ConnectionManager::GetInstance()->IsConnected(deviceId)) {
        rangDev->SetRangingState(RangingAdapterState::CONNECTED);
        HILOGI("Device already connected, directly start sle ranging");
        rangingDev_.EnsureInsert(deviceId, rangDev);
        return StartSleRanging(deviceId);
    }

    rangDev->SetRangingState(RangingAdapterState::CONNECTING);
    int ret = ConnectionManager::GetInstance()->Connect(deviceId);
    FCM_CHECK_RETURN_RET(ret == RANGING_NO_ERROR, RANGING_ERR_OPERATION_FAILED, "connect fail");
    rangingDev_.EnsureInsert(deviceId, rangDev);
    return RANGING_NO_ERROR;
}

int RangingManager::DoStartSleRanging(const std::string &deviceId)
{
    FCM_CHECK_RETURN_RET(nearlinkRanging_ != nullptr, RANGING_ERR_OPERATION_FAILED, "nearlinkRanging_ nullptr");
    Nearlink::RangingConfig config;
    Nearlink::NearlinkRemoteDevice remoteDevice(deviceId,
                                                static_cast<int>(Nearlink::NlTransportType::NL_TRANSPORT_SLE));
    Nearlink::NlErrCode ret = nearlinkRanging_->StartSleRanging(remoteDevice, config);
    if (ret != Nearlink::NlErrCode::NL_NO_ERROR) {
        HILOGE("Failed to start SLE ranging for deviceId: %{public}s, ret: %{public}d", GET_ENCRYPT_ADDR(deviceId),
               static_cast<int>(ret));
        std::shared_ptr<RangingDevice> rangDev = GetRangingDevice(deviceId);
        if (rangDev != nullptr) {
            bool isAclConn = ConnectionManager::GetInstance()->IsConnected(deviceId);
            rangDev->SetRangingState(isAclConn ? RangingAdapterState::CONNECTED : RangingAdapterState::IDLE);
        }
        return RANGING_ERR_OPERATION_FAILED;
    }
    return RANGING_NO_ERROR;
}

int RangingManager::StartSleRanging(const std::string &deviceId)
{
    HILOGI("StartSleRanging enter deviceId: %{public}s", GET_ENCRYPT_ADDR(deviceId));
    std::shared_ptr<RangingDevice> rangDev = GetRangingDevice(deviceId);
    FCM_CHECK_RETURN_RET(rangDev != nullptr, RANGING_ERR_DEVICE_NOT_INITIATED, "Not found deviceId: %{public}s",
                         GET_ENCRYPT_ADDR(deviceId));
    bool isAclConn = ConnectionManager::GetInstance()->IsConnected(deviceId);
    FCM_CHECK_RETURN_RET(isAclConn, RANGING_ERR_OPERATION_FAILED, "not connected deviceId:%{public}s",
                         GET_ENCRYPT_ADDR(deviceId));

    if (!rangDev->TryTransitionState(RangingAdapterState::CONNECTED, RangingAdapterState::RANGING)) {
        HILOGW("StartSleRanging: not in CONNECTED state or already ranging");
        return RANGING_ERR_OPERATION_FAILED;
    }
    return DoStartSleRanging(deviceId);
}

void RangingManager::OnRangingStateChange(const std::string &deviceId, int32_t state)
{
    HILOGI("OnRangingStateChange deviceId: %{public}s, state: %{public}d", GET_ENCRYPT_ADDR(deviceId), state);
    std::shared_ptr<RangingDevice> rangDev = GetRangingDevice(deviceId);
    FCM_CHECK_RETURN(rangDev != nullptr, "Not found deviceId: %{public}s", GET_ENCRYPT_ADDR(deviceId));
    if (state == SLE_RANGING_STARTED) {
        rangDev->TryTransitionState(RangingAdapterState::CONNECTED, RangingAdapterState::RANGING);
    } else if (state == SLE_RANGING_STOPPED) {
        rangDev->TryTransitionState(RangingAdapterState::RANGING, RangingAdapterState::IDLE);
    }
    NotifyRangingStateChanged(deviceId, state);
}

void RangingManager::OnRangingResult(const Nearlink::RangingResult &result)
{
    HILOGD("OnRangingResult deviceId: %{public}s, distance:%{public}f, prob:%{public}f",
           GET_ENCRYPT_ADDR(result.GetAddress()), result.GetDistance(), result.GetProb());
    std::string addr = result.GetAddress();
    std::shared_ptr<RangingDevice> rangDev = GetRangingDevice(addr);
    FCM_CHECK_RETURN(rangDev != nullptr, "Not found deviceId: %{public}s", GET_ENCRYPT_ADDR(addr));
    AdapterRangingData data(result.GetAddress(), static_cast<int32_t>(result.GetDistance() * CONVERT_M_TO_CM),
                            result.GetRssi(), -1); /* not support angle, set -1 invalid */
    data.SetConfidence(static_cast<int32_t>(RangingConfidence::MEDIUM));

    std::shared_ptr<BaseRangingAdapterCallback> callback;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        FCM_CHECK_RETURN(adapterCallback_ != nullptr, "adapterCallback_ nullptr");
        callback = adapterCallback_;
    }
    callback->OnRangingResult(data);
}

void RangingManager::HandleConnectedState(std::shared_ptr<RangingDevice> &rangDev)
{
    if (!rangDev->TryTransitionState(RangingAdapterState::CONNECTING, RangingAdapterState::CONNECTED)) {
        rangDev->TryTransitionState(RangingAdapterState::IDLE, RangingAdapterState::CONNECTED);
    }
}

void RangingManager::HandleDisconnectedState(const std::string &deviceId, std::shared_ptr<RangingDevice> &rangDev)
{
    FCM_CHECK_RETURN(nearlinkRanging_ != nullptr, "nearlinkRanging_ nullptr");
    Nearlink::NearlinkRemoteDevice remoteDevice(deviceId,
                                                static_cast<int>(Nearlink::NlTransportType::NL_TRANSPORT_SLE));
    nearlinkRanging_->StopSleRanging(remoteDevice);
    rangDev->SetRangingState(RangingAdapterState::IDLE);
    NotifyRangingStateChanged(deviceId, SLE_RANGING_STOPPED);
}

void RangingManager::OnConnectionStateChange(const std::string &deviceId, int32_t state)
{
    HILOGI("OnConnectionStateChange deviceId: %{public}s, state: %{public}d", GET_ENCRYPT_ADDR(deviceId), state);
    std::shared_ptr<RangingDevice> rangDev = GetRangingDevice(deviceId);
    FCM_CHECK_RETURN(rangDev != nullptr, "Not found deviceId: %{public}s", GET_ENCRYPT_ADDR(deviceId));
    if (state == static_cast<int32_t>(Nearlink::SleConnState::SLE_CONNECTION_STATE_CONNECTED)) {
        HandleConnectedState(rangDev);
    } else if (state == static_cast<int32_t>(Nearlink::SleConnState::SLE_CONNECTION_STATE_ENCRYPTED)) {
        StartSleRanging(deviceId);
    } else if (state == static_cast<int32_t>(Nearlink::SleConnState::SLE_CONNECTION_STATE_DISCONNECTED)) {
        HandleDisconnectedState(deviceId, rangDev);
    }
}

std::shared_ptr<RangingDevice> RangingManager::GetRangingDevice(const std::string &deviceId)
{
    std::shared_ptr<RangingDevice> rangDev = nullptr;
    auto ret = rangingDev_.Find(deviceId, rangDev);
    if (ret) {
        return rangDev;
    }
    return nullptr;
}

void RangingManager::NotifyRangingStateChanged(const std::string &deviceId, int32_t state, int32_t cause)
{
    int32_t rangingDefaultHandle = -1; /* handle below 0 is invalid */
    RangingState rangingState = (state == SLE_RANGING_STARTED) ? RangingState::STATE_STARTED :
                                                                 RangingState::STATE_STOPPED;
    AdapterRangingStateInfo info(ADAPTER_RANGING, deviceId, rangingDefaultHandle, rangingState);
    if (cause >= 0 && cause <= static_cast<int32_t>(RangingStoppedCause::LINK_DISCONNECT)) {
        info.SetStoppedCause(static_cast<RangingStoppedCause>(cause));
    }
    std::shared_ptr<BaseRangingAdapterCallback> callback;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        FCM_CHECK_RETURN(adapterCallback_ != nullptr, "adapterCallback_ nullptr");
        callback = adapterCallback_;
    }
    callback->OnRangingStateChange(info);
}

void RangingManager::NotifyPassiveRangingStateChanged(const std::string &deviceId, int32_t handle, int32_t state,
                                                      int32_t cause)
{
    RangingState rangingState = (state == SLE_RANGING_STARTED) ? RangingState::STATE_STARTED :
                                                                 RangingState::STATE_STOPPED;
    AdapterRangingStateInfo info(ADAPTER_PASSIVE_RANGING, deviceId, handle, rangingState);
    if (cause >= 0 && cause <= static_cast<int32_t>(RangingStoppedCause::LINK_DISCONNECT)) {
        info.SetStoppedCause(static_cast<RangingStoppedCause>(cause));
    }
    std::shared_ptr<BaseRangingAdapterCallback> callback;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        FCM_CHECK_RETURN(adapterCallback_ != nullptr, "adapterCallback_ nullptr");
        callback = adapterCallback_;
    }
    callback->OnRangingStateChange(info);
}
}  // namespace FusionRanging
}  // namespace OHOS