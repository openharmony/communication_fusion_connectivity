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
#define LOG_TAG "FusionConnLoadUtils"
#endif

#include "fusion_conn_load_utils.h"

#include <dlfcn.h>
#include "log.h"

namespace OHOS {
namespace FusionConnectivity {

FusionConnectivityLoadUtils::FusionConnectivityLoadUtils(const std::string& path) : path_(path)
{
    proxyHandle_ = dlopen(path_.c_str(), RTLD_NOW);
    if (proxyHandle_ == nullptr) {
        HILOGE("Open symbol failed, error: %{public}s", dlerror());
    } else {
        HILOGI("Open symbol success");
    }
}

FusionConnectivityLoadUtils::~FusionConnectivityLoadUtils()
{
    if (proxyHandle_ == nullptr) {
        return;
    }
    int result = dlclose(proxyHandle_);
    HILOGI("Release symbol %{public}d", result);
    proxyHandle_ = nullptr;
}

void* FusionConnectivityLoadUtils::GetProxyFunc(const std::string& func)
{
    if (proxyHandle_ == nullptr) {
        HILOGE("Get func failed: %{public}s", func.c_str());
        return nullptr;
    }
    return dlsym(proxyHandle_, func.c_str());
}

bool FusionConnectivityLoadUtils::IsValid()
{
    return (proxyHandle_ != nullptr);
}

}
}
