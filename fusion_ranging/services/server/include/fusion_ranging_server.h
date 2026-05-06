/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef FUSION_RANGING_SERVER_H
#define FUSION_RANGING_SERVER_H

#include <atomic>
#include <memory>
#include <string>
#include "fusion_ranging_stub.h"
#include "permission_manager.h"
#include "fusion_connectivity_errorcode.h"
#include "if_system_ability_manager.h"
#include "iremote_object.h"
#include "iservice_registry.h"
#include "ranging_capability_supported.h"
#include "ranging_params.h"
#include "ranging_observer_stub.h"
#include "base_def.h"
#include "system_ability.h"
#include "safe_map.h"
#include "app_mgr_interface.h"

namespace OHOS {
namespace FusionRanging {

class RangingAppStateObserver;
class SessionManager;
class ProcessDeathManager;

class FusionRangingServer
    : public SystemAbility, public FusionRangingStub {
    DECLARE_SYSTEM_ABILITY(FusionRangingServer);

public:
    static sptr<FusionRangingServer> GetInstance();
    void OnStart() override;
    void OnStop() override;

    FusionRangingServer();
    ~FusionRangingServer();

    int32_t CallbackEnter(uint32_t code) override;
    int32_t CallbackExit([[maybe_unused]] uint32_t code, [[maybe_unused]] int32_t result) override
    {
        return 0;
    }

    ErrCode GetRangingCapability(RangingCapabilitySupported &capability) override;
    ErrCode StartRanging(const RangingParams &params) override;
    ErrCode StopRanging(const RangingParams &params) override;
    ErrCode StartPassiveRanging(int32_t capabilityType, int32_t &handle) override;
    ErrCode StopPassiveRanging(int32_t capabilityType, int32_t handle) override;
    ErrCode RegisterObserver(const sptr<IRangingObserver> &observer) override;
    ErrCode DeregisterObserver(const sptr<IRangingObserver> &observer) override;

    void OnForegroundApplicationChanged(const AppExecFwk::AppStateData &appStateData);
    void HandleProcessDeath(int32_t uid);

private:
    void NotifyStateObservers(int32_t uid, RangingState state, RangingStoppedCause cause);
    void CheckAndUnloadIfIdle();
    void CheckAndUnloadSA();
    void InitializePermissionsMap();

    static sptr<FusionRangingServer> instance_;
    static std::mutex instanceLock_;
    std::atomic<bool> isStopping_ = false;
    std::map<int, FusionConnectivity::PermissionItem> permissionsMap_{};

    DECLARE_IMPL();
};
}  // namespace FusionRanging
}  // namespace OHOS
#endif  // FUSION_RANGING_SERVER_H