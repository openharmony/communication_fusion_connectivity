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
#define LOG_TAG "JsPartnerAgentExtensionLoader"
#endif

#include "partner_agent_extension_module_loader.h"

#include "bundle_info.h"
#include "partner_agent_extension.h"
#include "js_partner_agent_extension.h"
#include "log.h"

#include <dlfcn.h>

constexpr char STS_PARTNER_AGENT_EXT_LIB_NAME[] = "libpartner_agent_extension_ani.z.so";
static constexpr char STS_PARTNER_AGENT_EXT_CREATE_FUNC[] =
    "OHOS_STS_PartnerAgentExtension_Creation";

namespace OHOS {
namespace FusionConnectivity {
typedef PartnerAgentExtension* (*CREATE_FUNC)(const std::unique_ptr<AbilityRuntime::Runtime>& runtime);

__attribute__((no_sanitize("cfi"))) PartnerAgentExtension* CreateAniExtension(
    const std::unique_ptr<AbilityRuntime::Runtime>& runtime)
{
    void *handle = dlopen(STS_PARTNER_AGENT_EXT_LIB_NAME, RTLD_LAZY);
    if (handle == nullptr) {
        HILOGE("open sts_partner_agent_extension library %{public}s failed, reason: %{public}sn",
            STS_PARTNER_AGENT_EXT_LIB_NAME, dlerror());
        return new (std::nothrow) PartnerAgentExtension();
    }

    auto func = reinterpret_cast<CREATE_FUNC>(dlsym(handle, STS_PARTNER_AGENT_EXT_CREATE_FUNC));
    if (func == nullptr) {
        dlclose(handle);
        HILOGE("get sts_partner_agent_extension symbol %{public}s in %{public}s failed, reason: %{public}sn",
            STS_PARTNER_AGENT_EXT_CREATE_FUNC, STS_PARTNER_AGENT_EXT_LIB_NAME, dlerror());
        return new (std::nothrow) PartnerAgentExtension();
    }

    auto instance = func(runtime);
    if (instance == nullptr) {
        dlclose(handle);
        HILOGE("get sts_partner_agent_extension instance in %{public}s failed, reason: %{public}sn",
            STS_PARTNER_AGENT_EXT_CREATE_FUNC, dlerror());
        return new (std::nothrow) PartnerAgentExtension();
    }
    return instance;
}

PartnerAgentExtensionModuleLoader::PartnerAgentExtensionModuleLoader() = default;
PartnerAgentExtensionModuleLoader::~PartnerAgentExtensionModuleLoader() = default;

AbilityRuntime::Extension* PartnerAgentExtensionModuleLoader::Create(
    const std::unique_ptr<AbilityRuntime::Runtime>& runtime) const
{
    HILOGD("Create module loader.");
    if (!runtime) {
        return PartnerAgentExtension::Create(runtime);
    }

    switch (runtime->GetLanguage()) {
        case AbilityRuntime::Runtime::Language::JS:
            return JsPartnerAgentExtension::Create(runtime);
        case AbilityRuntime::Runtime::Language::ETS:
            return CreateAniExtension(runtime);
        default:
            return PartnerAgentExtension::Create(runtime);
    }
}

std::map<std::string, std::string> PartnerAgentExtensionModuleLoader::GetParams()
{
    std::map<std::string, std::string> params;
    // type means extension type in ExtensionAbilityType of extension_ability_info.h
    params.insert(std::pair<std::string, std::string>(
        "type", std::to_string(static_cast<int32_t>(AppExecFwk::ExtensionAbilityType::PARTNER_AGENT))));
    // extension name
    params.insert(std::pair<std::string, std::string>("name", "PartnerAgentExtension"));
    return params;
}

extern "C" __attribute__((visibility("default"))) void* OHOS_EXTENSION_GetExtensionModule()
{
    return &PartnerAgentExtensionModuleLoader::GetInstance();
}
} // namespace FusionConnectivity
} // namespace OHOS
 