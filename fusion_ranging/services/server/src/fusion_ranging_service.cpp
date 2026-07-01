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
#define LOG_TAG "FusionRangingService"
#endif

#include "fusion_ranging_service.h"
#include <functional>
#include <dlfcn.h>
#include <climits>
#include <mutex>
#include <chrono>
#include "ranging_adapter_factory.h"
#include "fusion_ranging_errorcode.h"
#include "fcm_thread_util.h"
#include "common_utils.h"
#include "log_utils.h"

namespace {
#ifdef SUPPORT_NEARLINK_RANGING
const std::string REFERENCE_LIB_PATH = std::string(NEARLINK_RANGING_ADAPTER_PATH);
const std::string FILESEPARATOR = "/";
const std::string REFERENCE_LIB_NAME = "libnearlink_ranging_adapter_ext.z.so";
const std::string RANGING_ADAPTER_EXT_LIB = REFERENCE_LIB_PATH + FILESEPARATOR + REFERENCE_LIB_NAME;
void *g_rangingAdapterHandle = nullptr;
std::mutex g_rangingAdapterMutex;
std::once_flag g_rangingAdapterFlag;
#endif
}

namespace OHOS {
namespace FusionRanging {
constexpr int32_t START_PASSIVE_RANGING_PROMISE_TIMEOUT_MS = 500;
constexpr int32_t INVALID_PASSIVE_RANGING_HANDLE = -1;

class FusionRangingAdapterCallback : public BaseRangingAdapterCallback {
public:
    explicit FusionRangingAdapterCallback() {}

    void OnRangingStateChange(const AdapterRangingStateInfo &info) override
    {
        FusionRangingService::GetInstance()->OnAdapterRangingStateChanged(info);
    }

