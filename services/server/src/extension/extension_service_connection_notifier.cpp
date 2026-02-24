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
#define LOG_TAG "ExtensionServiceConnectionNotifier"
#endif

#include "bundle_helper.h"
#include "extension_service_connection_notifier.h"
#include "want_agent_info.h"
#include "want_agent_helper.h"

static int32_t g_uid = 7037;
static const std::string SETTING_BUNDLE_NAME = FUSION_CONNECTIVITY_SETTINGS_BUNDLE_NAME;
static const std::string SETTING_BUNDLE_NAME_ABILITY = FUSION_CONNECTIVITY_SETTINGS_MAIN_ABILITY;
static const std::string BUNDLE_NAME_KEY = "EntryAbility";
static const std::string URI = "fusion_bonded_entry";

namespace OHOS {
namespace FusionConnectivity {
static std::string GetNotifierText(NotificationType type)
{
    HILOGI("notificationType: %{public}d", type);
    const std::map<NotificationType, std::string> notifierTextMap {
        { NotificationType::TELEPHONY_CONTROL_ONLY,           "可进行通话控制，获取相应信息，应用后台运行将增加耗电。" },
        { NotificationType::MEDIA_CONTROL_ONLY,               "可进行媒体控制，获取相应信息，应用后台运行将增加耗电。" },
        { NotificationType::MEDIA_AND_TELEPHONY_CONTROL,      "可进行通话、媒体控制，获取相应信息，应用后台运行将增加耗电。" },
    };

    auto it = notifierTextMap.find(type);
    if (it == notifierTextMap.end()) {
        HILOGE("Not find notificationType: %{public}d", type);
        return "Unknown";
    }

    return it->second;
}

ExtensionServiceConnectionNotifier& ExtensionServiceConnectionNotifier::GetInstance()
{
    static ExtensionServiceConnectionNotifier instance;
    return instance;
}

void ExtensionServiceConnectionNotifier::SendNotification(ExtensionSubscriberInfo &subscriberInfo,
    const NotificationType &notificationType, const std::string &notificationAddress, int32_t &notificationId)
{
    std::lock_guard<std::mutex> guard(requestMutex_);
    std::shared_ptr<OHOS::Notification::NotificationNormalContent> normalContent =
            std::make_shared<OHOS::Notification::NotificationNormalContent>();
    std::string label = "";
    BundleHelper::GetInstance().GetPartnerAgentExtensionAbilityLabel(subscriberInfo.bundleName,
        subscriberInfo.userId, label);
    normalContent->SetTitle("与" + label + "应用数据互通中");
    normalContent->SetText(GetNotifierText(notificationType));
    std::shared_ptr<OHOS::Notification::NotificationContent> content =
            std::make_shared<OHOS::Notification::NotificationContent>(normalContent);

    request_.SetNotificationId(notificationId_.fetch_add(1));
    notificationId = request_.GetNotificationId();
    request_.SetContent(content);
    request_.SetCreatorUid(g_uid);
    request_.SetUnremovable(true);
    request_.SetRemoveAllowed(false);
    request_.SetTapDismissed(false);
    SetWantAgent(request_, notificationAddress);
    icon_ = BundleHelper::GetInstance().GetSettingsAbilityIcon();
    if (icon_.has_value()) {
        request_.SetLittleIcon(icon_.value());
    }
    int32_t result = OHOS::Notification::NotificationHelper::PublishNotification(request_);
    HILOGI("PublishNotification result : %{public}d", result);
}

void ExtensionServiceConnectionNotifier::SetWantAgent(OHOS::Notification::NotificationRequest &request,
    const std::string &notificationAddress)
{
    HILOGI("SetWantAgent");
    if (SETTING_BUNDLE_NAME.empty() || notificationAddress.empty()) {
        HILOGI("not have setting uri param show page");
        return;
    }
    auto want = std::make_shared<AAFwk::Want>();
    want->SetElementName(SETTING_BUNDLE_NAME, SETTING_BUNDLE_NAME_ABILITY);
    want->SetUri(URI);
    std::string deviceId = notificationAddress;
    want->SetParam("deviceId", deviceId);
    want->SetParam("deviceMode", static_cast<int32_t>(DeviceMode::BLUETOOTH_MODE));
    std::vector<std::shared_ptr<AAFwk::Want>> wants;
    wants.push_back(want);

    std::vector<AbilityRuntime::WantAgent::WantAgentConstant::Flags> flags;
    flags.push_back(AbilityRuntime::WantAgent::WantAgentConstant::Flags::CONSTANT_FLAG);

    AbilityRuntime::WantAgent::WantAgentInfo wantAgentInfo(
        0, AbilityRuntime::WantAgent::WantAgentConstant::OperationType::START_ABILITY, flags, wants, nullptr);
    auto wantAgent = AbilityRuntime::WantAgent::WantAgentHelper::GetWantAgent(wantAgentInfo);
    request.SetWantAgent(wantAgent);
}

void ExtensionServiceConnectionNotifier::CancelNotification(const int32_t notificationId)
{
    if (notificationId == INVALID_NOTIFICATION_ID) {
        HILOGE("notificationId is invaild");
        return;
    }
    int32_t result = OHOS::Notification::NotificationHelper::CancelNotification(notificationId);
    HILOGI("notificationId %{public}d, result %{public}d", notificationId, result);
}
} // namespace FusionConnectivity
} // namespace OHOS