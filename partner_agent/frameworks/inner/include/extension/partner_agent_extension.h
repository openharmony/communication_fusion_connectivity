/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributd under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef PARTNER_AGENT_EXTENSION_H
#define PARTNER_AGENT_EXTENSION_H

#include "common_event_data.h"
#include "extension_base.h"
#include "runtime.h"
#include "partner_agent_extension_context.h"
#include "partner_device_address.h"

namespace OHOS {
namespace FusionConnectivity {
enum class PartnerAgentExtensionResult : int32_t {
    OK = 0,
    INVALID_PARAM,
    INTERNAL_ERROR,
    OBJECT_RELEASED,
    SET_OBJECT_FAIL,
    GET_OBJECT_FAIL,
    GET_METHOD_FAIL,
    CALL_METHOD_FAIL,
};
class PartnerAgentExtension : public AbilityRuntime::ExtensionBase<PartnerAgentExtensionContext> {
public:
    PartnerAgentExtension() = default;
    virtual ~PartnerAgentExtension() = default;

    /**
     * @brief Create and init context.
     *
     * @param record the extension record.
     * @param application the application info.
     * @param handler the extension handler.
     * @param token the remote token.
     * @return The created context.
     */
    virtual std::shared_ptr<PartnerAgentExtensionContext> CreateAndInitContext(
        const std::shared_ptr<AppExecFwk::AbilityLocalRecord>& record,
        const std::shared_ptr<AppExecFwk::OHOSApplication>& application,
        std::shared_ptr<AppExecFwk::AbilityHandler>& handler,
        const sptr<IRemoteObject>& token) override;

    /**
     * @brief Init the extension.
     *
     * @param record the extension record.
     * @param application the application info.
     * @param handler the extension handler.
     * @param token the remote token.
     */
    virtual void Init(const std::shared_ptr<AppExecFwk::AbilityLocalRecord>& record,
        const std::shared_ptr<AppExecFwk::OHOSApplication>& application,
        std::shared_ptr<AppExecFwk::AbilityHandler>& handler,
        const sptr<IRemoteObject>& token) override;

    /**
     * @brief Create Extension.
     *
     * @param runtime The runtime.
     * @return The PartnerAgentExtension instance.
     */
    static PartnerAgentExtension* Create(const std::unique_ptr<AbilityRuntime::Runtime>& runtime);

    virtual PartnerAgentExtensionResult OnDeviceDiscovered(const PartnerDeviceAddress& deviceAddress);

    virtual PartnerAgentExtensionResult OnDestroyWithReason(const int32_t reason);
};
}  // namespace FusionConnectivity
}  // namespace OHOS

#endif  // PARTNER_AGENT_EXTENSION_H