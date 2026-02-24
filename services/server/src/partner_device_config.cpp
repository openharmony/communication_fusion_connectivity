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

#include "partner_device_config.h"
#include "log.h"
#include "file_ex.h"
#include "datetime_ex.h"
#include "accesstoken_kit.h"
#include "access_token_error.h"
#include "common_utils.h"
#include "cJSON.h"
#include "bluetooth_host.h"
#include "parameter.h"
#include "parameters.h"

namespace OHOS {
namespace FusionConnectivity {
using namespace OHOS::Bluetooth;

#ifdef __cplusplus
extern "C" {
#endif

static constexpr const char *PARTNER_DEVICE_CONFIG_PATH =
        "/data/service/el1/public/partner_device_agent/partner_agent_device.xml";
const int MAX_PARTNER_DEVICE_CONFIG_SIZE = 100;
const int MAX_LOST_TIME_DAYS = 30;
std::mutex g_configFileMutex;

static bool IsPairedDevice(const PartnerDeviceAddress &deviceAddress)
{
    int pairState = PAIR_NONE;
    auto device = BluetoothHost::GetDefaultHost().GetRemoteDevice(
        deviceAddress.GetAddress(), Bluetooth::BTTransport::ADAPTER_BREDR);
    device.GetPairState(pairState);
    return pairState == PAIR_PAIRED;
}

static void JsonAddDeviceCapabilityToObject(cJSON *item, PartnerDevice::DeviceInfo &deviceInfo)
{
    cJSON *capability = cJSON_CreateObject();
    if (!capability) {
        HILOGE("capability is nullptr");
        return;
    }
    cJSON_AddBoolToObject(capability, "isSupportBR", deviceInfo.capability.isSupportBR);
    cJSON_AddBoolToObject(capability, "isSupportBleAdvertiser", deviceInfo.capability.isSupportBleAdvertiser);
    cJSON_AddItemToObject(item, "capability", capability);
}

static void JsonAddBusinessCapabilityToObject(cJSON *item, PartnerDevice::DeviceInfo &deviceInfo)
{
    cJSON *businessCapability = cJSON_CreateObject();
    if (!businessCapability) {
        HILOGE("businessCapability is nullptr");
        return;
    }
    cJSON_AddBoolToObject(
        businessCapability, "isSupportMediaControl", deviceInfo.businessCapability.isSupportMediaControl);
    cJSON_AddBoolToObject(
        businessCapability, "isSupportTelephonyControl", deviceInfo.businessCapability.isSupportTelephonyControl);
    cJSON_AddItemToObject(item, "businessCapability", businessCapability);
}

void UpdatePartnerDeviceConfig(PartnerDeviceMap &deviceMap)
{
    cJSON *root = cJSON_CreateArray();
    FCM_CHECK_RETURN(root, "root is nullptr");

    deviceMap.Iterate([root](const PartnerDeviceMapKey &key, std::shared_ptr<PartnerDevice> &deviceSptr) {
        FCM_CHECK_RETURN(deviceSptr, "deviceSptr is nullptr");
        PartnerDevice::DeviceInfo deviceInfo = deviceSptr->GetDeviceInfo();

        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "version", "1.0");
        cJSON_AddStringToObject(item, "bundleName", deviceInfo.bundleName.c_str());
        cJSON_AddStringToObject(item, "abilityName", deviceInfo.abilityName.c_str());
        {
            cJSON *addressInfo = cJSON_CreateObject();
            std::string address = deviceInfo.deviceAddress.GetAddress();
            cJSON_AddStringToObject(addressInfo, "address", address.c_str());
            cJSON_AddNumberToObject(
                addressInfo, "addressType", static_cast<int>(deviceInfo.deviceAddress.GetAddressType()));
            if (deviceInfo.deviceAddress.HasRawAddressType()) {
                cJSON_AddNumberToObject(
                    addressInfo, "rawAddressType", static_cast<int>(deviceInfo.deviceAddress.GetRawAddressType()));
            }
            cJSON_AddItemToObject(item, "addressInfo", addressInfo);
        }
        cJSON_AddNumberToObject(item, "tokenId", deviceInfo.tokenId);
        cJSON_AddNumberToObject(item, "registerTimestamp", deviceInfo.registerTimestamp);
        if (deviceInfo.lostTimestamp > 0) {
            cJSON_AddNumberToObject(item, "lostTimestamp", deviceInfo.lostTimestamp);
        }
        cJSON_AddBoolToObject(item, "isUserEnabled", deviceInfo.isUserEnabled);
        JsonAddDeviceCapabilityToObject(item, deviceInfo);
        JsonAddBusinessCapabilityToObject(item, deviceInfo);

        cJSON_AddItemToArray(root, item);
    });

    char *jsonString = cJSON_Print(root);
    if (jsonString == nullptr) {
        cJSON_Delete(root);
        return;
    }
    {
        // 写文件持久化
        std::lock_guard<std::mutex> lock(g_configFileMutex);
        SaveStringToFile(PARTNER_DEVICE_CONFIG_PATH, jsonString);
    }

    cJSON_free(jsonString);
    cJSON_Delete(root);

    // 写配置参数，避免SA被无效拉起，导致应用页面加载时间变长
    std::string sizeStr = std::to_string(deviceMap.Size());
    SetParameter("persist.fusion_connectivity.partner_agent_devices", sizeStr.c_str());
}

void ClearPartnerDeviceConfig()
{
    // 写文件持久化
    std::lock_guard<std::mutex> lock(g_configFileMutex);
    SaveStringToFile(PARTNER_DEVICE_CONFIG_PATH, "[]");
}

int ParseAddressInfoJsonObject(const cJSON *addressInfo, PartnerDeviceAddress &deviceAddress)
{
    const cJSON *address = cJSON_GetObjectItemCaseSensitive(addressInfo, "address");
    const cJSON *addressType = cJSON_GetObjectItemCaseSensitive(addressInfo, "addressType");
    // rawAddressType 是可选参数
    const cJSON *rawAddressType = cJSON_GetObjectItemCaseSensitive(addressInfo, "rawAddressType");
    bool isCheckFail = !cJSON_IsString(address) || !cJSON_IsNumber(addressType) ||
        (rawAddressType != nullptr && !cJSON_IsNumber(rawAddressType));
    if (isCheckFail) {
        HILOGE("Invalid addressInfo format.");
        return FCM_ERR_INTERNAL_ERROR;
    }
    if (!IsValidAddress(address->valuestring)) {
        HILOGE("Invalid address");
        return FCM_ERR_INTERNAL_ERROR;
    }
    if (addressType->valueint < static_cast<int>(BluetoothAddressType::VIRTUAL) ||
        addressType->valueint > static_cast<int>(BluetoothAddressType::REAL)) {
        HILOGE("Invalid addressType: %{public}d", addressType->valueint);
        return FCM_ERR_INTERNAL_ERROR;
    }
    deviceAddress = PartnerDeviceAddress(
        address->valuestring, static_cast<BluetoothAddressType>(addressType->valueint));
    if (rawAddressType != nullptr) {
        if (rawAddressType->valueint < static_cast<int>(BluetoothRawAddressType::PUBLIC) ||
            rawAddressType->valueint > static_cast<int>(BluetoothRawAddressType::RANDOM)) {
            HILOGE("Invalid rawAddressType: %{public}d", rawAddressType->valueint);
            return FCM_ERR_INTERNAL_ERROR;
        }
        deviceAddress = PartnerDeviceAddress(
            address->valuestring,
            static_cast<BluetoothAddressType>(addressType->valueint),
            static_cast<BluetoothRawAddressType>(rawAddressType->valueint));
    }
    return FCM_NO_ERROR;
}

int ParseDeviceCapability(const cJSON *capability, DeviceCapability &deviceCapability)
{
    const cJSON *isSupportBR = cJSON_GetObjectItemCaseSensitive(capability, "isSupportBR");
    const cJSON *isSupportBleAdvertiser = cJSON_GetObjectItemCaseSensitive(capability, "isSupportBleAdvertiser");
    bool isCheckFail = !cJSON_IsBool(isSupportBR) || !cJSON_IsBool(isSupportBleAdvertiser);
    if (isCheckFail) {
        HILOGE("Invalid capability format.");
        return FCM_ERR_INTERNAL_ERROR;
    }
    deviceCapability.isSupportBR = static_cast<bool>(isSupportBR->valueint);
    deviceCapability.isSupportBleAdvertiser = static_cast<bool>(isSupportBleAdvertiser->valueint);
    return FCM_NO_ERROR;
}

int ParseBusinessCapability(const cJSON *businessCapabilityJson, BusinessCapability &businessCapability)
{
    const cJSON *isSupportMediaControl =
        cJSON_GetObjectItemCaseSensitive(businessCapabilityJson, "isSupportMediaControl");
    const cJSON *isSupportTelephonyControl =
        cJSON_GetObjectItemCaseSensitive(businessCapabilityJson, "isSupportTelephonyControl");
    bool isCheckFail = !cJSON_IsBool(isSupportMediaControl) || !cJSON_IsBool(isSupportTelephonyControl);
    if (isCheckFail) {
        HILOGE("Invalid business capability format.");
        return FCM_ERR_INTERNAL_ERROR;
    }
    businessCapability.isSupportMediaControl = static_cast<bool>(isSupportMediaControl->valueint);
    businessCapability.isSupportTelephonyControl = static_cast<bool>(isSupportTelephonyControl->valueint);
    return FCM_NO_ERROR;
}

static bool IsValidAppTokenId(uint32_t tokenId)
{
    OHOS::Security::AccessToken::HapTokenInfo hapTokenInfo;
    int ret = OHOS::Security::AccessToken::AccessTokenKit::GetHapTokenInfo(tokenId, hapTokenInfo);
    return ret != OHOS::Security::AccessToken::ERR_TOKENID_NOT_EXIST;
}

int ParseLostTimestamp(
    const cJSON *lostTimestamp, const PartnerDeviceAddress &deviceAddress, PartnerDevice::DeviceInfo &deviceInfo)
{
    if (cJSON_IsNumber(lostTimestamp)) {
        int64_t now = GetDaysSince1970ToNow();
        if (now - lostTimestamp->valueint > MAX_LOST_TIME_DAYS) {
            HILOGE("The device has been unpaired for more than 30 days; delete it.");
            return FCM_ERR_INTERNAL_ERROR;
        }
        deviceInfo.lostTimestamp = lostTimestamp->valueint;
    } else {
        deviceInfo.lostTimestamp = 0;
    }
    // 在SA下电期间，该设备已重新配对上
    if (IsPairedDevice(deviceAddress)) {
        deviceInfo.lostTimestamp = 0;
    }
    return FCM_NO_ERROR;
}

int ParsePartnerDeviceInfo(cJSON *root, int index, PartnerDevice::DeviceInfo &deviceInfo)
{
    cJSON *item = cJSON_GetArrayItem(root, index);
    FCM_CHECK_RETURN_RET(item, FCM_ERR_INTERNAL_ERROR, "Failed to get array item (%{public}d).", index);

    const cJSON *version = cJSON_GetObjectItemCaseSensitive(item, "version");
    const cJSON *bundleName = cJSON_GetObjectItemCaseSensitive(item, "bundleName");
    const cJSON *abilityName = cJSON_GetObjectItemCaseSensitive(item, "abilityName");
    const cJSON *addressInfo = cJSON_GetObjectItemCaseSensitive(item, "addressInfo");
    const cJSON *tokenId = cJSON_GetObjectItemCaseSensitive(item, "tokenId");
    const cJSON *registerTimestamp = cJSON_GetObjectItemCaseSensitive(item, "registerTimestamp");
    const cJSON *lostTimestamp = cJSON_GetObjectItemCaseSensitive(item, "lostTimestamp");
    const cJSON *capability = cJSON_GetObjectItemCaseSensitive(item, "capability");
    const cJSON *businessCapabilityJson = cJSON_GetObjectItemCaseSensitive(item, "businessCapability");
    const cJSON *isUserEnabled = cJSON_GetObjectItemCaseSensitive(item, "isUserEnabled");
    // 检查字段是否存在
    bool isCheckFail = !cJSON_IsString(version) || !cJSON_IsString(bundleName) || !cJSON_IsString(abilityName) ||
        !cJSON_IsObject(addressInfo) || !cJSON_IsNumber(tokenId) || !cJSON_IsNumber(registerTimestamp) ||
        !cJSON_IsObject(capability) || !cJSON_IsBool(isUserEnabled);
    if (isCheckFail) {
        HILOGE("Invalid JSON format.");
        return FCM_ERR_INTERNAL_ERROR;
    }
    // 解析 addressInfo 对象
    PartnerDeviceAddress deviceAddress;
    if (ParseAddressInfoJsonObject(addressInfo, deviceAddress) != FCM_NO_ERROR) {
        return FCM_ERR_INTERNAL_ERROR;
    }
    // 解析 capability 对象
    DeviceCapability deviceCapability;
    if (ParseDeviceCapability(capability, deviceCapability) != FCM_NO_ERROR) {
        return FCM_ERR_INTERNAL_ERROR;
    }
    // 解析 businessCapability 对象
    BusinessCapability businessCapability;
    if (ParseBusinessCapability(businessCapabilityJson, businessCapability) != FCM_NO_ERROR) {
        return FCM_ERR_INTERNAL_ERROR;
    }
    // 检查tokenid是否有效
    if (!IsValidAppTokenId(tokenId->valueint)) {
        HILOGE("Invalid tokenId: %{public}d, app is uninstall", tokenId->valueint);
        return FCM_ERR_INTERNAL_ERROR;
    }
    // 检查 lostTimestamp 注册是否失效
    if (ParseLostTimestamp(lostTimestamp, deviceAddress, deviceInfo) != FCM_NO_ERROR) {
        return FCM_ERR_INTERNAL_ERROR;
    }

    deviceInfo.bundleName = bundleName->valuestring;
    deviceInfo.abilityName = abilityName->valuestring;
    deviceInfo.deviceAddress = deviceAddress;
    deviceInfo.tokenId = tokenId->valueint;
    deviceInfo.registerTimestamp = registerTimestamp->valueint;
    deviceInfo.isUserEnabled = static_cast<bool>(isUserEnabled->valueint);
    deviceInfo.capability = deviceCapability;
    deviceInfo.businessCapability = businessCapability;
    return FCM_NO_ERROR;
}

void LoadPartnerDeviceConfig(PartnerDeviceMap &deviceMap, CreatePartnerDeviceFunc createPartnerDeviceFunc)
{
    std::string fileContent;
    {
        std::lock_guard<std::mutex> lock(g_configFileMutex);
        bool ret = LoadStringFromFile(PARTNER_DEVICE_CONFIG_PATH, fileContent);
        if (!ret) {
            HILOGE("load partner device config failed");
            return;
        }
    }
    // 解析 JSON 字符串
    cJSON *root = cJSON_Parse(fileContent.c_str());
    if (root == nullptr) {
        const char *errorPtr = cJSON_GetErrorPtr();
        if (errorPtr != NULL) {
            HILOGE("parse partner device config failed, %{public}s", errorPtr);
        }
        ClearPartnerDeviceConfig();
        return;
    }
    int size = cJSON_GetArraySize(root);
    if (size > MAX_PARTNER_DEVICE_CONFIG_SIZE) {
        HILOGE("partner device config size reach max, there may be resource leaks.");
        ClearPartnerDeviceConfig();
        cJSON_Delete(root);
        return;
    }

    for (int i = 0; i < size; i++) {
        PartnerDevice::DeviceInfo deviceInfo;
        int ret = ParsePartnerDeviceInfo(root, i, deviceInfo);
        if (ret != FCM_NO_ERROR) {
            // 清除虚拟MAC固化
            continue;
        }
        auto key = std::make_pair(deviceInfo.tokenId, deviceInfo.deviceAddress.GetAddress());
        deviceMap.EnsureInsert(key, createPartnerDeviceFunc(deviceInfo));
    }

    // 释放内存
    cJSON_Delete(root);
    // 重新刷新配置文件
    UpdatePartnerDeviceConfig(deviceMap);
}

#ifdef __cplusplus
}
#endif

}  // namespace OHOS
}  // namespace FusionConnectivity
