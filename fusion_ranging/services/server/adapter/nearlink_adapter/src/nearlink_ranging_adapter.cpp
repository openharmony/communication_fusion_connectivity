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
#define LOG_TAG "NearlinkRangingAdapter"
#endif

#include "nearlink_ranging_adapter.h"
#include "ranging_manager.h"
#include "ranging_adapter_factory.h"
#include "log_utils.h"

namespace OHOS {
namespace FusionRanging {

namespace {
constexpr uint8_t NEARLINK_INVALID_RANGING_CAPABILITY = 0xFF;
std::atomic<uint8_t> g_capability = NEARLINK_INVALID_RANGING_CAPABILITY;

bool IsNearlinkRangingAdapterSupported()
{
    uint8_t capability = g_capability.load();
    if ((capability != NEARLINK_INVALID_RANGING_CAPABILITY) &&
        (capability != static_cast<uint8_t>(Nearlink::HadmSupportCapability::NOT_SUPPORT))) {
        return true;
    }
    capability = static_cast<uint8_t>(Nearlink::HadmSupportCapability::NOT_SUPPORT);
    auto ret = Nearlink::NearlinkSleRanging::GetRangingSupportedCapability(capability);
    HILOGI("IsNearlinkRangingAdapterSupported ret:%{public}d, capability:%{public}d", static_cast<int32_t>(ret),
           capability);
    g_capability = capability;
    return (ret == Nearlink::NlErrCode::NL_NO_ERROR) && (capability != NEARLINK_INVALID_RANGING_CAPABILITY) &&
           (capability != static_cast<uint8_t>(Nearlink::HadmSupportCapability::NOT_SUPPORT));
}

AutoRegisterRangingAdapter<NearlinkRangingAdapter> g_registerNearlinkRangingAdapter(
    RangingTypes::NEARLINK_HADM, []() { return std::make_shared<NearlinkRangingAdapter>(); },
    &IsNearlinkRangingAdapterSupported);
}  // anonymous namespace

NearlinkRangingAdapter::NearlinkRangingAdapter() {}

NearlinkRangingAdapter::~NearlinkRangingAdapter()
{
    DeInit();
}

int NearlinkRangingAdapter::Init()
{
    HILOGI("NearlinkRangingAdapter Init");
    return RangingManager::GetInstance()->Init();
}

int NearlinkRangingAdapter::DeInit()
{
    HILOGI("NearlinkRangingAdapter DeInit");
    return RangingManager::GetInstance()->DeInit();
}

int NearlinkRangingAdapter::StartRanging(const std::string &deviceId)
{
    return RangingManager::GetInstance()->StartRanging(deviceId);
}

int NearlinkRangingAdapter::StopRanging(const std::string &deviceId)
{
    return RangingManager::GetInstance()->StopRanging(deviceId);
}

int NearlinkRangingAdapter::StartPassiveRanging(int32_t &handle)
{
    return RangingManager::GetInstance()->StartPassiveRanging(handle);
}

int NearlinkRangingAdapter::StopPassiveRanging(int32_t handle)
{
    return RangingManager::GetInstance()->StopPassiveRanging(handle);
}

int NearlinkRangingAdapter::PauseRanging(const std::string &deviceId)
{
    return RangingManager::GetInstance()->PauseRanging(deviceId);
}

int NearlinkRangingAdapter::ResumeRanging(const std::string &deviceId)
{
    return RangingManager::GetInstance()->ResumeRanging(deviceId);
}

int NearlinkRangingAdapter::SetCallback(const std::shared_ptr<BaseRangingAdapterCallback> &callback)
{
    return RangingManager::GetInstance()->SetCallback(callback);
}
}  // namespace FusionRanging
}  // namespace OHOS