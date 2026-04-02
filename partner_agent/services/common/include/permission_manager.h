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

#ifndef PERMISSION_MANAGER_H
#define PERMISSION_MANAGER_H

#include "permission_item.h"
#include "fusion_connectivity_errorcode.h"

namespace OHOS {
namespace FusionConnectivity {
constexpr static const char *PERMISSION_ACCESS_BLUETOOTH = "ohos.permission.ACCESS_BLUETOOTH";

#define NO_NEED_CHECK_PERMISSION PermissionItem(std::set<std::string>{})

class PermissionManager {
public:
    static bool VerifyPermission(const std::string &permission);
    static FcmErrCode VerifyPermissions(const PermissionItem &item);
    static bool IsSystemCaller();
    static bool IsNativeCaller();
    static bool IsSystemHap();
    static std::string GetCallingName();
    static bool IsHapApp(uint32_t tokenId);
    static bool IsNeedAddPermissionUsedRecord(const std::string &permission, uint32_t tokenId);
};
}  // namespace FusionConnectivity
}  // namespace OHOS
#endif  // PERMISSION_MANAGER_H
