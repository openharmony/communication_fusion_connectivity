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
#define LOG_TAG "EtsPartnerAgentExtensionContext"
#endif

#include "ets_partner_agent_extension_context.h"
#include "log.h"
#include "ets_extension_context.h"

namespace OHOS {
namespace FusionConnectivity {
using namespace OHOS::AbilityRuntime;
constexpr const char *PARTNER_AGENT_EXTENSION_CONTEXT_CLASS_NAME =
    "@ohos.FusionConnectivity.PartnerAgentExtensionContext.PartnerAgentExtensionContext";

ani_object CreateEtsPartnerAgentExtensionContext(ani_env *env, std::shared_ptr<PartnerAgentExtensionContext> context)
{
    HILOGI("Create ets partner agent extension context");
    if (env == nullptr || context == nullptr) {
        HILOGE("env or context is nullptr");
        return nullptr;
    }
    ani_class cls = nullptr;
    ani_object contextObj = nullptr;
    ani_field field = nullptr;
    ani_method method = nullptr;
    ani_status status = ANI_ERROR;
    if ((status = env->FindClass(PARTNER_AGENT_EXTENSION_CONTEXT_CLASS_NAME, &cls)) != ANI_OK || cls == nullptr) {
        HILOGE("FindClass failed, status: %d", status);
        return nullptr;
    }
    if ((status = env->Class_FindMethod(cls, "<ctor>", nullptr, &method)) != ANI_OK || method == nullptr) {
        HILOGE("FindMethod failed, status: %{public}d", status);
        return nullptr;
    }
    if ((status = env->Object_New(cls, method, &contextObj)) != ANI_OK || contextObj == nullptr) {
        HILOGE("Object_New failed, status: %{public}d", status);
        return nullptr;
    }
    if ((status = env->Class_FindField(cls, "nativePartnerAgentExtensionContext", &field)) != ANI_OK ||
        field == nullptr) {
        HILOGE("FindField failed, status: %{public}d", status);
        return nullptr;
    }
    ani_long nativeContextLong = (ani_long)context.get();
    if ((status = env->Object_SetField_Long(contextObj, field, nativeContextLong)) != ANI_OK) {
        HILOGE("set field status: %{public}d", status);
        return nullptr;
    }
    OHOS::AbilityRuntime::CreateEtsExtensionContext(env, cls, contextObj, context, context->GetAbilityInfo());
    return contextObj;
}
}  // namespace FusionConnectivity
}  // namespace OHOS
