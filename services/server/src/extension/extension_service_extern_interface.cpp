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
#define LOG_TAG "ExtensionServiceExternInterface"
#endif

#include <cstdint>

#include "extension_service_common.h"
#include "extension_service_connection_service.h"
#include "fusion_conn_load_utils.h"

#define SYMBOL_EXPORT __attribute__ ((visibility("default")))
namespace OHOS {
namespace FusionConnectivity {
#ifdef __cplusplus
extern "C" {
#endif

SYMBOL_EXPORT int32_t Connect(const std::string& bundleName, const std::string& extensionName, const int32_t userId)
{
    std::shared_ptr<ExtensionSubscriberInfo> extensionSubscriberInfo =
        std::make_shared<ExtensionSubscriberInfo>(bundleName, extensionName, userId);
    return ExtensionServiceConnectionService::GetInstance()->Connect(extensionSubscriberInfo);
}

SYMBOL_EXPORT void OnDestroyWithReason(const std::string& bundleName,
    const std::string& extensionName, const int32_t userId, const int32_t reason)
{
    HILOGI("SYMBOL_EXPORT OnDestroyWithReason");
    std::shared_ptr<ExtensionSubscriberInfo> extensionSubscriberInfo =
        std::make_shared<ExtensionSubscriberInfo>(bundleName, extensionName, userId);
    ExtensionServiceConnectionService::GetInstance()->NotifyOnDestroyWithReason(extensionSubscriberInfo, reason);
}

SYMBOL_EXPORT void OnDeviceDiscovered(const std::string& bundleName, const std::string& extensionName,
    const int32_t userId, const PartnerDeviceAddress& deviceAddress, const NotificationType& type)
{
    HILOGI("SYMBOL_EXPORT OnDeviceDiscovered");
    std::shared_ptr<ExtensionSubscriberInfo> extensionSubscriberInfo =
        std::make_shared<ExtensionSubscriberInfo>(bundleName, extensionName, userId);
    ExtensionServiceConnectionService::GetInstance()->NotifyOnDeviceDiscovered(extensionSubscriberInfo,
        deviceAddress, type);
}
#ifdef __cplusplus
}
#endif
}  // namespace FusionConnectivity
}  // namespace OHOS
