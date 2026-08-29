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

#include "ets_partner_agent_extension.h"
#include "log.h"
#include "ets_runtime.h"
#include "ets_extension_context.h"
#include "ets_partner_agent_extension_context.h"
#include "partner_agent_extension_stub_impl.h"
#include "partner_agent_extension.h"
#include "ani.h"
#include "ability_handler.h"

#include <utility>

namespace OHOS {
namespace FusionConnectivity {
namespace {
constexpr ani_size ANI_LOCAL_SCOPE_CAPACITY = 16;

class AniLocalScopeGuard {
public:
    explicit AniLocalScopeGuard(ani_env* env) : env_(env)
    {
        if (env_ == nullptr) {
            return;
        }
        ani_status status = env_->CreateLocalScope(ANI_LOCAL_SCOPE_CAPACITY);
        if (status != ANI_OK) {
            HILOGE("CreateLocalScope failed, status : %{public}d", status);
            env_ = nullptr;
        }
    }

    ~AniLocalScopeGuard()
    {
        if (env_ == nullptr) {
            return;
        }
        ani_status status = env_->DestroyLocalScope();
        if (status != ANI_OK) {
            HILOGE("DestroyLocalScope failed, status : %{public}d", status);
        }
    }

    AniLocalScopeGuard(const AniLocalScopeGuard&) = delete;
    AniLocalScopeGuard& operator=(const AniLocalScopeGuard&) = delete;

private:
    ani_env* env_;
};
} // namespace

extern "C" __attribute__((visibility("default"))) PartnerAgentExtension* OHOS_STS_PartnerAgentExtension_Creation(
    const std::unique_ptr<AbilityRuntime::Runtime>& runtime)
{
    if (runtime == nullptr) {
        HILOGE("runtime is null");
        return nullptr;
    }
    return new (std::nothrow) ETSPartnerAgentExtension(static_cast<AbilityRuntime::ETSRuntime &>(*runtime));
}

ETSPartnerAgentExtension::ETSPartnerAgentExtension(AbilityRuntime::ETSRuntime& etsRuntime)
    : etsRuntime_(etsRuntime)
{}

ETSPartnerAgentExtension::~ETSPartnerAgentExtension()
{
    HILOGI("Ets partner agent extension destructor.");
    auto context = GetContext();
    if (context) {
        context->Unbind();
    }

    etsAbilityObj_.reset();
}

void ETSPartnerAgentExtension::Init(const std::shared_ptr<AppExecFwk::AbilityLocalRecord>& record,
    const std::shared_ptr<AppExecFwk::OHOSApplication>& application,
    std::shared_ptr<AppExecFwk::AbilityHandler>& handler,
    const sptr<IRemoteObject>& token)
{
    HILOGI("Init call");
    PartnerAgentExtension::Init(record, application, handler, token);
    if (Extension::abilityInfo_ == nullptr || Extension::abilityInfo_->srcEntrance.empty()) {
        HILOGE("srcEntrance of abilityInfo is empty");
        return;
    }
    auto abilityInfo = record->GetAbilityInfo();
    if (abilityInfo == nullptr) {
        HILOGE("abilityInfo is null");
        return;
    }
    std::string srcPath;
    GetSrcPath(srcPath);
    std::string moduleName(Extension::abilityInfo_->moduleName);
    moduleName.append("::").append(abilityInfo_->name);
    HILOGI("moduleName:%{public}s,srcPath:%{public}s, compileMode :%{public}d",
        moduleName.c_str(), srcPath.c_str(), abilityInfo_->compileMode);
    BindContext(abilityInfo, record->GetWant(), moduleName, srcPath);
    HILOGI("Init End");
}

void ETSPartnerAgentExtension::GetSrcPath(std::string& srcPath)
{
    srcPath.append(Extension::abilityInfo_->moduleName + "/");
    srcPath.append(Extension::abilityInfo_->srcEntrance);
    auto pos = srcPath.rfind('.');
    if (pos != std::string::npos) {
        srcPath.erase(pos);
        srcPath.append(".abc");
    }
}

void ETSPartnerAgentExtension::UpdatePartnerAgentExtensionObj(
    std::shared_ptr<AbilityRuntime::AbilityInfo> &abilityInfo,
    const std::string &moduleName, const std::string &srcPath)
{
    HILOGI("UpdatePartnerAgentExtensionObj call");
    etsAbilityObj_ = etsRuntime_.LoadModule(moduleName, srcPath, abilityInfo->hapPath,
        abilityInfo->compileMode == AppExecFwk::CompileMode::ES_MODULE, false, abilityInfo_->srcEntrance);
    if (etsAbilityObj_ == nullptr) {
        HILOGI("null etsAbilityObj_");
        return;
    }
    HILOGI("UpdatePartnerAgentExtensionObj End");
}

void ETSPartnerAgentExtension::BindContext(std::shared_ptr<AbilityRuntime::AbilityInfo> &abilityInfo,
    std::shared_ptr<AAFwk::Want> want, const std::string &moduleName, const std::string &srcPath)
{
    HILOGI("BindContext call");
    UpdatePartnerAgentExtensionObj(abilityInfo, moduleName, srcPath);

    auto env = etsRuntime_.GetAniEnv();
    if (env == nullptr) {
        HILOGE("env is null");
        return;
    }
    AniLocalScopeGuard scopeGuard(env);
    if (etsAbilityObj_ == nullptr || want == nullptr) {
        HILOGE("etsAbilityObj_ or abilityContext_ is null");
        return;
    }

    auto context = GetContext();
    if (context == nullptr) {
        HILOGE("get context error");
        return;
    }
    ani_ref contextObj = CreateEtsPartnerAgentExtensionContext(env, context);
    if (contextObj == nullptr) {
        HILOGE("Create context obj error");
        return;
    }

    ani_ref contextGlobalRef = nullptr;
    ani_field field = nullptr;
    ani_status status = ANI_ERROR;

    if ((status = env->GlobalReference_Create(contextObj, &contextGlobalRef)) != ANI_OK) {
        HILOGE("GlobalReference_Create failed, status : %{public}d", status);
        return;
    }
    if ((status = env->Class_FindField(etsAbilityObj_->aniCls, "context", &field)) != ANI_OK) {
        env->GlobalReference_Delete(contextGlobalRef);
        HILOGE("Class_FindField failed, status : %{public}d", status);
        return;
    }

    if ((status = env->Object_SetField_Ref(etsAbilityObj_->aniObj, field, contextGlobalRef)) != ANI_OK) {
        env->GlobalReference_Delete(contextGlobalRef);
        HILOGE("Object_SetField failed, status : %{public}d", status);
        return;
    }
    HILOGI("BindContext End");
}


void ETSPartnerAgentExtension::OnStart(const AAFwk::Want& want)
{
    HILOGI("OnStart");
    if (etsAbilityObj_ == nullptr) {
        HILOGE("etsAbilityObj_ is null");
        return;
    }

    auto env = etsRuntime_.GetAniEnv();
    if (env == nullptr) {
        HILOGE("env is null");
        return;
    }
    Extension::OnStart(want);
}

sptr<IRemoteObject> ETSPartnerAgentExtension::OnConnect(const AAFwk::Want& want)
{
    HILOGI("OnConnect");
    Extension::OnConnect(want);
    sptr<PartnerAgentExtensionStubImpl> remoteObject = new (std::nothrow) PartnerAgentExtensionStubImpl(
        std::static_pointer_cast<ETSPartnerAgentExtension>(shared_from_this()));
    if (remoteObject == nullptr) {
        HILOGE("failed to create PartnerAgentStubImpl!");
        return nullptr;
    }
    return remoteObject->AsObject();
}

void ETSPartnerAgentExtension::OnDisconnect(const AAFwk::Want& want)
{
    HILOGI("OnDisconnect");
    Extension::OnDisconnect(want);
}

void ETSPartnerAgentExtension::OnStop()
{
    HILOGI("OnStop");
    if (handler_ == nullptr) {
        HILOGE("handler is invalid");
        Extension::OnStop();
        return;
    }
    if (etsAbilityObj_ == nullptr) {
        HILOGE("etsAbilityObj_ is null");
        Extension::OnStop();
        return;
    }
    auto runner = handler_->GetEventRunner();
    if (runner == nullptr) {
        HILOGE("event runner is invalid");
        Extension::OnStop();
        return;
    }
    constexpr int32_t reason = 1;
    if (runner->IsCurrentRunnerThread()) {
        auto env = etsRuntime_.GetAniEnv();
        if (env == nullptr) {
            HILOGE("env is null");
            Extension::OnStop();
            return;
        }
        AniLocalScopeGuard scopeGuard(env);
        CallOnDestroyWithReason(env, reason);
    } else {
        OnDestroyWithReason(reason);
    }
    Extension::OnStop();
}

PartnerAgentExtensionResult ETSPartnerAgentExtension::OnDestroyWithReason(const int32_t reason)
{
    HILOGI("enter OnDestroyWithReason is %{public}d", reason);
    if (handler_ == nullptr) {
        HILOGE("handler is invalid");
        return PartnerAgentExtensionResult::INTERNAL_ERROR;
    }
    std::weak_ptr<ETSPartnerAgentExtension> wThis =
        std::static_pointer_cast<ETSPartnerAgentExtension>(shared_from_this());
    auto task = [wThis, reason]() {
        std::shared_ptr<ETSPartnerAgentExtension> sThis = wThis.lock();
        if (sThis == nullptr) {
            HILOGE("null sThis");
            return;
        }
        auto env = sThis->etsRuntime_.GetAniEnv();
        if (env == nullptr) {
            HILOGE("env is null");
            return;
        }
        AniLocalScopeGuard scopeGuard(env);
        sThis->CallOnDestroyWithReason(env, reason);
    };
    handler_->PostTask(task, "OnDestroyWithReason");
    return PartnerAgentExtensionResult::OK;
}

void ETSPartnerAgentExtension::CallOnDestroyWithReason(ani_env* env, int32_t reason)
{
    if (env == nullptr) {
        HILOGE("env is null");
        return;
    }
    if (notified_.exchange(true)) {
        HILOGW("onDestroyWithReason already notified");
        return;
    }
    ani_int iReson = static_cast<ani_int>(reason);
    CallObjectMethod(false, "onDestroyWithReason", "i:", iReson);
}

PartnerAgentExtensionResult ETSPartnerAgentExtension::OnDeviceDiscovered(const PartnerDeviceAddress& deviceAddress)
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
    std::weak_ptr<ETSPartnerAgentExtension> wThis =
        std::static_pointer_cast<ETSPartnerAgentExtension>(shared_from_this());
    auto task = [wThis, deviceAddress]() {
        std::shared_ptr<ETSPartnerAgentExtension> sThis = wThis.lock();
        if (sThis == nullptr) {
            HILOGE("null sThis");
            return;
        }
        auto env = sThis->etsRuntime_.GetAniEnv();
        if (env == nullptr) {
            HILOGE("env is null");
            return;
        }
        AniLocalScopeGuard scopeGuard(env);
        ani_object addressObj = sThis->PartnerDeviceAddressToAni(env, deviceAddress);
        if (addressObj == nullptr) {
            HILOGE("addressObj is null");
            return;
        }
        auto addressRef = static_cast<ani_ref>(addressObj);
        sThis->CallObjectMethod(false, "onDeviceDiscovered",
            "C{@ohos.FusionConnectivity.partnerAgent.PartnerDeviceAddress}:", addressRef);
    };
    handler_->PostTask(task, "OnDeviceDiscovered");
    return PartnerAgentExtensionResult::OK;
}

ani_ref ETSPartnerAgentExtension::CallObjectMethod(bool withResult, const char* name, const char* signature, ...)
{
    HILOGI("CallObjectMethod %{public}s", name);
    ani_status status = ANI_ERROR;
    ani_method method = nullptr;
    auto env = etsRuntime_.GetAniEnv();
    if (env == nullptr) {
        HILOGE("env is null");
        return nullptr;
    }
    AniLocalScopeGuard scopeGuard(env);
    if (etsAbilityObj_ == nullptr) {
        HILOGE("etsAbilityObj_ is null");
        return nullptr;
    }
    if ((status = env->Class_FindMethod(etsAbilityObj_->aniCls, name, signature, &method)) != ANI_OK) {
        HILOGE("Class_FindMethod failed, status : %{public}d", status);
        return nullptr;
    }
    if (method == nullptr) {
        HILOGE("method is null");
        return nullptr;
    }
    ani_ref res = nullptr;
    va_list args;
    if (withResult) {
        va_start(args, signature);
        status = env->Object_CallMethod_Ref_V(etsAbilityObj_->aniObj, method, &res, args);
        va_end(args);
        if (status != ANI_OK) {
            HILOGE("Object_CallMethod_Ref_V failed, status : %{public}d", status);
            return nullptr;
        }
        return res;
    }
    va_start(args, signature);
    if ((status = env->Object_CallMethod_Void_V(etsAbilityObj_->aniObj, method, args)) != ANI_OK) {
        HILOGE("Object_CallMethod_Void_V failed, status : %{public}d", status);
    }
    va_end(args);
    return nullptr;
}

ani_object ETSPartnerAgentExtension::PartnerDeviceAddressToAni(ani_env* env, const PartnerDeviceAddress& deviceAddress)
{
    if (env == nullptr) {
        HILOGE("env is null");
        return nullptr;
    }
    ani_class aniClass;
    ani_status status = env->FindClass(
        "@ohos.FusionConnectivity.partnerAgent.PartnerDeviceAddress", &aniClass);
    if (status != ANI_OK) {
        HILOGE("FindClass failed, status : %{public}d", status);
        return nullptr;
    }
    ani_method aniMethod;
    status = env->Class_FindMethod(aniClass, "<ctor>", nullptr, &aniMethod);
    if (status != ANI_OK) {
        HILOGE("Class_FindMethod failed, status : %{public}d", status);
        return nullptr;
    }
    ani_object aniAddress;
    status = env->Object_New(aniClass, aniMethod, &aniAddress);
    if (status != ANI_OK) {
        HILOGE("Object_New failed, status : %{public}d", status);
        return nullptr;
    }
    auto aniBluetoothAddressObj = BluetoothAddressToAni(env, deviceAddress);
    if (aniBluetoothAddressObj == nullptr) {
        HILOGE("aniBluetoothAddressObj is null");
        return nullptr;
    }
    status = env->Object_SetPropertyByName_Ref(aniAddress, "bluetoothAddress", aniBluetoothAddressObj);
    if (status != ANI_OK) {
        HILOGE("Object_SetPropertyByName_Ref failed, status : %{public}d", status);
        return nullptr;
    }
    return aniAddress;
}

static constexpr std::pair<int32_t, const char *> BLUETOOTH_ADDRESS_TYPE_MAP[] = {
    { static_cast<int32_t>(BluetoothAddressType::VIRTUAL), "VIRTUAL" },
    { static_cast<int32_t>(BluetoothAddressType::REAL), "REAL" },
};

static constexpr std::pair<int32_t, const char *> BLUETOOTH_RAW_ADDRESS_TYPE_MAP[] = {
    { static_cast<int32_t>(BluetoothRawAddressType::PUBLIC), "PUBLIC" },
    { static_cast<int32_t>(BluetoothRawAddressType::RANDOM), "RANDOM" },
};

static ani_enum_item GetEnumItemByValue(ani_env *env, const char *enumName, int32_t value,
    const std::pair<int32_t, const char *> *valueNameMap, size_t mapSize)
{
    const char *itemName = nullptr;
    for (size_t i = 0; i < mapSize; i++) {
        if (valueNameMap[i].first == value) {
            itemName = valueNameMap[i].second;
            break;
        }
    }
    if (itemName == nullptr) {
        HILOGE("unknown enum value %{public}d for %{public}s", value, enumName);
        return nullptr;
    }
    ani_enum aniEnum = nullptr;
    ani_status status = env->FindEnum(enumName, &aniEnum);
    if (status != ANI_OK || aniEnum == nullptr) {
        HILOGE("FindEnum %{public}s failed, status : %{public}d", enumName, status);
        return nullptr;
    }
    ani_enum_item item = nullptr;
    status = env->Enum_GetEnumItemByName(aniEnum, itemName, &item);
    if (status != ANI_OK || item == nullptr) {
        HILOGE("Enum_GetEnumItemByName %{public}s failed, status : %{public}d", itemName, status);
        return nullptr;
    }
    return item;
}

ani_object ETSPartnerAgentExtension::CreateAniBluetoothAddress(ani_env* env)
{
    if (env == nullptr) {
        HILOGE("env is null");
        return nullptr;
    }
    ani_class aniClass;
    ani_status status = env->FindClass("@ohos.bluetooth.common.common.BluetoothAddress", &aniClass);
    if (status != ANI_OK) {
        HILOGE("FindClass failed, status : %{public}d", status);
        return nullptr;
    }
    ani_method aniMethod;
    status = env->Class_FindMethod(aniClass, "<ctor>", nullptr, &aniMethod);
    if (status != ANI_OK) {
        HILOGE("Class_FindMethod failed, status : %{public}d", status);
        return nullptr;
    }
    ani_object aniBluetoothAddress;
    status = env->Object_New(aniClass, aniMethod, &aniBluetoothAddress);
    if (status != ANI_OK) {
        HILOGE("Object_New failed, status : %{public}d", status);
        return nullptr;
    }
    return aniBluetoothAddress;
}

bool ETSPartnerAgentExtension::SetAniEnumProperty(ani_env* env, ani_object targetObj, const char* propName,
    const char* enumName, int32_t enumValue,
    const std::pair<int32_t, const char *> *valueNameMap, size_t mapSize)
{
    if (env == nullptr || targetObj == nullptr || propName == nullptr || enumName == nullptr) {
        HILOGE("invalid param");
        return false;
    }
    ani_enum_item item = GetEnumItemByValue(env, enumName, enumValue, valueNameMap, mapSize);
    if (item == nullptr) {
        HILOGE("GetEnumItemByValue failed for %{public}s", enumName);
        return false;
    }
    ani_status status = env->Object_SetPropertyByName_Ref(targetObj, propName, item);
    if (status != ANI_OK) {
        HILOGE("Object_SetPropertyByName_Ref %{public}s failed, status : %{public}d", propName, status);
        return false;
    }
    return true;
}

bool ETSPartnerAgentExtension::SetAniBluetoothAddressProperties(ani_env* env, ani_object aniBluetoothAddress,
    const PartnerDeviceAddress& deviceAddress)
{
    if (env == nullptr || aniBluetoothAddress == nullptr) {
        HILOGE("invalid param");
        return false;
    }
    ani_string aniAddressStr = GetAniString(env, deviceAddress.GetAddress());
    if (aniAddressStr == nullptr) {
        HILOGE("GetAniString failed");
        return false;
    }
    ani_status status = env->Object_SetPropertyByName_Ref(aniBluetoothAddress, "address", aniAddressStr);
    if (status != ANI_OK) {
        HILOGE("Object_SetPropertyByName_Ref address failed, status : %{public}d", status);
        return false;
    }
    bool ret = SetAniEnumProperty(env, aniBluetoothAddress, "addressType",
        "@ohos.bluetooth.common.common.BluetoothAddressType",
        static_cast<int32_t>(deviceAddress.GetAddressType()),
        BLUETOOTH_ADDRESS_TYPE_MAP, sizeof(BLUETOOTH_ADDRESS_TYPE_MAP) / sizeof(BLUETOOTH_ADDRESS_TYPE_MAP[0]));
    if (!ret) {
        HILOGE("SetAniEnumProperty addressType failed");
        return false;
    }
    if (deviceAddress.HasRawAddressType()) {
        ret = SetAniEnumProperty(env, aniBluetoothAddress, "rawAddressType",
            "@ohos.bluetooth.common.common.BluetoothRawAddressType",
            static_cast<int32_t>(deviceAddress.GetRawAddressType()),
            BLUETOOTH_RAW_ADDRESS_TYPE_MAP,
            sizeof(BLUETOOTH_RAW_ADDRESS_TYPE_MAP) / sizeof(BLUETOOTH_RAW_ADDRESS_TYPE_MAP[0]));
        if (!ret) {
            HILOGE("SetAniEnumProperty rawAddressType failed");
            return false;
        }
    }
    return true;
}

ani_object ETSPartnerAgentExtension::BluetoothAddressToAni(ani_env* env, const PartnerDeviceAddress& deviceAddress)
{
    if (env == nullptr) {
        HILOGE("env is null");
        return nullptr;
    }
    ani_object aniBluetoothAddress = CreateAniBluetoothAddress(env);
    if (aniBluetoothAddress == nullptr) {
        HILOGE("CreateAniBluetoothAddress failed");
        return nullptr;
    }
    if (!SetAniBluetoothAddressProperties(env, aniBluetoothAddress, deviceAddress)) {
        HILOGE("SetAniBluetoothAddressProperties failed");
        return nullptr;
    }
    return aniBluetoothAddress;
}

ani_string ETSPartnerAgentExtension::GetAniString(ani_env *env, const std::string &str)
{
    if (env == nullptr) {
        HILOGE("env is null");
        return nullptr;
    }
    ani_string aniStr = nullptr;
    ani_status status = env->String_NewUTF8(str.c_str(), str.size(), &aniStr);
    if (status != ANI_OK) {
        HILOGE("status: %{public}d", status);
        return nullptr;
    }
    return aniStr;
}
}  // namespace FusionConnectivity
}  // namespace OHOS
