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
#include "log_utils.h"

#ifndef LOG_TAG
#define LOG_TAG "FusionRangingObserver"
#endif

namespace OHOS {
namespace FusionRanging {

int32_t RangingObserverStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
                                             MessageOption &option)
{
    std::u16string localDescriptor = GetDescriptor();
    std::u16string remoteDescriptor = data.ReadInterfaceToken();
    if (localDescriptor != remoteDescriptor) {
        return ERR_TRANSACTION_FAILED;
    }
    switch (code) {
        case static_cast<uint32_t>(IRangingObserverIpcCode::COMMAND_ON_RANGING_STATE_CHANGED): {
            std::unique_ptr<RangingStateChangeInfo> stateInfo(data.ReadParcelable<RangingStateChangeInfo>());
            if (!stateInfo) {
                HILOGE("Read [RangingStateChangeInfo] failed!");
                return ERR_INVALID_DATA;
            }
            ErrCode errCode = OnRangingStateChanged(*stateInfo);
            if (!reply.WriteInt32(errCode)) {
                HILOGE("Write Int32 failed!");
                return ERR_INVALID_VALUE;
            }
            return ERR_NONE;
        }
        case static_cast<uint32_t>(IRangingObserverIpcCode::COMMAND_ON_RANGING_RESULT): {
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

int32_t RangingObserverImpl::OnRangingStateChanged(const RangingStateChangeInfo &info)
{
    HILOGI("RangingObserverImpl::OnRangingStateChanged state: %{public}d, cause: %{public}d",
           static_cast<int32_t>(info.GetState()), static_cast<int32_t>(info.GetCause()));
    auto handlerLocal = handler_.promote();
    if (handlerLocal != nullptr) {
        handlerLocal->OnRangingStateChanged(info);
    }
    return ERR_NONE;
}

int32_t RangingObserverImpl::OnRangingResult(const RangingResult &result)
{
    auto handlerLocal = handler_.promote();
    if (handlerLocal != nullptr) {
        handlerLocal->OnRangingResult(result);
    }
    return ERR_NONE;
}
}  // namespace FusionRanging
}  // namespace OHOS