/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of, use or distribute this file except in compliance with
 * the License. You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ohos.FusionConnectivity.ranging.ani.hpp"
#include "taihe_fusion_ranging_observer.h"
#include "log_utils.h"

ANI_EXPORT ani_status ANI_Constructor(ani_vm *vm, uint32_t *result)
{
    ani_env *env;
    if (vm == nullptr || result == nullptr) {
        return ANI_ERROR;
    }
    if (ANI_OK != vm->GetEnv(ANI_VERSION_1, &env)) {
        return ANI_ERROR;
    }
    if (ANI_OK != ohos::FusionConnectivity::ranging::ANIRegister(env)) {
        HILOGE("Error from ohos.FusionConnectivity.ranging::ANIRegister");
        return ANI_ERROR;
    }
    *result = ANI_VERSION_1;
    if (ANI_OK != OHOS::FusionRanging::TaiheInitFusionRangingObserver(vm)) {
        HILOGE("Error from TaiheInitFusionRangingObserver");
        return ANI_ERROR;
    }
    return ANI_OK;
}
