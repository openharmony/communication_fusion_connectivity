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

#ifndef FUSION_RANGING_ERRORCODE_H
#define FUSION_RANGING_ERRORCODE_H

namespace OHOS {
namespace FusionRanging {

enum RangingErrCode {
    RANGING_NO_ERROR = 0,
    RANGING_ERR_PERMISSION_FAILED = 201,
    RANGING_ERR_INVALID_PARAM = 401,
    RANGING_ERR_API_NOT_SUPPORT = 801,
    RANGING_ERR_OBJECT_NOT_FOUND = 34900050,
    RANGING_ERR_OBJECT_ALREADY_EXIST = 34900051,
    RANGING_ERR_ADAPTER_NOT_SUPPORT = 34900052,
    RANGING_ERR_SWITCH_IS_OFF = 34900053,
    RANGING_ERR_SERVICE_NOT_PROVIDED = 34900054,
    RANGING_ERR_PARAM_IS_OCCUPIED = 34900055,
    RANGING_ERR_OPERATION_NOT_ALLOW = 34900056,
    RANGING_ERR_INTERNAL_ERROR = 34900099,
};
}  // namespace FusionRanging
}  // namespace OHOS
#endif  // FUSION_RANGING_ERRORCODE_H