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
#define LOG_TAG "PartnerAgentExtensionContext"
#endif

#include "partner_agent_extension_context.h"

#include "ability_business_error.h"
#include "ability_manager_client.h"
#include "ability_manager_errors.h"
#include "accesstoken_kit.h"
#include "ipc_skeleton.h"
#include "tokenid_kit.h"

namespace OHOS {
namespace FusionConnectivity {
const size_t PartnerAgentExtensionContext::CONTEXT_TYPE_ID(
    std::hash<const char*> {} ("PartnerAgentExtensionContext"));

PartnerAgentExtensionContext::PartnerAgentExtensionContext() {}

PartnerAgentExtensionContext::~PartnerAgentExtensionContext() {}
}  // namespace FusionConnectivity
}  // namespace OHOS