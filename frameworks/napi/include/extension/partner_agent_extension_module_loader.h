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

#ifndef PARTNER_AGENT_EXTENSION_MODULE_LOADER
#define PARTNER_AGENT_EXTENSION_MODULE_LOADER

#include "extension_module_loader.h"
#include "runtime.h"

namespace OHOS {
namespace FusionConnectivity {
class PartnerAgentExtensionModuleLoader : public AbilityRuntime::ExtensionModuleLoader,
                                            public Singleton<PartnerAgentExtensionModuleLoader> {
    DECLARE_SINGLETON(PartnerAgentExtensionModuleLoader);

public:
    /**
    * @brief Create Extension.
    *
    * @param runtime The runtime.
    * @return The Extension instance.
    */
    virtual AbilityRuntime::Extension* Create(const std::unique_ptr<AbilityRuntime::Runtime>& runtime) const override;

    /**
    * @brief Get the Params object
    *
    * @return std::map<std::string, std::string> The map of extension type and extension name.
    */
    virtual std::map<std::string, std::string> GetParams() override;
};
} // namespace FusionConnectivity
} // namespace OHOS
#endif // PARTNER_AGENT_EXTENSION_MODULE_LOADER
 