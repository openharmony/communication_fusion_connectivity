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

#ifndef NEARLINK_RANGING_CALLBACK_H
#define NEARLINK_RANGING_CALLBACK_H

#include <string>
#include "nearlink_sle_ranging.h"

namespace OHOS {
namespace FusionRanging {

class NearlinkRangingCallback
    : public std::enable_shared_from_this<NearlinkRangingCallback>, public Nearlink::SleRangingCallback {
public:
    NearlinkRangingCallback();
    ~NearlinkRangingCallback() override;

    NearlinkRangingCallback(const NearlinkRangingCallback &) = delete;
    NearlinkRangingCallback &operator=(const NearlinkRangingCallback &) = delete;

    void OnSleRangingResult(const Nearlink::RangingResult &result) override;
    void OnSleRangingStateChange(const Nearlink::RangingState &state) override;
};
}  // namespace FusionRanging
}  // namespace OHOS
#endif  // NEARLINK_RANGING_CALLBACK_H