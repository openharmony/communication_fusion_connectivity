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

#ifndef FUSION_RANGING_TYPES_H
#define FUSION_RANGING_TYPES_H

#include <cstdint>

namespace OHOS {
namespace FusionRanging {

enum class RangingTypes {
    NEARLINK_HADM = 1,
    RANGING_TYPE_MAX,
};

enum class RangingState {
    STATE_STOPPED = 0,
    STATE_STARTED = 1,
};

enum class RangingStoppedCause {
    NO_ERROR = 0,
    INTERNAL_ERROR = 1,
    BUSINESS_CONFLICT = 2,
    LIMITED_RESOURCE = 3,
    BACKGROUND_PAUSED = 4,
    LINK_DISCONNECT = 5,
};

enum class RangingConfidence {
    HIGH = 0,
    MEDIUM = 1,
    LOW = 2,
};
}  // namespace FusionRanging
}  // namespace OHOS
#endif  // FUSION_RANGING_TYPES_H