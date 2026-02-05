/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#define LOG_TAG "JsPartnerAgentExtension"
#endif

#include "js_partner_agent_extension.h"
#include <memory>

#include "ability_info.h"
#include "ability_handler.h"
#include "js_runtime.h"
#include "js_runtime_utils.h"
#include "js_partner_agent_extension_context.h"
#include "napi_common_want.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "napi_remote_object.h"
#include "log.h"
#include "partner_agent_extension_stub_impl.h"
#include "napi_parser_utils.h"

namespace OHOS {
namespace FusionConnectivity {
namespace {
constexpr size_t ARGC_ONE = 1;
}

using namespace OHOS::AppExecFwk;

napi_value AttachPartnerAgentExtensionContext(napi_env env, void* value, void*)
{
    HILOGI("AttachPartnerAgentExtensionContext");
    if (value == nullptr) {
        HILOGE("invalid parameter.");
        return nullptr;
    }

    auto ptr = reinterpret_cast<std::weak_ptr<PartnerAgentExtensionContext>*>(value)->lock();
    if (ptr == nullptr) {
        HILOGE("invalid context.");
        return nullptr;
    }

    napi_value object = CreateJsPartnerAgentExtensionContext(env, ptr);
    auto napiContextObj = AbilityRuntime::JsRuntime::LoadSystemModuleByEngine(env,
        "FusionConnectivity.PartnerAgentExtensionContext", &object, 1)->GetNapiValue();
    if (napiContextObj == nullptr) {
        HILOGE("load context failed.");
        return nullptr;
    }
    napi_coerce_to_native_binding_object(env, napiContextObj, AbilityRuntime::DetachCallbackFunc,
        AttachPartnerAgentExtensionContext, value, nullptr);
    auto workContext = new (std::nothrow) std::weak_ptr<PartnerAgentExtensionContext>(ptr);
    if (workContext == nullptr) {
        HILOGE("invalid PartnerAgentExtensionContext.");
        return nullptr;
    }
    napi_wrap(env, napiContextObj, workContext,
        [](napi_env, void* data, void*) {
            HILOGI("Finalizer for weak_ptr partner agent extension context is called");
            delete static_cast<std::weak_ptr<PartnerAgentExtensionContext>*>(data);
        }, nullptr, nullptr);
    return napiContextObj;
}

JsPartnerAgentExtension* JsPartnerAgentExtension::Create(
    const std::unique_ptr<AbilityRuntime::Runtime>& runtime)
{
    HILOGI("Js partner agent extension create.");
    return new (std::nothrow) JsPartnerAgentExtension(static_cast<AbilityRuntime::JsRuntime&>(*runtime));
}

JsPartnerAgentExtension::JsPartnerAgentExtension(AbilityRuntime::JsRuntime& jsRuntime)
    : jsRuntime_(jsRuntime) {}
JsPartnerAgentExtension::~JsPartnerAgentExtension()
{
    HILOGI("Js partner agent extension destructor.");
    auto context = GetContext();
    if (context) {
        context->Unbind();
    }

    jsRuntime_.FreeNativeReference(std::move(jsObj_));
}

void JsPartnerAgentExtension::Init(const std::shared_ptr<AppExecFwk::AbilityLocalRecord>& record,
    const std::shared_ptr<AppExecFwk::OHOSApplication>& application,
    std::shared_ptr<AppExecFwk::AbilityHandler>& handler,
    const sptr<IRemoteObject>& token)
{
    PartnerAgentExtension::Init(record, application, handler, token);
    if (Extension::abilityInfo_ == nullptr || Extension::abilityInfo_->srcEntrance.empty()) {
        HILOGE("srcEntrance of abilityInfo is empty");
        return;
    }
    std::string srcPath(Extension::abilityInfo_->moduleName + "/");
    srcPath.append(Extension::abilityInfo_->srcEntrance);
    srcPath.erase(srcPath.rfind('.'));
    srcPath.append(".abc");

    std::string moduleName(Extension::abilityInfo_->moduleName);
    moduleName.append("::").append(abilityInfo_->name);
    HILOGI("moduleName: %{public}s.", moduleName.c_str());
    AbilityRuntime::HandleScope handleScope(jsRuntime_);
    napi_env env = jsRuntime_.GetNapiEnv();

    jsObj_ = jsRuntime_.LoadModule(moduleName, srcPath, abilityInfo_->hapPath,
        abilityInfo_->compileMode == CompileMode::ES_MODULE);
    if (jsObj_ == nullptr) {
        HILOGE("Failed to load module");
        return;
    }
    napi_value obj = jsObj_->GetNapiValue();
    if (obj == nullptr) {
        HILOGE("Failed to get partner agent extension object");
        return;
    }
    ExecNapiWrap(env, obj);
}

void JsPartnerAgentExtension::ExecNapiWrap(napi_env env, napi_value obj)
{
    auto context = GetContext();
    if (context == nullptr) {
        HILOGE("Failed to get context");
        return;
    }

    napi_value contextObj = CreateJsPartnerAgentExtensionContext(env, context);
    auto shellContextRef = AbilityRuntime::JsRuntime::LoadSystemModuleByEngine(
        env, "FusionConnectivity.PartnerAgentExtensionContext", &contextObj, ARGC_ONE);
    if (shellContextRef == nullptr) {
        HILOGE("Failed to get shell context reference");
        return;
    }
    napi_value nativeObj = shellContextRef->GetNapiValue();
    if (nativeObj == nullptr) {
        HILOGE("Failed to get context native object");
        return;
    }

    auto workContext = new (std::nothrow) std::weak_ptr<PartnerAgentExtensionContext>(context);
    if (workContext == nullptr) {
        HILOGE("invalid PartnerAgentExtensionContext.");
        return;
    }
    napi_coerce_to_native_binding_object(env, nativeObj, AbilityRuntime::DetachCallbackFunc,
        AttachPartnerAgentExtensionContext, workContext, nullptr);
    context->Bind(jsRuntime_, shellContextRef.release());
    napi_set_named_property(env, obj, "context", nativeObj);

    HILOGD("Set partner agent extension context");
    napi_wrap(env, nativeObj, workContext,
        [](napi_env, void* data, void*) {
        HILOGI("Finalizer for weak_ptr partner agent extension context is called");
        delete static_cast<std::weak_ptr<PartnerAgentExtensionContext>*>(data);
        }, nullptr, nullptr);

    HILOGI("Init end.");
}

void JsPartnerAgentExtension::OnStart(const AAFwk::Want& want)
{
    HILOGI("called");
    Extension::OnStart(want);
}

void JsPartnerAgentExtension::OnStop()
{
    HILOGI("called");
    OnDestroyWithReason(1);
    Extension::OnStop();
}

sptr<IRemoteObject> JsPartnerAgentExtension::OnConnect(const AAFwk::Want& want)
{
    HILOGI("OnConnect called");
    Extension::OnConnect(want);
    sptr<PartnerAgentExtensionStubImpl> remoteObject = new (std::nothrow) PartnerAgentExtensionStubImpl(
        std::static_pointer_cast<JsPartnerAgentExtension>(shared_from_this()));
    if (remoteObject == nullptr) {
        HILOGE("failed to create PartnerAgentStubImpl!");
        return nullptr;
    }
    return remoteObject->AsObject();
}

void JsPartnerAgentExtension::OnDisconnect(const AAFwk::Want& want)
{
    HILOGI("called");
    Extension::OnDisconnect(want);
}

std::weak_ptr<JsPartnerAgentExtension> JsPartnerAgentExtension::GetWeakPtr()
{
    return std::static_pointer_cast<JsPartnerAgentExtension>(shared_from_this());
}

void NapiCallFunctionWithMethod(napi_env env, napi_value obj, napi_value method, size_t argc, napi_value *argv)
{
    auto status = napi_call_function(env, obj, method, argc, argv, nullptr);
    if (status != napi_ok) {
        HILOGE("napi_call_function failed, status: %{public}d", status);
    }

    // Check whether the JS application triggers an exception in callback. If it is, clear it.
    bool isExist = false;
    status = napi_is_exception_pending(env, &isExist);
    HILOGD("napi_is_exception_pending status: %{public}d, isExist: %{public}d", status, isExist);
    if (isExist) {
        HILOGI("Clear JS application's exception");
        napi_value exception = nullptr;
        status = napi_get_and_clear_last_exception(env, &exception);
        HILOGD("napi_get_and_clear_last_exception status: %{public}d", status);
    }
}

napi_value CreatePartnerAgentExtensionAbilityDestroyReason(napi_env env, const int32_t reason)
{
    HILOGI("enter reason is %{public}d", reason);
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_int32(env, reason, &result));
    return result;
}

napi_value CreatePartnerDeviceAddress(napi_env env, const PartnerDeviceAddress& deviceAddress)
{
    napi_value nPartnerDeviceAddress = nullptr;
    NAPI_CALL(env, napi_create_object(env, &nPartnerDeviceAddress));

    napi_value napiBluetoothAddress = nullptr;
    NAPI_CALL(env, napi_create_object(env, &napiBluetoothAddress));

    napi_value nDeviceAddress = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, deviceAddress.GetAddress().c_str(),
        NAPI_AUTO_LENGTH, &nDeviceAddress));
    NAPI_CALL(env, napi_set_named_property(env, napiBluetoothAddress, "address", nDeviceAddress));

    napi_value nDeviceAddressType = nullptr;
    NAPI_CALL(env, napi_create_int32(env, static_cast<int32_t>(deviceAddress.GetAddressType()), &nDeviceAddressType));
    NAPI_CALL(env, napi_set_named_property(env, napiBluetoothAddress, "addressType", nDeviceAddressType));

    if (deviceAddress.HasRawAddressType()) {
        napi_value nRawAddressType = nullptr;
        NAPI_CALL(env, napi_create_int32(env,
            static_cast<int32_t>(deviceAddress.GetRawAddressType()), &nRawAddressType));
        NAPI_CALL(env, napi_set_named_property(env, napiBluetoothAddress, "rawAddressType", nRawAddressType));
    }

    NAPI_CALL(env, napi_set_named_property(env, nPartnerDeviceAddress, "bluetoothAddress", napiBluetoothAddress));
    return nPartnerDeviceAddress;
}


