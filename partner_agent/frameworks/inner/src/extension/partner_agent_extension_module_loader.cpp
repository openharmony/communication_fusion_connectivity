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
#define LOG_TAG "PartnerAgentExtensionModuleLoader"
#endif

#include "partner_agent_extension_module_loader.h"

#include "partner_agent_extension.h"
#include "js_partner_agent_extension.h"
#include "log.h"

#include <dlfcn.h>

namespace OHOS {
namespace FusionConnectivity {

PartnerAgentExtensionModuleLoader::PartnerAgentExtensionModuleLoader() = default;
PartnerAgentExtensionModuleLoader::~PartnerAgentExtensionModuleLoader() = default;

namespace {

PartnerAgentExtension* CreateAniExtension(const std::unique_ptr<AbilityRuntime::Runtime>& runtime)
{
    HILOGI("CreateAniExtension begin.");
    static void* handle = dlopen("libpartner_agent_extension_ani.z.so", RTLD_LAZY);
    if (handle == nullptr) {
        HILOGE("Failed to open ani library.");
        return nullptr;
    }
    const std::string funcName = "OHOS_STS_PartnerAgentExtension_Creation";
    using PartnerAgentExtensionCreateFunc = PartnerAgentExtension* (*)(const std::unique_ptr<AbilityRuntime::Runtime>&);
    auto createFunc = reinterpret_cast<PartnerAgentExtensionCreateFunc>(dlsym(handle, funcName.c_str()));
    if (createFunc == nullptr) {
        HILOGE("Failed to find ani extension create function.");
        return nullptr;
    }
    return createFunc(runtime);
}

} // namespace

AbilityRuntime::Extension* PartnerAgentExtensionModuleLoader::Create(
    const std::unique_ptr<AbilityRuntime::Runtime>& runtime) const
{
    HILOGD("Create module loader.");
    if (runtime) {
        switch (runtime->GetLanguage()) {
            case AbilityRuntime::Runtime::Language::JS:
                return JsPartnerAgentExtension::Create(runtime);
            case AbilityRuntime::Runtime::Language::ETS:
                return CreateAniExtension(runtime);
            default:
                break;
        }
    }
    return PartnerAgentExtension::Create(runtime);
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
