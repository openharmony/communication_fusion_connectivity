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

#ifndef FCM_COMMON_EVENT_SUBSCRIDER_H
#define FCM_COMMON_EVENT_SUBSCRIDER_H

#include <string>
#include <vector>

#include "common_event_manager.h"
#include "common_event_subscriber.h"
#include "common_event_support.h"

namespace OHOS {
namespace FusionConnectivity {

class FcmCommonEventSubscriber : public OHOS::EventFwk::CommonEventSubscriber {
public:
    using CommonEventCallback = std::function<void(const OHOS::EventFwk::CommonEventData &data)>;

    FcmCommonEventSubscriber(const OHOS::EventFwk::CommonEventSubscribeInfo &subscriberInfo,
        const std::string &action, CommonEventCallback callback)
        : OHOS::EventFwk::CommonEventSubscriber(subscriberInfo), actionVec_({action}), callback_(callback) {}
    FcmCommonEventSubscriber(const OHOS::EventFwk::CommonEventSubscribeInfo &subscriberInfo,
        const std::vector<std::string> &actionVec, CommonEventCallback callback)
        : OHOS::EventFwk::CommonEventSubscriber(subscriberInfo), actionVec_({actionVec}), callback_(callback) {}
    ~FcmCommonEventSubscriber() {}

    void OnReceiveEvent(const OHOS::EventFwk::CommonEventData &data) override;

private:
    std::vector<std::string> actionVec_;
    CommonEventCallback callback_;
};
} // namespace FusionConnectivity
} // namespace OHOS

#endif // FCM_COMMON_EVENT_SUBSCRIDER_H