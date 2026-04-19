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
#include <mutex>
#include "ranging_adapter_factory.h"
#include "log_util.h"
#include "fusion_ranging_errorcode.h"

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

class FusionRangingAdapterCallback : public BaseRangingAdapterCallback {
public:
    explicit FusionRangingAdapterCallback(const std::shared_ptr<FusionRangingService> adapter)
        : interRangingAdapter_(adapter)
    {
    }

    void OnRangingStateChange(int32_t state) override
    {
        auto adapter = interRangingAdapter_.lock();
        if (adapter) {
            adapter->OnAdapterRangingStateChanged(state);
        }
    }

    void OnRangingResult(const AdapterRangingData &data) override
    {
        auto adapter = interRangingAdapter_.lock();
        HILOGI("FusionRangingService OnRangingResult deviceId:%{public}s", GET_ENCRYPT_ADDR(data.GetDeviceId()));
        if (adapter) {
            RangingResult result;
            result.SetDeviceId(data.GetDeviceId());
            RangingMeasurement dist(data.IsVaild(), data.GetDistance(), RangingConfidence::MEDIUM);
            result.SetDistance(dist);
            RangingMeasurement ang(data.IsVaild(), data.GetAngle(), RangingConfidence::LOW);
            result.SetAngle(ang);
            result.SetRssi(data.GetRssi());
            adapter->OnRangingResult(result);
        }
    }

private:
    std::weak_ptr<FusionRangingService> interRangingAdapter_;
};

FusionRangingService::FusionRangingService()
{
    InitConfiguration();
}

FusionRangingService::~FusionRangingService()
{
    DeInitConfiguration();

    std::lock_guard<std::mutex> lock(rangingMutex_);
    for (auto &[type, adapter] : rangingAdapters_) {
        if (adapter) {
            adapter->DeInit();
        }
    }
    rangingAdapters_.clear();
    rangingData_.clear();
    HILOGI("FusionRangingService destroyed, adapters and data cleared");
}

std::shared_ptr<FusionRangingService> FusionRangingService::GetInstance()
{
    static std::shared_ptr<FusionRangingService> instance(new FusionRangingService());
    return instance;
}

int FusionRangingService::StartRanging(const RangingParams &params,
                                       const std::function<void(const RangingResult &)> &callback)
{
    HILOGI("StartRanging service");
    if (params.GetDeviceId().empty()) {
        HILOGE("Invalid device id");
        return static_cast<int>(RangingErrCode::RANGING_ERR_INVALID_PARAM);
    }

    std::lock_guard<std::mutex> lock(rangingMutex_);
    auto it = rangingData_.find(params.GetDeviceId());
    if (it != rangingData_.end()) {
        HILOGW("Device %{public}s already in ranging state", GET_ENCRYPT_ADDR(params.GetDeviceId()));
        return static_cast<int>(RangingErrCode::RANGING_ERR_ALREADY_RANGING);
    }

    const bool isSupport = RangingAdapterFactory::Instance().IsRangingAdapterSupported(params.GetCapabilityType());
    HILOGI("StartRanging type:%{public}d, isSupport:%{public}d", params.GetCapabilityType(), isSupport);
    if (!isSupport) {
        return static_cast<int>(RangingErrCode::RANGING_ERR_ADAPTER_NOT_FOUND);
    }
    auto ret = CreateRangingAdapter(params);
    HILOGI("StartRanging service CreateRangingAdapter ret:%{public}d", ret);
    if (ret != 0) {
        return ret;
    }

    auto rangingCallback = std::make_shared<FusionRangingAdapterCallback>(shared_from_this());
    auto adapter = GetAdapter(params.GetCapabilityType());
    adapter->SetCallback(rangingCallback);

    ret = adapter->StartRanging(params.GetDeviceId(), params.GetRole());
    HILOGI("StartRanging service StartRanging ret:%{public}d", ret);
    if (ret != 0) {
        HILOGE("Adapter start ranging failed for device: %{public}s, ret: %{public}d",
               GET_ENCRYPT_ADDR(params.GetDeviceId()), ret);
        return ret;
    }

    RangingDeviceInfo deviceInfo;
    deviceInfo.params = params;
    deviceInfo.callback = callback;
    deviceInfo.state = RangingState::STATE_STARTED;
    rangingData_[params.GetDeviceId()] = deviceInfo;
    HILOGI("Start ranging for device: %{public}s, role: %{public}d, capabilityType: %{public}d",
           GET_ENCRYPT_ADDR(params.GetDeviceId()), static_cast<int>(params.GetRole()),
           static_cast<int>(params.GetCapabilityType()));
    return 0;
}

int FusionRangingService::StopRanging(const std::string &deviceId)
{
    if (deviceId.empty()) {
        HILOGE("Invalid device id");
        return static_cast<int>(RangingErrCode::RANGING_ERR_INVALID_PARAM);
    }

    std::lock_guard<std::mutex> lock(rangingMutex_);
    auto it = rangingData_.find(deviceId);
    if (it == rangingData_.end()) {
        HILOGE("Device %{public}s not found", GET_ENCRYPT_ADDR(deviceId));
        return static_cast<int>(RangingErrCode::RANGING_ERR_DEVICE_NOT_FOUND);
    }

    RangingParams params = it->second.params;
    auto adapterIt = rangingAdapters_.find(params.GetCapabilityType());
    if (adapterIt != rangingAdapters_.end()) {
        adapterIt->second->StopRanging(deviceId);
    }

    rangingData_.erase(it);
    HILOGI("Stop ranging for device: %{public}s", GET_ENCRYPT_ADDR(deviceId));
    return 0;
}