    void OnRangingResult(const AdapterRangingData &data) override
    {
        RangingResult result;
        result.SetDeviceId(data.GetDeviceId());
        RangingMeasurement dist(data.GetDistance(), RangingConfidence::MEDIUM);
        result.SetDistance(dist);
        RangingMeasurement ang(data.GetAngle(), RangingConfidence::LOW);
        result.SetAngle(ang);
        result.SetRssi(data.GetRssi());
        FusionRangingService::GetInstance()->OnRangingResult(result);
    }
};

FusionRangingService::FusionRangingService()
{
    InitConfiguration();
}

FusionRangingService::~FusionRangingService()
{
    adapters_.Iterate([&](const RangingTypes &type, const std::shared_ptr<BaseRangingAdapter> &adapter) {
        if (adapter != nullptr) {
            adapter->DeInit();
        }
    });
    adapters_.Clear();
    devicesInfo_.Clear();
    HILOGI("FusionRangingService destroyed, adapters and data cleared");
    DeInitConfiguration();
}

FusionRangingService *FusionRangingService::GetInstance()
{
    static FusionRangingService instance;
    return &instance;
}

bool FusionRangingService::IsRangingSupported(RangingTypes capabilityType)
{
    return RangingAdapterFactory::Instance().IsRangingAdapterSupported(capabilityType);
}

int FusionRangingService::StartRanging(const RangingParams &params, const sptr<IRangingObserver> &observer,
                                       int32_t callerUid)
{
    HILOGI("StartRanging device:%{public}s, type:%{public}d", GET_ENCRYPT_ADDR(params.GetDeviceId()),
           params.GetCapabilityType());
    if (!IsValidAddress(params.GetDeviceId())) {
        return RANGING_ERR_PARAM_NOT_MEET_SPECIFICATIONS;
    }

    if (!IsRangingSupported(params.GetCapabilityType())) {
        return RANGING_ERR_RANGING_SERVICE_DISABLED;
    }

    if (GetRangingDevice(params.GetDeviceId()) != nullptr) {
        return RANGING_ERR_DEVICE_ALREADY_INITIATED;
    }
    FusionConnectivity::DoInRangingThread(
        [this, params, observer, callerUid]() { HandleStartRanging(params, observer, callerUid); }, 0);
    return RANGING_NO_ERROR;
}

void FusionRangingService::HandleStartRanging(const RangingParams &params, const sptr<IRangingObserver> &observer,
                                              int32_t callerUid)
{
    auto ret = ProcessStartRanging(params, observer, callerUid);
    if (ret == RANGING_NO_ERROR) {
        return;
    }
    FCM_CHECK_RETURN(observer != nullptr, "observer nullptr device:%{public}s", GET_ENCRYPT_ADDR(params.GetDeviceId()));
    RangingStateChangeInfo stateInfo(params.GetDeviceId(), INVALID_PASSIVE_RANGING_HANDLE, RangingState::STATE_STOPPED,
                                     RangingStoppedCause::INTERNAL_ERROR);
    observer->OnRangingStateChanged(stateInfo);
}

int FusionRangingService::ProcessStartRanging(const RangingParams &params, const sptr<IRangingObserver> &observer,
                                              int32_t callerUid)
{
    auto deviceInfo = GetRangingDevice(params.GetDeviceId());
    FCM_CHECK_RETURN_RET(deviceInfo == nullptr, RANGING_NO_ERROR, "already exist device:%{public}s",
                         GET_ENCRYPT_ADDR(params.GetDeviceId()));

    const bool isSupport = RangingAdapterFactory::Instance().IsRangingAdapterSupported(params.GetCapabilityType());
    FCM_CHECK_RETURN_RET(isSupport, RANGING_ERR_OPERATION_FAILED, "Faild capability type:%{public}d",
                         params.GetCapabilityType());
    auto ret = CreateRangingAdapter(params.GetCapabilityType());
    FCM_CHECK_RETURN_RET(ret == RANGING_NO_ERROR, RANGING_ERR_OPERATION_FAILED, "Fail create ret:%{public}d", ret);

    auto rangingCallback = std::make_shared<FusionRangingAdapterCallback>();
    auto adapter = GetAdapter(params.GetCapabilityType());
    FCM_CHECK_RETURN_RET(adapter != nullptr, RANGING_ERR_OPERATION_FAILED, "Get adapter fail");

    adapter->SetCallback(rangingCallback);
    ret = adapter->StartRanging(params.GetDeviceId());
    HILOGI("ProcessStartRanging ret:%{public}d", ret);
    if (ret != 0) {
        HILOGE("Adapter start ranging failed for device: %{public}s, ret: %{public}d",
               GET_ENCRYPT_ADDR(params.GetDeviceId()), ret);
        adapter->StopRanging(params.GetDeviceId());
        return RANGING_ERR_OPERATION_FAILED;
    }
    auto info = std::make_shared<RangingDeviceInfo>(callerUid, params, observer, RangingState::STATE_STARTED);
    devicesInfo_.EnsureInsert(params.GetDeviceId(), info);
    HILOGI("ProcessStartRanging device:%{public}s, capabilityType:%{public}d", GET_ENCRYPT_ADDR(params.GetDeviceId()),
           static_cast<int>(params.GetCapabilityType()));
    return RANGING_NO_ERROR;
}

int FusionRangingService::StopRanging(const std::string &deviceId, int32_t callerUid)
{
    FCM_CHECK_RETURN_RET(!deviceId.empty(), RANGING_ERR_INVALID_PARAM, "invalid device:%{public}s",
                         GET_ENCRYPT_ADDR(deviceId));
    auto info = GetRangingDevice(deviceId);
    FCM_CHECK_RETURN_RET(info != nullptr, RANGING_ERR_DEVICE_NOT_INITIATED, "not found device:%{public}s",
                         GET_ENCRYPT_ADDR(deviceId));
    FCM_CHECK_RETURN_RET(info->callerUid_ == callerUid, RANGING_ERR_OPERATION_FAILED, "Operation not allow");
    auto adapter = GetAdapter(info->params_.GetCapabilityType());
    if (adapter != nullptr) {
        adapter->StopRanging(deviceId);
    }
    devicesInfo_.Erase(deviceId);
    HILOGI("Stop ranging for device: %{public}s", GET_ENCRYPT_ADDR(deviceId));
    return RANGING_NO_ERROR;
}

int FusionRangingService::StartPassiveRanging(RangingTypes capabilityType, int32_t &handle,
                                              const sptr<IRangingObserver> &observer, int32_t callerUid)
{
    auto promise = std::make_shared<std::promise<int>>();
    auto self = this;
    FusionConnectivity::DoInRangingThread([self, capabilityType, &handle, observer, callerUid, promise]() {
        int result = self->HandleStartPassiveRanging(capabilityType, handle, observer, callerUid);
        promise->set_value(result);
    });
    auto future = promise->get_future();
    auto status = future.wait_for(std::chrono::milliseconds(START_PASSIVE_RANGING_PROMISE_TIMEOUT_MS));
    if (status == std::future_status::timeout) {
        HILOGE("StartPassiveRanging timeout");
        return RANGING_ERR_OPERATION_FAILED;
    }
    return future.get();
}

int FusionRangingService::HandleStartPassiveRanging(RangingTypes capabilityType, int32_t &advHandle,
                                                    const sptr<IRangingObserver> &observer, int32_t callerUid)
{
    const bool isSupport = RangingAdapterFactory::Instance().IsRangingAdapterSupported(capabilityType);
    HILOGI("StartPassiveRanging type:%{public}d, isSupport:%{public}d", capabilityType, isSupport);
    FCM_CHECK_RETURN_RET(isSupport, RANGING_ERR_RANGING_TYPE_NOT_SUPPORT, "Faild capability type:%{public}d",
                         capabilityType);
    auto ret = CreateRangingAdapter(capabilityType);
    FCM_CHECK_RETURN_RET(ret == RANGING_NO_ERROR, RANGING_ERR_OPERATION_FAILED, "crate capability type:%{public}d",
                         capabilityType);
    auto adapter = GetAdapter(capabilityType);
    FCM_CHECK_RETURN_RET(adapter != nullptr, RANGING_ERR_OPERATION_FAILED, "Get adapter fail");
    auto rangingCallback = std::make_shared<FusionRangingAdapterCallback>();
    adapter->SetCallback(rangingCallback);
    ret = adapter->StartPassiveRanging(advHandle);
    if (ret == RANGING_NO_ERROR) {
        HILOGI("HandleStartPassiveRanging add handle:%{public}d, uid:%{public}d", advHandle, callerUid);
        if (GetPassiveRangingHandle(advHandle) == nullptr) {
            auto handleInfo = std::make_shared<PassiveRangingHandle>(callerUid, capabilityType, advHandle, observer);
            advHandles_.EnsureInsert(advHandle, handleInfo);
        }
        return ret;
    } else {
        return RANGING_ERR_OPERATION_FAILED;
    }
}

int FusionRangingService::StopPassiveRanging(RangingTypes capabilityType, int32_t handle, int32_t callerUid)
{
    auto handleInfo = GetPassiveRangingHandle(handle);
    HILOGI("StopPassiveRanging capabiity:%{public}d, handle:%{public}d", capabilityType, handle);
    FCM_CHECK_RETURN_RET(handleInfo != nullptr, RANGING_ERR_OPERATION_FAILED, "not found handle");
    FCM_CHECK_RETURN_RET(handleInfo->callerUid_ == callerUid, RANGING_ERR_OPERATION_FAILED, "Operation not allow");
    auto adapter = GetAdapter(capabilityType);
    FCM_CHECK_RETURN_RET(adapter != nullptr, RANGING_ERR_OPERATION_FAILED, "Get adapter fail");
    auto ret = adapter->StopPassiveRanging(handle);
    advHandles_.Erase(handle);
    HILOGI("StopPassiveRanging handle:%{public}d, ret=%{public}d", handle, ret);
    if (ret != RANGING_NO_ERROR) {
        return RANGING_ERR_OPERATION_FAILED;
    }
    return ret;
}

int FusionRangingService::PauseRanging(const std::string &deviceId)
{
    auto info = GetRangingDevice(deviceId);
    FCM_CHECK_RETURN_RET(info != nullptr, RANGING_ERR_OPERATION_FAILED, "not found device:%{public}s",
                         GET_ENCRYPT_ADDR(deviceId));
    auto adapter = GetAdapter(info->params_.GetCapabilityType());
    FCM_CHECK_RETURN_RET(adapter != nullptr, RANGING_ERR_OPERATION_FAILED, "Get adapter fail");
    auto ret = adapter->PauseRanging(deviceId);
    FCM_CHECK_RETURN_RET(ret != RANGING_NO_ERROR, RANGING_ERR_OPERATION_FAILED, "ret:%{public}d", ret);
    HILOGI("Pause ranging for device: %{public}s", GET_ENCRYPT_ADDR(deviceId));
    return RANGING_NO_ERROR;
}

int FusionRangingService::ResumeRanging(const std::string &deviceId)
{
    auto info = GetRangingDevice(deviceId);
    FCM_CHECK_RETURN_RET(info != nullptr, RANGING_ERR_OPERATION_FAILED, "not found device:%{public}s",
                         GET_ENCRYPT_ADDR(deviceId));

    auto adapter = GetAdapter(info->params_.GetCapabilityType());
    FCM_CHECK_RETURN_RET(adapter != nullptr, RANGING_ERR_OPERATION_FAILED, "Get adapter fail");
    auto ret = adapter->ResumeRanging(deviceId);
    HILOGI("Resume ranging for device: %{public}s, ret:%{public}d", GET_ENCRYPT_ADDR(deviceId), ret);
    FCM_CHECK_RETURN_RET(ret != RANGING_NO_ERROR, RANGING_ERR_OPERATION_FAILED, "ret:%{public}d", ret);
    return RANGING_NO_ERROR;
}

int FusionRangingService::CreateRangingAdapter(RangingTypes capabilityType)
{
    auto existingAdapter = GetAdapter(capabilityType);
    FCM_CHECK_RETURN_RET(existingAdapter == nullptr, RANGING_NO_ERROR, "adapter already exist type:%{public}d",
                         capabilityType);

    auto newAdapter = RangingAdapterFactory::Instance().CreateRangingAdapter(capabilityType);
    FCM_CHECK_RETURN_RET(newAdapter != nullptr, RANGING_ERR_OPERATION_FAILED, "create adapter fail");
    int ret = newAdapter->Init();
    if (ret != RANGING_NO_ERROR) {
        HILOGE("Adapter init failed, releasing adapter");
        newAdapter->DeInit();
        return RANGING_ERR_OPERATION_FAILED;
    }
    adapters_.EnsureInsert(capabilityType, newAdapter);
    HILOGI("Create adapter for capability type: %{public}d", static_cast<int>(capabilityType));
    return RANGING_NO_ERROR;
}

void FusionRangingService::InitConfiguration()
{
#ifdef SUPPORT_NEARLINK_RANGING
    std::call_once(g_rangingAdapterFlag, [&]() {
        std::lock_guard<std::mutex> lock(g_rangingAdapterMutex);
        if (g_rangingAdapterHandle == nullptr) {
            char canonicalPath[PATH_MAX] = {0};
            if (realpath(RANGING_ADAPTER_EXT_LIB.c_str(), canonicalPath) == nullptr) {
                HILOGE("Failed to normalize path: %{public}s", RANGING_ADAPTER_EXT_LIB.c_str());
                return;
            }
            if (strncmp(canonicalPath, REFERENCE_LIB_PATH.c_str(), REFERENCE_LIB_PATH.length()) != 0) {
                HILOGE("Invalid lib path after normalization: %{public}s", canonicalPath);
                return;
            }
            g_rangingAdapterHandle = dlopen(canonicalPath, RTLD_NOW);
            if (g_rangingAdapterHandle == nullptr) {
                HILOGE("Failed to load ranging adapter ext lib: %{public}s", dlerror());
                return;
            }
            HILOGI("Ranging adapter ext lib loaded successfully");
        }
    });
#endif
}

void FusionRangingService::DeInitConfiguration()
{
#ifdef SUPPORT_NEARLINK_RANGING
    std::lock_guard<std::mutex> lock(g_rangingAdapterMutex);
    if (g_rangingAdapterHandle != nullptr) {
        dlclose(g_rangingAdapterHandle);
        g_rangingAdapterHandle = nullptr;
        HILOGI("Ranging adapter ext lib unloaded successfully");
    }
#endif
}

std::shared_ptr<BaseRangingAdapter> FusionRangingService::GetAdapter(RangingTypes capabilityType)
{
    std::shared_ptr<BaseRangingAdapter> adapter = nullptr;
    auto ret = adapters_.Find(capabilityType, adapter);
    if (ret) {
        return adapter;
    }
    return nullptr;
}

void FusionRangingService::OnAdapterRangingStateChanged(const AdapterRangingStateInfo &info)
{
    HILOGI("OnAdapterRangingStateChanged:%{public}d", static_cast<int32_t>(info.GetState()));
    switch (info.GetType()) {
        case ADAPTER_RANGING: {
            auto deviceInfo = GetRangingDevice(info.GetDeviceId());
            if (deviceInfo != nullptr && deviceInfo->observer_ != nullptr) {
                RangingStateChangeInfo stateInfo(info.GetDeviceId(), info.GetHandle(), info.GetState(),
                                                 info.GetStoppedCause());
                deviceInfo->observer_->OnRangingStateChanged(stateInfo);
            }
            break;
        }
        case ADAPTER_PASSIVE_RANGING: {
            auto handleInfo = GetPassiveRangingHandle(info.GetHandle());
            if (handleInfo && handleInfo->observer_) {
                RangingStateChangeInfo stateInfo(info.GetDeviceId(), info.GetHandle(), info.GetState(),
                                                 info.GetStoppedCause());
                handleInfo->observer_->OnRangingStateChanged(stateInfo);
            }
            break;
        }
        default:
            break;
    }
}

void FusionRangingService::OnRangingResult(const RangingResult &result)
{
    auto deviceInfo = GetRangingDevice(result.GetDeviceId());
    if (deviceInfo != nullptr && deviceInfo->observer_ != nullptr) {
        deviceInfo->observer_->OnRangingResult(result);
    }
}

std::shared_ptr<RangingDeviceInfo> FusionRangingService::GetRangingDevice(const std::string &deviceId)
{
    std::shared_ptr<RangingDeviceInfo> info = nullptr;
    auto ret = devicesInfo_.Find(deviceId, info);
    if (ret && info != nullptr) {
        return info;
    }
    return nullptr;
}

std::shared_ptr<PassiveRangingHandle> FusionRangingService::GetPassiveRangingHandle(int32_t handle)
{
    std::shared_ptr<PassiveRangingHandle> handleInfo = nullptr;
    auto ret = advHandles_.Find(handle, handleInfo);
    if (ret && handleInfo != nullptr) {
        return handleInfo;
    }
    return nullptr;
}

void FusionRangingService::HandleProcessDeath(int32_t uid)
{
    HILOGI("HandleProcessDeath uid:%{public}d", uid);
    std::vector<std::shared_ptr<PassiveRangingHandle>> handleInfos;
    advHandles_.Iterate([&](int32_t handle, const std::shared_ptr<PassiveRangingHandle> info) {
        if (info->callerUid_ == uid) {
            handleInfos.push_back(info);
        }
    });
    for (auto it = handleInfos.begin(); it != handleInfos.end(); it++) {
        StopPassiveRanging((*it)->cap_, (*it)->advHandle_, uid);
    }

    std::vector<std::shared_ptr<RangingDeviceInfo>> devicesInfo = GetRangingDevicesInfoByUid(uid);
    for (auto it = devicesInfo.begin(); it != devicesInfo.end(); it++) {
        StopRanging((*it)->params_.GetDeviceId(), uid);
    }
}

void FusionRangingService::PauseRangingByUid(int32_t uid)
{
    HILOGI("PauseRangingByUid uid:%{public}d, size:%{public}d", uid, devicesInfo_.Size());
    std::vector<std::shared_ptr<RangingDeviceInfo>> devicesInfo = GetRangingDevicesInfoByUid(uid);
    for (auto it = devicesInfo.begin(); it != devicesInfo.end(); it++) {
        PauseRanging((*it)->params_.GetDeviceId());
    }
}

void FusionRangingService::ResumeRangingByUid(int32_t uid)
{
    HILOGI("ResumeRangingByUid uid:%{public}d, size:%{public}d", uid, devicesInfo_.Size());
    std::vector<std::shared_ptr<RangingDeviceInfo>> devicesInfo = GetRangingDevicesInfoByUid(uid);
    for (auto it = devicesInfo.begin(); it != devicesInfo.end(); it++) {
        ResumeRanging((*it)->params_.GetDeviceId());
    }
}

bool FusionRangingService::IsRangingEmpty()
{
    return devicesInfo_.IsEmpty() && advHandles_.IsEmpty();
}

std::vector<std::shared_ptr<RangingDeviceInfo>> FusionRangingService::GetRangingDevicesInfoByUid(int32_t uid)
{
    std::vector<std::shared_ptr<RangingDeviceInfo>> devicesInfo;
    devicesInfo_.Iterate([&](const std::string &deviceId, const std::shared_ptr<RangingDeviceInfo> &info) {
        if (info->callerUid_ == uid) {
            devicesInfo.push_back(info);
        }
    });
    return devicesInfo;
}
}  // namespace FusionRanging
}  // namespace OHOS