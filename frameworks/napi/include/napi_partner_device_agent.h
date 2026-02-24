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

#ifndef NAPI_PARTNER_DEVICE_AGENT_H
#define NAPI_PARTNER_DEVICE_AGENT_H

#include "napi_parser_utils.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace OHOS {
namespace FusionConnectivity {

void DefinePartnerDeviceAgentProperty(napi_env env, napi_value exports);
void DefinePartnerDeviceAgentInterface(napi_env env, napi_value exports);

napi_value IsPartnerAgentSupported(napi_env env, napi_callback_info info);
napi_value BindDevice(napi_env env, napi_callback_info info);
napi_value UnbindDevice(napi_env env, napi_callback_info info);
napi_value IsDeviceBound(napi_env env, napi_callback_info info);
napi_value GetBoundDevices(napi_env env, napi_callback_info info);
napi_value EnableDeviceControl(napi_env env, napi_callback_info info);
napi_value DisableDeviceControl(napi_env env, napi_callback_info info);
napi_value IsDeviceControlEnabled(napi_env env, napi_callback_info info);
}  // namespace FusionConnectivity
}  // namespace OHOS
#endif  // NAPI_PARTNER_DEVICE_AGENT_H
