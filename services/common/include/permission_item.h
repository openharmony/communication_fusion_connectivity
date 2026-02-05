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

#ifndef PERMISSION_ITEM_H
#define PERMISSION_ITEM_H

#include <string>
#include <set>

namespace OHOS {
namespace FusionConnectivity {
static constexpr bool SYSTEM_API = true;
static constexpr bool PUBLIC_API = false;
struct PermissionItem {
    explicit PermissionItem(bool isSystemApi, const char* permission)
        : systemCallerNeeded_(isSystemApi), permissions_(std::set<std::string>{permission}) {}
    explicit PermissionItem(bool isSystemApi, std::set<std::string> permissions)
        : systemCallerNeeded_(isSystemApi), permissions_(permissions) {}
    ~PermissionItem() = default;

    bool systemCallerNeeded_ = false;
    std::set<std::string> permissions_ {};
};
}  // namespace FusionConnectivity
}  // namespace OHOS
#endif  // PERMISSION_ITEM_H
