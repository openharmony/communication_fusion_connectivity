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

#ifndef PARTNER_AGENT_EXTENSION_STUB_IMPL_H
#define PARTNER_AGENT_EXTENSION_STUB_IMPL_H

#include <memory>
#include "partner_agent_extension.h"
#include "partner_agent_extension_stub.h"

namespace OHOS {
namespace FusionConnectivity {
class PartnerAgentExtensionStubImpl : public PartnerAgentExtensionStub {
public:
    explicit PartnerAgentExtensionStubImpl(const std::shared_ptr<PartnerAgentExtension>& extension)
        : extension_(extension) {}

    virtual ~PartnerAgentExtensionStubImpl() = default;
    // Before IPC OnRemoteRequestInner, Used for check permission
    int32_t CallbackEnter(uint32_t code) override;
    // After IPC OnRemoteRequestInner
    int32_t CallbackExit([[maybe_unused]] uint32_t code, [[maybe_unused]] int32_t result) override;

    ErrCode OnDeviceDiscovered(const PartnerDeviceAddress& deviceAddress, int32_t& retResult) override;

    ErrCode OnDestroyWithReason(const int32_t reason, int32_t& retResult) override;

private:
    std::weak_ptr<PartnerAgentExtension> extension_;
};
} // namespace EventFwk
} // namespace OHOS
#endif // PARTNER_AGENT_EXTENSION_STUB_IMPL_H
