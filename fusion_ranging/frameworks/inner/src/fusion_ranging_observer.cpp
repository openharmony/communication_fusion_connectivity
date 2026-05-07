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

#include "fusion_ranging_observer.h"
#include "log_utils.h"

#ifndef LOG_TAG
#define LOG_TAG "FusionRangingObserver"
#endif

namespace OHOS {
namespace FusionRanging {

int32_t RangingObserverImpl::OnRangingStateChanged(const RangingStateChangeInfo &info)
{
    HILOGI("RangingObserverImpl::OnRangingStateChanged state: %{public}d, cause: %{public}d",
           static_cast<int32_t>(info.GetState()), static_cast<int32_t>(info.GetCause()));
    auto handlerLocal = handler_.promote();
    if (handlerLocal != nullptr) {
        handlerLocal->OnRangingStateChanged(info);
    }
    return ERR_NONE;
}

int32_t RangingObserverImpl::OnRangingResult(const RangingResult &result)
{
    auto handlerLocal = handler_.promote();
    if (handlerLocal != nullptr) {
        handlerLocal->OnRangingResult(result);
    }
    return ERR_NONE;
}
}  // namespace FusionRanging
}  // namespace OHOS