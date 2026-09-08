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

#ifndef FCM_ERRORCODE_H
#define FCM_ERRORCODE_H

namespace OHOS {
namespace FusionConnectivity {

/* FusionConnectivity errcode defines */
enum FcmErrCode {
    // Common error codes
    FCM_ERR_PERMISSION_FAILED = 201,
    FCM_ERR_SYSTEM_PERMISSION_FAILED = 202,
    FCM_ERR_PROHIBITED_BY_EDM = 203,
    FCM_ERR_INVALID_PARAM = 401,
    FCM_ERR_API_NOT_SUPPORT = 801,

    // Customized error codes
    FCM_NO_ERROR = 0,

    FCM_ERR_DEVICE_NOT_FOUND = 34900001,
    FCM_ERR_APPLICATION_NOT_SUPPORT = 34900002,
    FCM_ERR_DEVICE_NOT_PAIRED = 34900003,
    FCM_ERR_DEVICE_ALREADY_BOUNDED = 34900004,
    FCM_ERR_BLUETOOTH_IS_OFF = 34900005,
    FCM_ERR_INTERNAL_ERROR = 34900099,

    // Inner error codes
    FCM_ERR_ASYNC_WORK_COMPLETE = -1,

    // Inner error codes for detailed error mapping
    FCM_ERR_INNER_BASE = -1000,
    // partner_device_config.cpp parse errors
    FCM_ERR_CONFIG_INVALID_ADDRESS_INFO = FCM_ERR_INNER_BASE - 1,
    FCM_ERR_CONFIG_INVALID_ADDRESS = FCM_ERR_INNER_BASE - 2,
    FCM_ERR_CONFIG_INVALID_ADDRESS_TYPE = FCM_ERR_INNER_BASE - 3,
    FCM_ERR_CONFIG_INVALID_RAW_ADDRESS_TYPE = FCM_ERR_INNER_BASE - 4,
    FCM_ERR_CONFIG_INVALID_CAPABILITY = FCM_ERR_INNER_BASE - 5,
    FCM_ERR_CONFIG_INVALID_BUSINESS_CAPABILITY = FCM_ERR_INNER_BASE - 6,
    FCM_ERR_CONFIG_DEVICE_EXPIRED = FCM_ERR_INNER_BASE - 7,
    FCM_ERR_CONFIG_INVALID_JSON = FCM_ERR_INNER_BASE - 8,
    FCM_ERR_CONFIG_INVALID_TOKEN_ID = FCM_ERR_INNER_BASE - 9,

    // server & inner framework errors
    FCM_ERR_EXTENSION_TYPE_NOT_SUPPORT = FCM_ERR_INNER_BASE - 10,
    FCM_ERR_PROXY_IS_NULL = FCM_ERR_INNER_BASE - 11,
};

}  // namespace FusionConnectivity
}  // namespace OHOS

#endif  // FCM_ERRORCODE_H
