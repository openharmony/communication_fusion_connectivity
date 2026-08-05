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

#ifndef NAPI_FUSION_CONNECTIVITY_ERROR_H
#define NAPI_FUSION_CONNECTIVITY_ERROR_H

#include <cstdint>
#include <string>
#include "log.h"
#include "napi/native_api.h"
#include "fusion_connectivity_errorcode.h"
#include "napi_ha_event_utils.h"

#define NAPI_FCM_CALL_RETURN(func)                                          \
    do {                                                                   \
        napi_status ret = (func);                                          \
        if (ret != napi_ok) {                                              \
            HILOGE("napi call function failed. ret:%{public}d", ret);      \
            return ret;                                                    \
        }                                                                  \
    } while (0)

#define NAPI_FCM_RETURN_IF(condition, msg, ret)              \
    do {                                                    \
        if ((condition)) {                                  \
            HILOGE(msg);                                    \
            return (ret);                                   \
        }                                                   \
    } while (0)

#define NAPI_FCM_ASSERT_RETURN(env, cond, errCode, retObj) \
do {                                                      \
    if (!(cond)) {                                        \
        HandleSyncErr((env), (errCode));                  \
        HILOGE("fusion connectivity ext napi assert failed.");                    \
        return (retObj);                                  \
    }                                               \
} while (0)

#define NAPI_FCM_ASSERT_RETURN_UNDEF(env, cond, errCode)  \
do {                                                     \
    napi_value res = nullptr;                            \
    napi_get_undefined((env), &res);                     \
    NAPI_FCM_ASSERT_RETURN((env), (cond), (errCode), res); \
} while (0)

struct ErrInfo {
    int32_t errCode;
    std::string errMsg;
};

// verify error code with num
#define NAPI_FCM_ASSERT_NUM_RETURN_VERIFY(env, cond, errCode, retObj)                     \
    do {                                                                                  \
        std::vector<int32_t> validErrCodes = apiContext.validErrCodes;                    \
        if (!(cond)) {                                                                    \
            FusionConnectivity::HandleSyncErrNumAdapter((env), (errCode), validErrCodes); \
            return (retObj);                                                              \
        }                                                                                 \
    } while (0)

#define NAPI_FCM_ASSERT_ERR_NUM_RETURN_VERIFY(env, cond, errCode)         \
    do {                                                                  \
        napi_value res = nullptr;                                         \
        napi_get_undefined((env), &res);                                  \
        NAPI_FCM_ASSERT_NUM_RETURN_VERIFY((env), (cond), (errCode), res); \
    } while (0)

struct ApiContext {
    std::shared_ptr<OHOS::FusionConnectivity::NapiHaEventUtils> haUtils = nullptr;
    std::vector<int32_t> validErrCodes{};
};

#ifndef NAPI_FCM_CONTEXT
#define NAPI_FCM_CONTEXT(env, apiName, validErrCodes)  \
ApiContext apiContext = ApiContext{                    \
    std::make_shared<NapiHaEventUtils>(env, apiName),  \
    validErrCodes                                      \
}
#endif

#ifndef CREATE_ASYNC_WORK_WITH_CONTEXT
#define CREATE_ASYNC_WORK_WITH_CONTEXT(env, info, func, needCallback) \
    FusionConnectivity::NapiAsyncWorkFactory::CreateAsyncWork(env, info, func, needCallback, apiContext)
#endif
namespace OHOS {
namespace FusionConnectivity {
void HandleSyncErr(napi_env env, int32_t errCode);
void HandleSyncErrAdapter(const napi_env &env, int32_t errCode, std::vector<int32_t> &validErrCodes);
void HandleSyncErrNumAdapter(const napi_env &env, int32_t errCode, std::vector<int32_t> &validErrCodes);
std::string GetNapiErrMsg(napi_env env, int32_t errCode);
ErrInfo ProcessErrCode(int32_t originalCode, const std::vector<int32_t> &validErrCodes);
}  // namespace FusionConnectivity
}  // namespace OHOS
#endif  // NAPI_FUSION_CONNECTIVITY_ERROR_H
