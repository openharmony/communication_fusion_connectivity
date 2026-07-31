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

#ifndef TAIHE_FUSION_CONNECTIVITY_ERROR_H
#define TAIHE_FUSION_CONNECTIVITY_ERROR_H

#include <cstdint>
#include <string>
#include <optional>
#include <vector>

#include "fusion_connectivity_errorcode.h"
#include "fusion_ranging_errorcode.h"
#include "log_utils.h"

namespace OHOS {
namespace FusionConnectivity {

#ifndef TAIHE_FC_ASSERT_RETURN
#define TAIHE_FC_ASSERT_RETURN(cond, errCode, retObj)                                      \
    do {                                                                                   \
        if (!(cond)) {                                                                     \
            HandleSyncErr((errCode));                                                      \
            HILOGE("FusionConnectivity taihe assert failed, errCode=%{public}d", errCode); \
            return (retObj);                                                               \
        }                                                                                  \
    } while (0)
#endif

#ifndef TAIHE_FC_ASSERT_RETURN_VOID
#define TAIHE_FC_ASSERT_RETURN_VOID(cond, errCode)                                         \
    do {                                                                                   \
        if (!(cond)) {                                                                     \
            HandleSyncErr((errCode));                                                      \
            HILOGE("FusionConnectivity taihe assert failed, errCode=%{public}d", errCode); \
            return;                                                                        \
        }                                                                                  \
    } while (0)
#endif

#ifndef TAIHE_FC_ASSERT_RETURN_VERIFY
#define TAIHE_FC_ASSERT_RETURN_VERIFY(cond, errCode, retObj)           \
    do {                                                               \
        std::vector<int32_t> validErrCodes = apiContext.validErrCodes; \
        if (!(cond)) {                                                 \
            HandleSyncErrAdapter((errCode), validErrCodes);            \
            return (retObj);                                           \
        }                                                              \
    } while (0)
#endif

#ifndef TAIHE_FC_ASSERT_RETURN_VOID_VERIFY
#define TAIHE_FC_ASSERT_RETURN_VOID_VERIFY(cond, errCode)              \
    do {                                                               \
        std::vector<int32_t> validErrCodes = apiContext.validErrCodes; \
        if (!(cond)) {                                                 \
            HandleSyncErrAdapter((errCode), validErrCodes);            \
        }                                                              \
    } while (0)
#endif

struct ApiContext {
    std::vector<int32_t> validErrCodes{};
};

#ifndef TAIHE_FC_CONTEXT_WITHOUT_HA
#define TAIHE_FC_CONTEXT_WITHOUT_HA(validErrCodes)     \
ApiContext apiContext = ApiContext{                    \
    validErrCodes                                      \
}
#endif

struct ErrInfo {
    int32_t errCode;
    std::string errMsg;
};

std::string GetTaiheErrMsg(const int32_t errCode);
void HandleSyncErr(int32_t errCode);
void HandleSyncErrWithValidCodes(int32_t errCode, const std::vector<int32_t> &validErrCodes);
void HandleSyncErrAdapter(int32_t errCode, const std::vector<int32_t> &validErrCodes);
ErrInfo ProcessErrCode(int32_t originalCode, const std::vector<int32_t> &validErrCodes);

struct TaihePromiseAndCallback {
    bool success;
    FcmErrCode errorCode;
    std::optional<uintptr_t> object;

    static TaihePromiseAndCallback Success(uintptr_t object)
    {
        return {true, FcmErrCode::FCM_NO_ERROR, object};
    }

    static TaihePromiseAndCallback Failure(FcmErrCode code)
    {
        return {false, code, std::nullopt};
    }
};
}  // namespace FusionConnectivity
}  // namespace OHOS
#endif  // TAIHE_FUSION_CONNECTIVITY_ERROR_H