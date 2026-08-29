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

#ifndef ETS_PATRNER_AGENT_EXTENSION_H
#define ETS_PATRNER_AGENT_EXTENSION_H
#include <atomic>
#include <memory>
#include <utility>

#include "ani.h"
#include "common_event_data.h"
#include "ets_runtime.h"
#include "partner_agent_extension.h"
#include "ets_native_reference.h"

namespace OHOS {
namespace FusionConnectivity {

class ETSPartnerAgentExtension : public PartnerAgentExtension {
public:
    explicit ETSPartnerAgentExtension(AbilityRuntime::ETSRuntime& etsRuntime);
    virtual ~ETSPartnerAgentExtension() override;

    static PartnerAgentExtension* Create(const std::unique_ptr<AbilityRuntime::Runtime>& runtime) = delete;

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

private:
    void GetSrcPath(std::string& srcPath);
    void UpdatePartnerAgentExtensionObj(std::shared_ptr<AbilityRuntime::AbilityInfo> &abilityInfo,
        const std::string &moduleName, const std::string &srcPath);
    void BindContext(std::shared_ptr<AbilityRuntime::AbilityInfo> &abilityInfo,
        std::shared_ptr<AAFwk::Want> want, const std::string &moduleName, const std::string &srcPath);
    ani_ref CallObjectMethod(bool withResult, const char* name, const char* signature, ...);
    ani_object PartnerDeviceAddressToAni(ani_env* env, const PartnerDeviceAddress& deviceAddress);
    ani_object BluetoothAddressToAni(ani_env* env, const PartnerDeviceAddress& deviceAddress);
    ani_object CreateAniBluetoothAddress(ani_env* env);
    bool SetAniBluetoothAddressProperties(ani_env* env, ani_object aniBluetoothAddress,
        const PartnerDeviceAddress& deviceAddress);
    bool SetAniEnumProperty(ani_env* env, ani_object targetObj, const char* propName,
        const char* enumName, int32_t enumValue,
        const std::pair<int32_t, const char *> *valueNameMap, size_t mapSize);
    ani_string GetAniString(ani_env *env, const std::string &str);
    void CallOnDestroyWithReason(ani_env* env, int32_t reason);

    AbilityRuntime::ETSRuntime& etsRuntime_;
    std::shared_ptr<AppExecFwk::ETSNativeReference> etsAbilityObj_;
    std::atomic<bool> notified_ { false };
};
} // namespace FusionConnectivity
} // namespace OHOS

#endif // ETS_PATRNER_AGENT_EXTENSION_H