int FusionRangingService::PauseRanging(const std::string &deviceId)
{
    if (deviceId.empty()) {
        HILOGE("PauseRanging: invalid device id");
        return static_cast<int>(RangingErrCode::RANGING_ERR_INVALID_PARAM);
    }

    std::lock_guard<std::mutex> lock(rangingMutex_);
    auto it = rangingData_.find(deviceId);
    if (it == rangingData_.end()) {
        HILOGE("PauseRanging: device %{public}s not found", GET_ENCRYPT_ADDR(deviceId));
        return static_cast<int>(RangingErrCode::RANGING_ERR_DEVICE_NOT_FOUND);
    }

    RangingParams params = it->second.params;
    auto adapterIt = rangingAdapters_.find(params.GetCapabilityType());
    if (adapterIt != rangingAdapters_.end()) {
        adapterIt->second->PauseRanging(deviceId);
    }

    HILOGI("Pause ranging for device: %{public}s", GET_ENCRYPT_ADDR(deviceId));
    return 0;
}

int FusionRangingService::ResumeRanging(const std::string &deviceId)
{
    if (deviceId.empty()) {
        HILOGE("ResumeRanging: invalid device id");
        return static_cast<int>(RangingErrCode::RANGING_ERR_INVALID_PARAM);
    }

    std::lock_guard<std::mutex> lock(rangingMutex_);
    auto it = rangingData_.find(deviceId);
    if (it == rangingData_.end()) {
        HILOGE("ResumeRanging: device %{public}s not found", GET_ENCRYPT_ADDR(deviceId));
        return static_cast<int>(RangingErrCode::RANGING_ERR_DEVICE_NOT_FOUND);
    }

    RangingParams params = it->second.params;
    auto adapterIt = rangingAdapters_.find(params.GetCapabilityType());
    if (adapterIt != rangingAdapters_.end()) {
        adapterIt->second->ResumeRanging(deviceId);
    }

    HILOGI("Resume ranging for device: %{public}s", GET_ENCRYPT_ADDR(deviceId));
    return 0;
}

int FusionRangingService::GetRangingData(const std::string &deviceId, int32_t &distance, int32_t &rssi)
{
    if (deviceId.empty()) {
        HILOGE("Invalid device id");
        return static_cast<int>(RangingErrCode::RANGING_ERR_INVALID_PARAM);
    }

    std::lock_guard<std::mutex> lock(rangingMutex_);
    auto it = rangingData_.find(deviceId);
    if (it == rangingData_.end()) {
        HILOGE("Device %{public}s not found in ranging data", GET_ENCRYPT_ADDR(deviceId));
        return static_cast<int>(RangingErrCode::RANGING_ERR_DEVICE_NOT_FOUND);
    }

    distance = 0;
    rssi = -50;
    return 0;
}

int FusionRangingService::CreateRangingAdapter(const RangingParams &params)
{
    RangingTypes capabilityType = params.GetCapabilityType();
    auto it = rangingAdapters_.find(capabilityType);
    if (it != rangingAdapters_.end()) {
        return 0;
    }

    auto adapter = RangingAdapterFactory::Instance().CreateRangingAdapter(capabilityType);
    if (adapter == nullptr) {
        HILOGE("Unsupported capability type: %{public}d", static_cast<int>(capabilityType));
        return static_cast<int>(RangingErrCode::RANGING_ERR_ADAPTER_NOT_FOUND);
    }

    int ret = adapter->Init();
    if (ret != 0) {
        HILOGE("Adapter init failed, releasing adapter");
        adapter->DeInit();
        return ret;
    }

    rangingAdapters_[capabilityType] = adapter;
    HILOGI("Create adapter for capability type: %{public}d", static_cast<int>(capabilityType));
    return 0;
}

void FusionRangingService::InitConfiguration()
{
#ifdef SUPPORT_NEARLINK_RANGING
    std::call_once(g_rangingAdapterFlag, [&]() {
        std::lock_guard<std::mutex> lock(g_rangingAdapterMutex);
        if (g_rangingAdapterHandle == nullptr) {
            g_rangingAdapterHandle = dlopen(RANGING_ADAPTER_EXT_LIB.c_str(), RTLD_NOW);
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
    auto it = rangingAdapters_.find(capabilityType);
    if (it != rangingAdapters_.end()) {
        return it->second;
    }
    return nullptr;
}

void FusionRangingService::OnAdapterRangingStateChanged(int32_t state)
{
    HILOGI("OnAdapterRangingStateChanged:%{public}d", state);
}

void FusionRangingService::OnRangingResult(const RangingResult &result)
{
    HILOGI("OnRangingResult deviceId: %{public}s", GET_ENCRYPT_ADDR(result.GetDeviceId()));
    std::lock_guard<std::mutex> lock(rangingMutex_);
    auto it = rangingData_.find(result.GetDeviceId());
    if (it != rangingData_.end() && it->second.callback) {
        HILOGI("OnRangingResult callback deviceId: %{public}s", GET_ENCRYPT_ADDR(result.GetDeviceId()));
        it->second.callback(result);
    }
}
}  // namespace FusionRanging
}  // namespace OHOS