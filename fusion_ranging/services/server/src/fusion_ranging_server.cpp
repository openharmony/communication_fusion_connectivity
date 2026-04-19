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

#ifndef LOG_TAG
#define LOG_TAG "FusionRangingServer"
#endif

#include "fusion_ranging_server.h"

#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <vector>

#include "app_state_observer.h"
#include "fusion_ranging_errorcode.h"
#include "fusion_ranging_service.h"
#include "ipc_skeleton.h"
#include "log_util.h"
#include "process_death_manager.h"
#include "session_manager.h"
#include "state_observer_registry.h"

namespace OHOS {
namespace FusionRanging {

constexpr int32_t FUSION_RANGING_SYS_ABILITY_ID = 8631;

const bool REGISTER_RESULT = SystemAbility::MakeAndRegisterAbility(FusionRnaingServer::GetInstance().GetRefPtr());

} // namespace FusionRanging 
} // namespace OHOS