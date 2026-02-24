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

#include "common_utils.h"
#include "os_account_manager.h"
#include "log.h"

namespace OHOS {
namespace FusionConnectivity {
namespace {
constexpr size_t ADDRESS_LENGTH = 17;
constexpr size_t ADDRESS_COLON_INDEX = 2;
constexpr size_t ADDRESS_SEPARATOR_UNIT = 3;

constexpr int32_t DEFAULT_USER_ID = 100; // 100是指主用户空间
constexpr int32_t USER_ID_INIT = -1;
}  // namespace

bool IsValidAddress(const std::string &addr)
{
    if (addr.empty() || addr.length() != ADDRESS_LENGTH) {
        return false;
    }
    for (size_t i = 0; i < ADDRESS_LENGTH; i++) {
        char c = addr[i];
        switch (i % ADDRESS_SEPARATOR_UNIT) {
            case 0:
            case 1:
                if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
                    break;
                }
                return false;
            case ADDRESS_COLON_INDEX:
            default:
                if (c == ':') {
                    break;
                }
                return false;
        }
    }
    return true;
}

int32_t GetCurrentActiveUserId()
{
    int32_t userId = USER_ID_INIT;
    int32_t ret = OHOS::AccountSA::OsAccountManager::GetForegroundOsAccountLocalId(userId);
    if (ret != ERR_OK) {
        HILOGE("Current user get userId failed use default userId, code is %{public}d", ret);
        userId = DEFAULT_USER_ID;
    }
    return userId;
}

}  // namespace FusionConnectivity
}  // namespace OHOS
