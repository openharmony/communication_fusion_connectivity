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
#define LOG_TAG "fcm_ha_event_utils"
#endif

#include "napi_ha_event_utils.h"
#include "log.h"
#include "fusion_connectivity_errorcode.h"
#ifndef CROSS_PLATFORM
#include "app_event.h"
#include "app_event_processor_mgr.h"
#endif

namespace OHOS {
namespace FusionConnectivity {

constexpr int64_t INVALID_PROCESSOR_ID = -200;
const std::string SDK_NAME = "FusionConnectivityKit";
constexpr int32_t TIME_MS_PER_SECOND = 1000;
constexpr int32_t TIME_NS_PER_MS = 1000000;

int64_t NapiHaEventUtils::processorId_ = -1;
std::mutex NapiHaEventUtils::processorLock_;
SafeMap<napi_env, int32_t> NapiHaEventUtils::envErrCodeMap_ = {};

NapiHaEventUtils::NapiHaEventUtils(napi_env env, const std::string &apiName): env_(env), apiName_(apiName)
{
    beginTime_ = GetNowTimeMs();
    GenerateProcessorId();
}

NapiHaEventUtils::~NapiHaEventUtils()
{
    WriteEndEvent();
}

void NapiHaEventUtils::WriteErrCode(napi_env env, const int32_t errCode)
{
    envErrCodeMap_.EnsureInsert(env, errCode);
}

void NapiHaEventUtils::WriteErrCode(const int32_t errCode)
{
    NapiHaEventUtils::WriteErrCode(env_, errCode);
}

void NapiHaEventUtils::GenerateProcessorId()
{
    std::lock_guard<std::mutex> lock(processorLock_);
    if (processorId_ == -1) {
        processorId_ = AddProcessor();
    }

    if (processorId_ == INVALID_PROCESSOR_ID) {
        HILOGE("invaild processorId !!!");
        return;
    }
}

int64_t NapiHaEventUtils::GetNowTimeMs() const
{
#ifndef CROSS_PLATFORM
    struct timespec ts = {};
    clock_gettime(CLOCK_BOOTTIME, &ts);
    return (int64_t)ts.tv_sec * TIME_MS_PER_SECOND + (int64_t)ts.tv_nsec / TIME_NS_PER_MS;
#else
    return -1;
#endif
}

int64_t NapiHaEventUtils::AddProcessor()
{
#ifndef CROSS_PLATFORM
    HiviewDFX::HiAppEvent::ReportConfig config;
    config.name = "ha_app_event";
    config.configName = "SDK_FCM";
    return HiviewDFX::HiAppEvent::AppEventProcessorMgr::AddProcessor(config);
#else
    return -1;
#endif
}

std::string NapiHaEventUtils::RandomTransId() const
{
    return std::string("transId_") + std::to_string(std::rand());
}

void NapiHaEventUtils::WriteEndEvent() const
{
#ifndef CROSS_PLATFORM
    HiviewDFX::HiAppEvent::Event event("api_diagnostic", "api_called_stat", HiviewDFX::HiAppEvent::BEHAVIOR);
    std::string transId = RandomTransId();
    int32_t errCode = FCM_NO_ERROR;
    errCode = envErrCodeMap_.ReadVal(env_);
    envErrCodeMap_.Erase(env_);
    event.AddParam("trans_id", transId);
    event.AddParam("api_name", apiName_);
    event.AddParam("sdk_name", SDK_NAME);
    event.AddParam("begin_time", beginTime_);
    event.AddParam("end_time", GetNowTimeMs());
    event.AddParam("result", (errCode == FCM_NO_ERROR ? 0 : 1));
    event.AddParam("error_code", errCode);
    int ret = Write(event);
    HILOGD("WriteEndEvent transId:%{public}s, apiName:%{public}s, sdkName:%{public}s, errCode:%{public}d,"
        "ret:%{public}d", transId.c_str(), apiName_.c_str(), SDK_NAME.c_str(), errCode, ret);
#endif
}

} // namespace FusionConnectivity
} // namespace OHOS
