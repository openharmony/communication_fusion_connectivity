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

#ifndef NEARLINK_RANGING_ADAPTER_H
#define NEARLINK_RANGING_ADAPTER_H

#include <string>
#include "base_ranging_adapter.h"

namespace OHOS {
namespace FusionRanging {

class NearlinkRangingAdapter : public BaseRangingAdapter {
public:
    NearlinkRangingAdapter();
    ~NearlinkRangingAdapter();

    int Init() override;
    int DeInit() override;
    int StartRanging(const std::string &deviceId) override;
    int StopRanging(const std::string &deviceId) override;
    int PauseRanging(const std::string &deviceId) override;
    int ResumeRanging(const std::string &deviceId) override;
    int StartPassiveRanging(int32_t &handle) override;
    int StopPassiveRanging(int32_t handle) override;
    int SetCallback(const std::shared_ptr<BaseRangingAdapterCallback> &callback) override;
};
}  // namespace FusionRanging
}  // namespace OHOS
#endif  // NEARLINK_RANGING_ADAPTER_H