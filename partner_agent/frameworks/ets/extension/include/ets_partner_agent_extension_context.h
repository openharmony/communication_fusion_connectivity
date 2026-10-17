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

#ifndef ETS_PARTNER_AGENT_EXTENSION_CONTEXT_H
#define ETS_PARTNER_AGENT_EXTENSION_CONTEXT_H

#include <memory>
#include "ani.h"
#include "partner_agent_extension_context.h"

namespace OHOS {
namespace FusionConnectivity {

ani_object CreateEtsPartnerAgentExtensionContext(
    ani_env *env, std::shared_ptr<PartnerAgentExtensionContext> context);

} // namespace FusionConnectivity
} // namespace OHOS

#endif // ETS_PARTNER_AGENT_EXTENSION_CONTEXT_H
