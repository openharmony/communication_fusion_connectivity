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
#define LOG_TAG "PartnerAgentExtension"
#endif

#include "log.h"
#include "js_partner_agent_extension.h"
#include "partner_agent_extension.h"

namespace OHOS {
namespace FusionConnectivity {
using namespace OHOS::AppExecFwk;
PartnerAgentExtension* PartnerAgentExtension::Create(
    const std::unique_ptr<AbilityRuntime::Runtime>& runtime)
{
    HILOGI("Create begin.");
    if (!runtime) {
        HILOGI("Create !runtime.");
        return new (std::nothrow) PartnerAgentExtension();
    }

    switch (runtime->GetLanguage()) {
        case AbilityRuntime::Runtime::Language::JS:
            return JsPartnerAgentExtension::Create(runtime);
        default:
            return new (std::nothrow) PartnerAgentExtension();
    }
}

void PartnerAgentExtension::Init(const std::shared_ptr<AbilityLocalRecord>& record,
    const std::shared_ptr<OHOSApplication>& application,
    std::shared_ptr<AbilityHandler>& handler,
    const sptr<IRemoteObject>& token)
{
    HILOGI("Init begin.");
    ExtensionBase<PartnerAgentExtensionContext>::Init(record, application, handler, token);
    HILOGI("Init success.");
}

std::shared_ptr<PartnerAgentExtensionContext> PartnerAgentExtension::CreateAndInitContext(
    const std::shared_ptr<AbilityLocalRecord>& record,
    const std::shared_ptr<OHOSApplication>& application,
    std::shared_ptr<AbilityHandler>& handler,
    const sptr<IRemoteObject>& token)
{
    HILOGI("CreateAndInitContext begin.");
    std::shared_ptr<PartnerAgentExtensionContext> context =
        ExtensionBase<PartnerAgentExtensionContext>::CreateAndInitContext(
            record, application, handler, token);
    if (context == nullptr) {
        HILOGE("context is null.");
        return context;
    }
    HILOGI("CreateAndInitContext end.");
    return context;
}

PartnerAgentExtensionResult PartnerAgentExtension::OnDeviceDiscovered(const PartnerDeviceAddress& deviceAddress)
{
    HILOGI("OnDeviceDiscovered begin.");
    return PartnerAgentExtensionResult::OK;
}

PartnerAgentExtensionResult PartnerAgentExtension::OnDestroyWithReason(const int32_t reason)
{
    HILOGI("OnDestroyWithReason begin.");
    return PartnerAgentExtensionResult::OK;
}

}  // namespace FusionConnectivity
}  // namespace OHOS