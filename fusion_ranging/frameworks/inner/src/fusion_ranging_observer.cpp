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

#include "fusion_ranging_observer.h"
#include "log_util.h"

#ifndef LOG_TAG
#define LOG_TAG "FusionRangingObserver"
#endif

namespace OHOS {
namespace FusionRanging {

RangingStateObserverImpl::RangingStateObserverImpl() {}

void RangingStateObserverImpl::SetStateCallback(std::function<void(const RangingStateChangeInfo &)> callback)
{
    stateCallback_ = callback;
}

int32_t RangingStateObserverImpl::OnRangingStateChanged(const RangingStateChangeInfo &info)
{
    HILOGI("OnRangingStateChanged state: %{public}d, cause: %{public}d", static_cast<int32_t>(info.GetState()),
           static_cast<int32_t>(info.GetCause()));
    if (stateCallback_) {
        stateCallback_(info);
    }
    return 0;
}

RangingResultObserverImpl::RangingResultObserverImpl() {}

void RangingResultObserverImpl::SetResultCallback(std::function<void(const RangingResult &)> callback)
{
    resultCallback_ = callback;
}

int32_t RangingResultObserverImpl::OnRangingResult(const RangingResult &result)
{
    HILOGI("OnRangingResult deviceId: %{public}s", GET_ENCRYPT_ADDR(result.GetDeviceId()));
    if (resultCallback_) {
        resultCallback_(result);
    }
    return 0;
}

FusionRangingSaStatusChange::FusionRangingSaStatusChange() {}

void FusionRangingSaStatusChange::SetRemoveCallback(std::function<void()> callback)
{
    removeCallback_ = callback;
}

void FusionRangingSaStatusChange::OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId)
{
    HILOGI("SA added, systemAbilityId:%{public}d", systemAbilityId);
}

void FusionRangingSaStatusChange::OnRemoveSystemAbility(int32_t systemAbilityId, const std::string &deviceId)
{
    HILOGI("SA removed, systemAbilityId:%{public}d", systemAbilityId);
    if (removeCallback_) {
        removeCallback_();
    }
}

int32_t RangingStateObserverStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
                                                  MessageOption &option)
{
    std::u16string localDescriptor = GetDescriptor();
    std::u16string remoteDescriptor = data.ReadInterfaceToken();
    if (localDescriptor != remoteDescriptor) {
        return ERR_TRANSACTION_FAILED;
    }
    switch (code) {
        case static_cast<uint32_t>(IRangingStateObserverIpcCode::COMMAND_ON_RANGING_STATE_CHANGED): {
            std::unique_ptr<RangingStateChangeInfo> info(data.ReadParcelable<RangingStateChangeInfo>());
            if (!info) {
                HILOGE("Read [RangingStateChangeInfo] failed!");
                return ERR_INVALID_DATA;
            }
            ErrCode errCode = OnRangingStateChanged(*info);
            if (!reply.WriteInt32(errCode)) {
                HILOGE("Write Int32 failed!");
                return ERR_INVALID_VALUE;
            }
            return ERR_NONE;
        }
        default:
            return IRemoteStub::OnRemoteRequest(code, data, reply, option);
    }
}

int32_t RangingResultObserverStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
                                                   MessageOption &option)
{
    std::u16string localDescriptor = GetDescriptor();
    std::u16string remoteDescriptor = data.ReadInterfaceToken();
    if (localDescriptor != remoteDescriptor) {
        return ERR_TRANSACTION_FAILED;
    }
    switch (code) {
        case static_cast<uint32_t>(IRangingResultObserverIpcCode::COMMAND_ON_RANGING_RESULT): {
            std::unique_ptr<RangingResult> rangingResult(data.ReadParcelable<RangingResult>());
            if (!rangingResult) {
                HILOGE("Read [RangingResult] failed!");
                return ERR_INVALID_DATA;
            }
            ErrCode errCode = OnRangingResult(*rangingResult);
            if (!reply.WriteInt32(errCode)) {
                HILOGE("Write Int32 failed!");
                return ERR_INVALID_VALUE;
            }
            return ERR_NONE;
        }
        default:
            return IRemoteStub::OnRemoteRequest(code, data, reply, option);
    }
}

}  // namespace FusionRanging
}  // namespace OHOS