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
#define LOG_TAG "BundleHelper"
#endif

#include "bundle_mgr_proxy.h"
#include "bundle_helper.h"
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "image_source.h"
#include "os_account_manager.h"
#include "system_ability_definition.h"

#include "log.h"

static const std::string IMAGE_FORMAT_PNG = "image/png";
static const std::string SETTING_BUNDLE_NAME = FUSION_CONNECTIVITY_SETTINGS_BUNDLE_NAME;
static const int32_t DEFAULT_ICON_WIDTH = 60;
static const int32_t DEFAULT_ICON_HEIGHT = 60;

namespace OHOS {
namespace FusionConnectivity {
BundleHelper::BundleHelper()
{
    deathRecipient_ = new (std::nothrow)
        RemoteDeathRecipient(std::bind(&BundleHelper::OnRemoteDied, this, std::placeholders::_1));
    if (deathRecipient_ == nullptr) {
        HILOGE("Failed to create RemoteDeathRecipient instance");
    }
}

BundleHelper::~BundleHelper()
{
    std::lock_guard<ffrt::mutex> lock(connectionMutex_);
    Disconnect();
}

void BundleHelper::OnRemoteDied(const wptr<IRemoteObject> &object)
{
    std::lock_guard<ffrt::mutex> lock(connectionMutex_);
    Disconnect();
}

void BundleHelper::Connect()
{
    if (bundleMgr_ != nullptr) {
        return;
    }

    sptr<ISystemAbilityManager> systemAbilityManager =
        SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityManager == nullptr) {
        return;
    }

    sptr<IRemoteObject> remoteObject = systemAbilityManager->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (remoteObject == nullptr) {
        return;
    }

