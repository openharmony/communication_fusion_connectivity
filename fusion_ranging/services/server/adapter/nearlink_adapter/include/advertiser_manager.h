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

#ifndef ADVERTISER_MANAGER_H
#define ADVERTISER_MANAGER_H

#include <string>
#include "base_def.h"

namespace OHOS {
namespace FusionRanging {

class AdvertiserManager {
public:
    static AdvertiserManager *GetInstance();

    int StartPassiveRanging(int32_t &advHandle);
    int StopPassiveRanging(int32_t handle);

    void OnSsapConnectionStateUpdate(const std::string &deviceId, int32_t advHandle, int state, int reason);
    void OnAdvStateChanged(int advHandle, int state);

private:
    AdvertiserManager();
    ~AdvertiserManager();
    bool IsPassiveRangingActive();
    int StopAdvertising(int32_t advHandle);
    int StopSsapServer(int32_t advHandle);
    int StartAdvertisingInternal(int32_t &advHandle);
    int StartSsapServerInternal(int32_t advHandle);

    DECLARE_IMPL();
};
}  // namespace FusionRanging
}  // namespace OHOS
#endif  // ADVERTISER_MANAGER_H