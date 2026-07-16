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

#ifndef TAIHE_FUSION_CONNECTIVITY_UTILS_H
#define TAIHE_FUSION_CONNECTIVITY_UTILS_H

#include <memory>
#include <shared_mutex>
#include <vector>

#include "stdexcept"
#include "taihe/array.hpp"
#include "taihe/runtime.hpp"
namespace OHOS {
namespace FusionConnectivity {
void TaiheCreateLocalScope(ani_env *env);
void TaiheDestroyLocalScope(ani_env *env);
ani_env *GetCurrentEnv(ani_vm *vm, bool &isAttach);
}  // namespace FusionConnectivity
}  // namespace OHOS
#endif  // TAIHE_FUSION_CONNECTIVITY_UTILS_H