PartnerAgentExtensionResult JsPartnerAgentExtension::OnDestroyWithReason(const int32_t reason)
{
    HILOGI("enter OnDestroyWithReason is %{public}d", reason);

    if (handler_ == nullptr) {
        HILOGE("handler is invalid");
        return PartnerAgentExtensionResult::INTERNAL_ERROR;
    }

    std::weak_ptr<JsPartnerAgentExtension> wThis = GetWeakPtr();
    auto task = [wThis, reason]() {
        std::shared_ptr<JsPartnerAgentExtension> sThis = wThis.lock();
        if (sThis == nullptr) {
            HILOGE("null sThis");
            return;
        }
        if (!sThis->jsObj_) {
            HILOGE("Not found PartnerAgentExtension.js");
            return;
        }
        AbilityRuntime::HandleScope handleScope(sThis->jsRuntime_);
        napi_env env = sThis->jsRuntime_.GetNapiEnv();

        napi_value napiResult = CreatePartnerAgentExtensionAbilityDestroyReason(env, reason);
        napi_value argv[] = {napiResult};
        napi_value obj = sThis->jsObj_->GetNapiValue();
        if (obj == nullptr) {
            HILOGE("Failed to get PartnerAgentExtension object");
            return;
        }

        napi_value method = nullptr;
        napi_get_named_property(env, obj, "onDestroyWithReason", &method);
        if (method == nullptr) {
            HILOGE("Failed to get onDestroyWithReason from PartnerAgentExtension object");
            return;
        }
        NapiCallFunctionWithMethod(env, obj, method, ARGC_ONE, argv);
    };
    handler_->PostTask(task, "OnDestroyWithReason");
    return PartnerAgentExtensionResult::OK;
}


