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

#ifndef JS_PARTNER_AGENT_EXTENSION_CONTEXT_H
#define JS_PARTNER_AGENT_EXTENSION_CONTEXT_H

#include <memory>

#include "partner_agent_extension_context.h"
#include "native_engine/native_engine.h"

namespace OHOS {
namespace FusionConnectivity {
napi_value CreateJsPartnerAgentExtensionContext(napi_env env,
    std::shared_ptr<PartnerAgentExtensionContext> context);

class JsPartnerAgentExtensionContext final {
public:
    explicit JsPartnerAgentExtensionContext(
        const std::shared_ptr<PartnerAgentExtensionContext>& context): context_(context) {}
    ~JsPartnerAgentExtensionContext() = default;

    static void Finalizer(napi_env env, void* data, void* hint)
    {
        std::unique_ptr<JsPartnerAgentExtensionContext>(
            static_cast<JsPartnerAgentExtensionContext*>(data));
    }

    std::shared_ptr<PartnerAgentExtensionContext> GetAbilityContext();
private:
    std::weak_ptr<PartnerAgentExtensionContext> context_;
};
} // namespace FusionConnectivity
} // namespace OHOS

#endif // JS_PARTNER_AGENT_EXTENSION_CONTEXT_H
 