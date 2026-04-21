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
#include "iremote_object.h"
#include "ranging_capability_supported.h"
#include "ranging_params.h"
#include "ranging_result_observer_stub.h"
#include "ranging_state_observer_stub.h"
#include "system_ability.h"

namespace OHOS {
namespace FusionRanging {

class RangingAppStateObserver;
class SessionManager;
class StateObserverRegistry;
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

    int32_t CallbackEnter([[maybe_unused]] uint32_t code) override
    {
        return 0;
    }

    int32_t CallbackExit([[maybe_unused]] uint32_t code, [[maybe_unused]] int32_t result) override
    {
        return 0;
    }

    ErrCode GetRangingCapability(RangingCapabilitySupported &capability) override;
    ErrCode StartRanging(const RangingParams &params, const sptr<IRangingResultObserver> &observer) override;
    ErrCode StopRanging(const std::string &deviceId, const sptr<IRangingResultObserver> &observer) override;
    ErrCode RegisterStateObserver(const sptr<IRangingStateObserver> &observer) override;
    ErrCode UnregisterStateObserver(const sptr<IRangingStateObserver> &observer) override;

    void PauseRangingByUid(int32_t uid);
    void ResumeRangingByUid(int32_t uid);
    void StopRangingByUid(int32_t uid);
    void CleanupAll();
    void CheckAndUnloadSA();

private:
    void NotifyStateObservers(int32_t uid, const RangingStateChangeInfo &info);
    void HandleProcessDeath(int32_t uid);
    void CheckAndUnloadIfIdle();
    void StopAllDeviceIdsByUid(int32_t uid);

    std::unique_ptr<SessionManager> sessionManager_;
    std::unique_ptr<StateObserverRegistry> stateObserverRegistry_;
    std::unique_ptr<ProcessDeathManager> processDeathManager_;
    std::unique_ptr<RangingAppStateObserver> appStateObserver_;
    std::atomic<bool> isStopping_ = false;
};

}  // namespace FusionRanging
}  // namespace OHOS

#endif  // FUSION_RANGING_SERVER_H