    bundleMgr_ = new AppExecFwk::BundleMgrProxy(remoteObject);
    if (bundleMgr_ != nullptr && deathRecipient_ != nullptr) {
        bundleMgr_->AsObject()->AddDeathRecipient(deathRecipient_);
    }
}

void BundleHelper::Disconnect()
{
    if (bundleMgr_ != nullptr) {
        bundleMgr_->AsObject()->RemoveDeathRecipient(deathRecipient_);
        bundleMgr_ = nullptr;
    }
    deathRecipient_ = nullptr;
}

bool BundleHelper::GetBundleInfoV9(
    const std::string& bundle, const int32_t flag,
    AppExecFwk::BundleInfo &bundleInfo, const int32_t userId)
{
    std::lock_guard<ffrt::mutex> lock(connectionMutex_);
    Connect();

    if (bundleMgr_ == nullptr) {
        return false;
    }
    int32_t ret = bundleMgr_->GetBundleInfoV9(bundle, flag, bundleInfo, userId);
    if (ret != ERR_OK) {
        HILOGE("Bundle failed %{public}s %{public}d %{public}d %{public}d.", bundle.c_str(),
            flag, userId, ret);
        return false;
    }
    return true;
}


bool BundleHelper::CheckPartnerAgentExtensionAbility(const std::string &bundleName, const int32_t userId)
{
    auto flags = static_cast<int32_t>(AppExecFwk::GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_APPLICATION)
        | static_cast<int32_t>(AppExecFwk::GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_HAP_MODULE)
        | static_cast<int32_t>(AppExecFwk::GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_ABILITY)
        | static_cast<int32_t>(AppExecFwk::GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_EXTENSION_ABILITY);

    AppExecFwk::BundleInfo bundleInfo;
    if (!GetBundleInfoV9(bundleName, flags, bundleInfo, userId)) {
        HILOGE("GetBundleInfoV9 error, bundleName = %{public}s, userId = %{public}d",
            bundleName.c_str(), userId);
        return false;
    }
    for (const auto& hapmodule : bundleInfo.hapModuleInfos) {
        for (const auto& extInfo : hapmodule.extensionInfos) {
            if (extInfo.type == AppExecFwk::ExtensionAbilityType::PARTNER_AGENT) {
                return true;
            }
        }
    }
    return false;
}

std::string BundleHelper::GetBundleLabel(const std::string& bundleName)
{
    AppExecFwk::BundleResourceInfo bundleResourceInfo = {};
    int32_t result = GetBundleResourceInfo(bundleName, bundleResourceInfo, 0);
    if (result != ERR_OK) {
        HILOGE("GetBundleResourceInfo failed. result is %{public}d", result);
        return "";
    }
    return bundleResourceInfo.label;
}

BundleHelper& BundleHelper::GetInstance()
{
    static BundleHelper instance;
    return instance;
}

std::shared_ptr<Media::PixelMap> BundleHelper::GetSettingsAbilityIcon()
{
    AppExecFwk::BundleResourceInfo bundleResourceInfo = {};
    int32_t result = GetBundleResourceInfo(SETTING_BUNDLE_NAME, bundleResourceInfo, 0);

    std::shared_ptr<Media::PixelMap> iconPixelmap = UnPackImage(bundleResourceInfo.icon);
    if (!ImageScale(iconPixelmap, DEFAULT_ICON_WIDTH, DEFAULT_ICON_HEIGHT)) {
        return nullptr;
    }
    return iconPixelmap;
}

ErrCode BundleHelper::GetBundleResourceInfo(const std::string &bundleName,
    AppExecFwk::BundleResourceInfo &bundleResourceInfo, const int32_t appIndex)
{
    ErrCode result = 0;
    std::lock_guard<ffrt::mutex> lock(connectionMutex_);
    Connect();
    if (bundleMgr_ == nullptr) {
        HILOGE("GetBundleInfo bundle proxy failed.");
        return -1;
    }
    sptr<AppExecFwk::IBundleResource> bundleResourceProxy = bundleMgr_->GetBundleResourceProxy();
    if (!bundleResourceProxy) {
        HILOGE("GetBundleInfo, get bundle resource proxy failed.");
        return -1;
    }

    int32_t flag = static_cast<int32_t>(AppExecFwk::ResourceFlag::GET_RESOURCE_INFO_ALL) |
        static_cast<int32_t>(AppExecFwk::ResourceFlag::GET_RESOURCE_INFO_WITH_LABEL);
    result = bundleResourceProxy->GetBundleResourceInfo(bundleName, flag, bundleResourceInfo, appIndex);
    return result;
}

bool BundleHelper::GetPartnerAgentExtensionAbilityLabel(const std::string &bundleName,
    const int32_t userId, std::string &label)
{
    label = "";
    auto flags = static_cast<int32_t>(AppExecFwk::GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_APPLICATION)
        | static_cast<int32_t>(AppExecFwk::GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_HAP_MODULE)
        | static_cast<int32_t>(AppExecFwk::GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_ABILITY)
        | static_cast<int32_t>(AppExecFwk::GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_EXTENSION_ABILITY);

    AppExecFwk::BundleInfo bundleInfo;
    if (!GetBundleInfoV9(bundleName, flags, bundleInfo, userId)) {
        HILOGE("GetBundleInfoV9 error, bundleName = %{public}s, userId = %{public}d",
            bundleName.c_str(), userId);
        return false;
    }
    for (const auto& hapmodule : bundleInfo.hapModuleInfos) {
        for (const auto& extInfo : hapmodule.extensionInfos) {
            if (extInfo.type == AppExecFwk::ExtensionAbilityType::PARTNER_AGENT) {
                label = GetBundleLabel(bundleName);
                return true;
            }
        }
    }
    return false;
}

std::shared_ptr<Media::PixelMap> BundleHelper::UnPackImage(const std::string &pixelMapStr)
{
    if (pixelMapStr.empty()) {
        return nullptr;
    }
    uint32_t errorCode {ERR_OK};
    Media::SourceOptions opts;
    auto imageSource = Media::ImageSource::CreateImageSource(reinterpret_cast<const uint8_t*>(pixelMapStr.data()),
        static_cast<uint32_t>(pixelMapStr.length()), opts, errorCode);
    if (errorCode != ERR_OK || !imageSource) {
        HILOGE("create imageSource failed");
        return nullptr;
    }
    Media::DecodeOptions decodeOpts;
    auto pixelMap = imageSource->CreatePixelMap(decodeOpts, errorCode);
    if (errorCode != ERR_OK || !pixelMap) {
        HILOGE("Failed to create pixelMap.");
        return nullptr;
    }
    return pixelMap;
}

bool BundleHelper::ImageScale(const std::shared_ptr<Media::PixelMap> &pixelMap, int32_t width, int32_t height)
{
    if (!pixelMap || width == 0 || height == 0) {
        HILOGE("invalid parameters");
        return false;
    }
 
    Media::ImageInfo imageInfo;
    pixelMap->GetImageInfo(imageInfo);
    if (imageInfo.size.width == 0|| imageInfo.size.height == 0) {
        HILOGE("invalid image info.");
        return false;
    }
    float xAxis = static_cast<float>(width) / static_cast<float>(imageInfo.size.width);
    float yAxis = static_cast<float>(height) / static_cast<float>(imageInfo.size.height);
    pixelMap->scale(xAxis, yAxis, Media::AntiAliasingOption::HIGH);
    return true;
}

}  // namespace FusionConnectivity
}  // namespace OHOS
