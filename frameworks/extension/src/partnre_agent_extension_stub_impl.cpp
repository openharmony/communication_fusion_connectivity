/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#define LOG_TAG "PartnerAgentExtensionStubImpl"
#endif

#include "log.h"
#include "partner_agent_extension_stub_impl.h"
#include "napi_ha_event_utils.h"
#include "fusion_connectivity_errorcode.h"

namespace OHOS {
namespace FusionConnectivity {

constexpr int32_t TIME_MS_PER_SECOND = 1000;
constexpr int32_t TIME_NS_PER_MS = 1000000;

static int64_t GetNowTimeMs()
{
#ifndef CROSS_PLATFORM
    struct timespec ts = {};
    clock_gettime(CLOCK_BOOTTIME, &ts);
    return (int64_t)ts.tv_sec * TIME_MS_PER_SECOND + (int64_t)ts.tv_nsec / TIME_NS_PER_MS;
#else
    return -1;
#endif
}

int32_t PartnerAgentExtensionStubImpl::CallbackEnter(uint32_t code)
{
    HILOGI("PartnerAgentExtensionStubImpl CallbackEnter ipc code: %{public}u", code);
    return ERR_OK;
}

int32_t PartnerAgentExtensionStubImpl::CallbackExit([[maybe_unused]] uint32_t code, [[maybe_unused]] int32_t result)
{
    HILOGI("PartnerAgentExtensionStubImpl enter");
    return ERR_OK;
}

ErrCode PartnerAgentExtensionStubImpl::OnDeviceDiscovered(const PartnerDeviceAddress& deviceAddress,
    int32_t& retResult)
{
    HILOGI("OnDeviceDiscovered begin.");
#ifndef CROSS_PLATFORM
    int64_t beginTime = GetNowTimeMs();
#endif
    if (deviceAddress.GetAddress().empty()) {
        HILOGE("deviceAddress is empty.");
        return ERR_INVALID_DATA;
    }
    auto extension = extension_.lock();
    if (extension == nullptr) {
        HILOGE("extension is nullptr");
        return ERR_INVALID_DATA; 
    }
    retResult = static_cast<int32_t>(extension->OnDeviceDiscovered(deviceAddress));
#ifndef CROSS_PLATFORM
    NapiHaEventUtils::WriteHaEvent("extension.OnDeviceDiscovered", retResult, beginTime);
#endif
    HILOGI("OnDeviceDiscovered end successfully.");
    return ERR_OK;
}

ErrCode PartnerAgentExtensionStubImpl::OnDestroyWithReason(const int32_t reason, int32_t& retResult)
{
    HILOGI("OnDestroyWithReason begin.");
#ifndef CROSS_PLATFORM
    int64_t beginTime = GetNowTimeMs();
#endif
    auto extension = extension_.lock();
    if (extension != nullptr) {
        retResult = static_cast<int32_t>(extension->OnDestroyWithReason(reason));
#ifndef CROSS_PLATFORM
        NapiHaEventUtils::WriteHaEvent("extension.OnDestroyWithReason", retResult, beginTime);
#endif
        HILOGI("OnDestroyWithReason end successfully.");
        return ERR_OK;
    }
    HILOGE("OnDestroyWithReason end failed.");
    return ERR_INVALID_DATA;
}
} // namespace EventFwk
} // namespace OHOS