PartnerAgentExtensionResult JsPartnerAgentExtension::OnDeviceDiscovered(const PartnerDeviceAddress& deviceAddress)
{
    HILOGI("OnDeviceDiscovered");
    if (deviceAddress.GetAddress().empty()) {
        HILOGE("deviceAddress is invalid");
        return PartnerAgentExtensionResult::INVALID_PARAM;
    }
    if (handler_ == nullptr) {
        HILOGE("handler is invalid");
        return PartnerAgentExtensionResult::INTERNAL_ERROR;
    }
    std::weak_ptr<JsPartnerAgentExtension> wThis = GetWeakPtr();
    auto task = [wThis, deviceAddress]() {
        std::shared_ptr<JsPartnerAgentExtension> sThis = wThis.lock();
        if (sThis == nullptr) {
            HILOGE("null sThis");
            return;
        }
        if (!sThis->jsObj_) {
            HILOGE("Not found NotificationSubscriberExtension.js");
            return;
        }

        AbilityRuntime::HandleScope handleScope(sThis->jsRuntime_);
        napi_env env = sThis->jsRuntime_.GetNapiEnv();
        napi_value napiResult = CreatePartnerDeviceAddress(env, deviceAddress);
        napi_value argv[] = {napiResult};
        napi_value obj = sThis->jsObj_->GetNapiValue();
        if (obj == nullptr) {
            HILOGE("Failed to get NotificationSubscriberExtension object");
            return;
        }

        napi_value method = nullptr;
        napi_get_named_property(env, obj, "onDeviceDiscovered", &method);
        if (method == nullptr) {
            HILOGE("Failed to get onDeviceDiscovered from NotificationSubscriberExtension object");
            return;
        }
        NapiCallFunctionWithMethod(env, obj, method, ARGC_ONE, argv);
    };
    handler_->PostTask(task, "onDeviceDiscovered");
    return PartnerAgentExtensionResult::OK;
}
}  // namespace FusionConnectivity
}  // namespace OHOS