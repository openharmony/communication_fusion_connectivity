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

#ifndef JS_PATRNER_AGENT_EXTENSION_H
#define JS_PATRNER_AGENT_EXTENSION_H
#include <memory>

#include "common_event_data.h"
#include "js_runtime.h"
#include "native_engine/native_reference.h"
#include "native_engine/native_value.h"
#include "runtime.h"
#include "partner_agent_extension.h"


namespace OHOS {
namespace FusionConnectivity {
class JsPartnerAgentExtension : public PartnerAgentExtension {
public:
    explicit JsPartnerAgentExtension(AbilityRuntime::JsRuntime& jsRuntime);
    virtual ~JsPartnerAgentExtension() override;

    /**
        * @brief Create JsPartnerAgentExtension.
        *
        * @param runtime The runtime.
        * @return The JsPartnerAgentExtension instance.
        */
    static JsPartnerAgentExtension* Create(const std::unique_ptr<AbilityRuntime::Runtime>& runtime);

    void Init(const std::shared_ptr<AppExecFwk::AbilityLocalRecord>& record,
                const std::shared_ptr<AppExecFwk::OHOSApplication>& application,
                std::shared_ptr<AppExecFwk::AbilityHandler>& handler,
                const sptr<IRemoteObject>& token) override;

    void OnStart(const AAFwk::Want& want) override;

    sptr<IRemoteObject> OnConnect(const AAFwk::Want& want) override;

    void OnDisconnect(const AAFwk::Want& want) override;

    void OnStop() override;

    virtual PartnerAgentExtensionResult OnDestroyWithReason(const int32_t reason) override;

    virtual PartnerAgentExtensionResult OnDeviceDiscovered(const PartnerDeviceAddress& deviceAddress) override;

    void ExecNapiWrap(napi_env env, napi_value obj);

    std::weak_ptr<JsPartnerAgentExtension> GetWeakPtr();

private:
    AbilityRuntime::JsRuntime& jsRuntime_;
    std::unique_ptr<NativeReference> jsObj_;
};
} // namespace FusionConnectivity
} // namespace OHOS

#endif // JS_PATRNER_AGENT_EXTENSION_H
