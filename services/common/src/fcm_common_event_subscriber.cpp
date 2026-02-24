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
#define LOG_TAG "FcmCommonEvent"
#endif

#include "fcm_common_event_subscriber.h"
#include <algorithm>
#include "log.h"

namespace OHOS {
namespace FusionConnectivity {

void FcmCommonEventSubscriber::OnReceiveEvent(const OHOS::EventFwk::CommonEventData &data)
{
    auto want = data.GetWant();
    std::string action = want.GetAction();
    if (std::find(actionVec_.begin(), actionVec_.end(), action) != actionVec_.end()) {
        HILOGI("Fcm common event %{public}s", action.c_str());
        callback_(data);
    }
}

}  // namespace FusionConnectivity
}  // namespace OHOS