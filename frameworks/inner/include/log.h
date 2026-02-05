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

#ifndef LOG_H
#define LOG_H

#include <stdint.h>         // for uint8_t
#include <string>           // for basic_string

#include "hilog/log.h"

namespace OHOS {
namespace FusionConnectivity {
#ifndef LOG_DOMAIN
#define LOG_DOMAIN 0xD000100
#endif  // LOG_DOMAIN

#ifndef LOG_TAG
#define LOG_TAG "FusionConnectivity"
#endif

#define HILOGF(fmt, ...)            \
    ((void)HILOG_IMPL(LOG_CORE, LOG_FATAL, LOG_DOMAIN, LOG_TAG, \
    fmt, ##__VA_ARGS__))
#define HILOGE(fmt, ...)            \
    ((void)HILOG_IMPL(LOG_CORE, LOG_ERROR, LOG_DOMAIN, LOG_TAG, \
    fmt, ##__VA_ARGS__))
#define HILOGW(fmt, ...)            \
    ((void)HILOG_IMPL(LOG_CORE, LOG_WARN, LOG_DOMAIN, LOG_TAG, \
    fmt, ##__VA_ARGS__))
#define HILOGI(fmt, ...)            \
    ((void)HILOG_IMPL(LOG_CORE, LOG_INFO, LOG_DOMAIN, LOG_TAG, \
    fmt, ##__VA_ARGS__))
#define HILOGD(fmt, ...)            \
    ((void)HILOG_IMPL(LOG_CORE, LOG_DEBUG, LOG_DOMAIN, LOG_TAG, \
    "[(%{public}s:%{public}d)]" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__))

#ifdef FCM_CHECK_RETURN
#undef FCM_CHECK_RETURN
#endif

#define FCM_CHECK_RETURN(cond, fmt, ...)            \
    do {                                            \
        if (!(cond)) {                              \
            HILOGE(fmt, ##__VA_ARGS__);             \
            return;                                 \
        }                                           \
    } while (0)

#ifdef FCM_CHECK_RETURN_RET
#undef FCM_CHECK_RETURN_RET
#endif

#define FCM_CHECK_RETURN_RET(cond, ret, fmt, ...)                   \
    do {                                                            \
        if (!(cond)) {                                              \
            HILOGE(fmt, ##__VA_ARGS__);                             \
            return ret;                                             \
        }                                                           \
    } while (0)

}  // namespace FusionConnectivity
}  // namespace OHOS
#endif  // LOG_H
