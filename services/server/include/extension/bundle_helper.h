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

#ifndef BUNDLE_HELPER_H
#define BUNDLE_HELPER_H

#include <memory>
#include <mutex>
#include <string>

#include "bundle_mgr_interface.h"
#include "ipc_skeleton.h"
#include "iremote_object.h"
#include "refbase.h"
#include "remote_death_recipient.h"
#include "pixel_map.h"
#include "ffrt.h"

namespace OHOS {
namespace FusionConnectivity {
class BundleHelper {
public:
    static BundleHelper& GetInstance();
    /**
     * @brief GetBundleInfoV9.
     * @param bundle bundle name.
     * @param flag query condition.
     * @param bundleInfo bundle info.
     * @param userId userId.
     * @return Returns true if succeed, retrun false otherwise.
     */
    bool GetBundleInfoV9(const std::string& bundle, const int32_t flag,
        AppExecFwk::BundleInfo &bundleInfo, const int32_t userId);

    bool CheckPartnerAgentExtensionAbility(const std::string &bundleName, const int32_t userId);

    bool GetPartnerAgentExtensionAbilityLabel(const std::string &bundleName, const int32_t userId, std::string &label);

    std::shared_ptr<Media::PixelMap> GetSettingsAbilityIcon();

    std::string GetBundleLabel(const std::string& bundleName);

    ErrCode GetBundleResourceInfo(const std::string &bundleName,
        AppExecFwk::BundleResourceInfo &bundleResourceInfo, const int32_t appIndex);
    
    /**
     * @brief Unpacks the string to an image.
     *
     * @param pixelMapStr Indicates the string of image.
     * @return Returns an image object.
     */
    std::shared_ptr<Media::PixelMap> UnPackImage(const std::string &pixelMapStr);

    bool ImageScale(const std::shared_ptr<Media::PixelMap> &pixelMap, int32_t width, int32_t height);
private:
    BundleHelper();
    ~BundleHelper();
    void Connect();
    void Disconnect();
    void OnRemoteDied(const wptr<IRemoteObject> &object);

private:
    sptr<AppExecFwk::IBundleMgr> bundleMgr_ = nullptr;
    ffrt::mutex connectionMutex_;
    sptr<RemoteDeathRecipient> deathRecipient_ = nullptr;
};
}  // namespace FusionConnectivity
}  // namespace OHOS
#endif  // BUNDLE_HELPER_H
