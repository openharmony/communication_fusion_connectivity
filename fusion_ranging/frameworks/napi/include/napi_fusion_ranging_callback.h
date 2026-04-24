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
#ifndef NAPI_FUSION_RANGING_CALLBACK_H
#define NAPI_FUSION_RANGING_CALLBACK_H

#include <shared_mutex>
#include "fusion_ranging_manager.h"

namespace OHOS {
namespace FusionRanging {

const char * const STR_FUSION_RANGING_CALLBACK_STATE_CHANGE = "FusionRangingStateChange";
const char * const STR_FUSION_RANGING_CALLBACK_RESULT = "FusionRangingResult";
class NapiGattClient;

class NapiFusionRangingCallback : public FusionRangingObserver {
public:
    void OnRangingStateChanged(const RangingStateChangeInfo &info) override;
    void OnRangingResult(const RangingResult &result) override;

    NapiFusionRangingCallback();
    ~NapiFusionRangingCallback() override = default;

    NapiAsyncWorkMap asyncWorkMap_ {};
private:
    friend class NapiGattClient;
    NapiEventSubscribeModule eventSubscribe_;

    std::string deviceAddr_ = INVALID_MAC_ADDRESS;
};
}  // namespace Bluetooth
}  // namespace OHOS
#endif /* NAPI_FUSION_RANGING_CALLBACK_H */