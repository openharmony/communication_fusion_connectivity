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
#define LOG_TAG "taihe_fusion_connectivity_utils"
#endif

#include "taihe_fusion_connectivity_utils.h"
#include "taihe_fusion_connectivity_error.h"
#include <string>
#include "fusion_ranging_errorcode.h"
#include "log_utils.h"

namespace OHOS {
namespace FusionConnectivity {

void TaiheCreateLocalScope(ani_env *env)
{
    ani_size nr_refs = 16;
    ani_status status = env->CreateLocalScope(nr_refs);
    if (status != ANI_OK) {
        HILOGE("CreateLocalScope failed, status(%{public}d)", status);
    }
}

void TaiheDestroyLocalScope(ani_env *env)
{
    ani_status status = env->DestroyLocalScope();
    if (status != ANI_OK) {
        HILOGE("DestroyLocalScope failed, status(%{public}d)", status);
    }
}

ani_env *GetCurrentEnv(ani_vm *vm, bool &isAttach)
{
    if (vm == nullptr) {
        HILOGE("null vm");
        return nullptr;
    }
    ani_env *threadEnv;
    if (ANI_OK != vm->GetEnv(ANI_VERSION_1, &threadEnv)) {
        HILOGE("GetEnv failed, AttachCurrentThread");
        ani_options aniArgs{0, nullptr};
        ani_status status = vm->AttachCurrentThread(&aniArgs, ANI_VERSION_1, &threadEnv);
        if (status != ANI_OK) {
            HILOGE("GetCurrentEnv failed, status(%{public}d)", status);
            return nullptr;
        }
        isAttach = true;
    }
    return threadEnv;
}
}  // namespace FusionConnectivity
}  // namespace OHOS