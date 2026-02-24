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

#ifndef EXTENSION_SERVICE_COMMON_H
#define EXTENSION_SERVICE_COMMON_H

#include <string>
#include "log.h"

namespace OHOS {
namespace FusionConnectivity {
enum class ExtensionServiceConnectionState {
    CREATED,
    CONNECTING,
    CONNECTED,
    DISCONNECTED
};

struct ExtensionSubscriberInfo {
    std::string bundleName;
    std::string extensionName;
    int32_t userId = -1;

    ExtensionSubscriberInfo(const std::string &bundleName, const std::string &extensionName, const int32_t userId) {
        this->bundleName = bundleName;
        this->extensionName = extensionName;
        this->userId = userId;
    }

    std::string GetKey() const
    {
        return bundleName + "_" + extensionName + "_" + std::to_string(userId);
    }
};
}
}
#endif // EXTENSION_SERVICE_COMMON_H
