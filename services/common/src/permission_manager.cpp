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

#include "permission_manager.h"

#include "accesstoken_kit.h"
#include "ipc_skeleton.h"
#include "tokenid_kit.h"
#include "privacy_kit.h"
#include "log.h"

using namespace OHOS::Security::AccessToken;

namespace OHOS {
namespace FusionConnectivity {
bool PermissionManager::IsHapApp(uint32_t tokenId)
{
    ATokenTypeEnum callingType = AccessTokenKit::GetTokenTypeFlag(tokenId);
    return callingType == ATokenTypeEnum::TOKEN_HAP;
}

bool PermissionManager::IsNeedAddPermissionUsedRecord(const std::string &permission, uint32_t tokenId)
{
    return (permission == PERMISSION_ACCESS_BLUETOOTH) && IsHapApp(tokenId);
}

bool PermissionManager::VerifyPermission(const std::string &permission)
{
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    int result = AccessTokenKit::VerifyAccessToken(tokenId, permission);
    if (result == PermissionState::PERMISSION_GRANTED) {
        if (IsNeedAddPermissionUsedRecord(permission, tokenId)) {
            int ret = PrivacyKit::AddPermissionUsedRecord(tokenId, permission, 1, 0);
            if (ret != 0) {
                HILOGE("AddPermissionUsedRecord failed, ret is %{public}d", ret);
            }
        }
        return true;
    } else {
        if (IsNeedAddPermissionUsedRecord(permission, tokenId)) {
            int ret = PrivacyKit::AddPermissionUsedRecord(tokenId, permission, 0, 1);
            if (ret != 0) {
                HILOGE("AddPermissionUsedRecord failed, ret is %{public}d", ret);
            }
        }
        return false;
    }
}

FcmErrCode PermissionManager::VerifyPermissions(const PermissionItem &item)
{
    if (item.systemCallerNeeded_ && !IsSystemCaller()) {
        HILOGE("%{public}s is not system caller", GetCallingName().c_str());
        return FCM_ERR_SYSTEM_PERMISSION_FAILED;
    }

    for (const std::string &perm : item.permissions_) {
        bool isGranted = VerifyPermission(perm);
        if (!isGranted) {
            HILOGE("%{public}s has no permission: %{public}s", GetCallingName().c_str(), perm.c_str());
            return FCM_ERR_PERMISSION_FAILED;
        }
    }
    return FCM_NO_ERROR;
}

bool PermissionManager::IsSystemCaller()
{
    uint64_t fullTokenId = IPCSkeleton::GetCallingFullTokenID();
    bool isSystemApp = TokenIdKit::IsSystemAppByFullTokenID(fullTokenId);
    ATokenTypeEnum callingType = AccessTokenKit::GetTokenTypeFlag(static_cast<uint32_t>(fullTokenId));
    bool isSystemHap = callingType == ATokenTypeEnum::TOKEN_HAP && isSystemApp;
    bool isNativeCaller = callingType == ATokenTypeEnum::TOKEN_NATIVE || callingType == ATokenTypeEnum::TOKEN_SHELL;
    if (isSystemHap || isNativeCaller) {
        return true;
    }
    return false;
}

bool PermissionManager::IsNativeCaller()
{
    uint64_t fullTokenId = IPCSkeleton::GetCallingFullTokenID();
    ATokenTypeEnum callingType = AccessTokenKit::GetTokenTypeFlag(static_cast<uint32_t>(fullTokenId));
    if (callingType == ATokenTypeEnum::TOKEN_NATIVE || callingType == ATokenTypeEnum::TOKEN_SHELL) {
        return true;
    }
    return false;
}


bool PermissionManager::IsSystemHap()
{
    uint64_t fullTokenId = IPCSkeleton::GetCallingFullTokenID();
    bool isSystemApp = TokenIdKit::IsSystemAppByFullTokenID(fullTokenId);
    ATokenTypeEnum callingType = AccessTokenKit::GetTokenTypeFlag(static_cast<uint32_t>(fullTokenId));
    if (callingType == ATokenTypeEnum::TOKEN_HAP && isSystemApp) {
        return true;
    }
    return false;
}

std::string PermissionManager::GetCallingName()
{
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    ATokenTypeEnum callingType = AccessTokenKit::GetTokenTypeFlag(tokenId);
    switch (callingType) {
        case ATokenTypeEnum::TOKEN_HAP : {
            HapTokenInfo hapTokenInfo;
            if (AccessTokenKit::GetHapTokenInfo(tokenId, hapTokenInfo) == AccessTokenKitRet::RET_SUCCESS) {
                return hapTokenInfo.bundleName;
            }
            HILOGE("[PERMISSION] callingType(%{public}d), GetHapTokenInfo failed.", callingType);
            return "";
        }
        case ATokenTypeEnum::TOKEN_SHELL:
        case ATokenTypeEnum::TOKEN_NATIVE: {
            NativeTokenInfo nativeTokenInfo;
            if (AccessTokenKit::GetNativeTokenInfo(tokenId, nativeTokenInfo) == AccessTokenKitRet::RET_SUCCESS) {
                return nativeTokenInfo.processName;
            }
            HILOGE("[PERMISSION] callingType(%{public}d), GetNativeTokenInfo failed.", callingType);
            return "";
        }
        default:
            HILOGE("[PERMISSION] callingType(%{public}d) is invalid.", callingType);
            return "";
    }
}
}  // namespace FusionConnectivity
}  // namespace OHOS
