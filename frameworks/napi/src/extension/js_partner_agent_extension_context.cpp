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
#define LOG_TAG "JsPartnerAgentExtensionContext"
#endif

#include "js_partner_agent_extension_context.h"

#include "ability_business_error.h"
#include "js_error_utils.h"
#include "js_extension_context.h"
#include "js_runtime.h"
#include "js_runtime_utils.h"
#include "napi/native_api.h"
#include "napi_common_want.h"
#include "want.h"
#include "log.h"

namespace OHOS {
namespace FusionConnectivity {
std::shared_ptr<PartnerAgentExtensionContext> JsPartnerAgentExtensionContext::GetAbilityContext()
{
    HILOGI("GetAbilityContext");
    return context_.lock();
}

napi_value CreateJsPartnerAgentExtensionContext(napi_env env,
    std::shared_ptr<PartnerAgentExtensionContext> context)
{
    HILOGI("Create js partner agent extension context");
    std::shared_ptr<OHOS::AppExecFwk::AbilityInfo> abilityInfo = nullptr;
    if (context) {
        abilityInfo = context->GetAbilityInfo();
    }

    napi_value objValue = CreateJsExtensionContext(env, context, abilityInfo);
    std::unique_ptr<JsPartnerAgentExtensionContext> jsContext =
        std::make_unique<JsPartnerAgentExtensionContext>(context);
    napi_wrap(
        env, objValue, jsContext.release(), JsPartnerAgentExtensionContext::Finalizer, nullptr, nullptr);

    return objValue;
}
}  // namespace FusionConnectivity
}  // namespace OHOS