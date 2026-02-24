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

#ifndef NOTIFICATION_DYNAMIC_LOOAD_UTILS_H
#define NOTIFICATION_DYNAMIC_LOOAD_UTILS_H

#include <string>

namespace OHOS {
namespace FusionConnectivity {

static const int32_t INVALID_NOTIFICATION_ID = 0;
enum class NotificationType {
    TELEPHONY_CONTROL_ONLY = 1,
    MEDIA_CONTROL_ONLY,
    MEDIA_AND_TELEPHONY_CONTROL
};

enum class DeviceMode {
    NEARLINK_MODE = 0,
    BLUETOOTH_MODE = 1
};

class FusionConnectivityLoadUtils {
public:
    explicit FusionConnectivityLoadUtils(const std::string& path);
    ~FusionConnectivityLoadUtils();
    bool IsValid();
    void* GetProxyFunc(const std::string& funcName);
private:
    std::string path_;
    void* proxyHandle_ = nullptr;
};

}
}
#endif // NOTIFICATION_DYNAMIC_LOOAD_UTILS_H
