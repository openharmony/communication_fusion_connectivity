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

#include "fusion_connectivity_fuzzer.h"

#include <thread>
#include "securec.h"
#include "partner_device_agent_server.h"

using namespace std;
using namespace OHOS::FusionConnectivity;
namespace OHOS {

namespace {
constexpr int HOST_FUZZ_DELAY_100_MS = 100;
}  // namespace

void EnableFuzzTest(const uint8_t *fuzzData, size_t size)
{
    OHOS::sptr<PartnerDeviceAgentServer> hostServer = PartnerDeviceAgentServer::GetInstance();
    if (hostServer == nullptr) {
        return;
    }
    hostServer->Enable();
}

}  // namespace OHOS

// fuzzer init
extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    OHOS::sptr<PartnerDeviceAgentServer> hostServer = PartnerDeviceAgentServer::GetInstance();
    if (hostServer == nullptr) {
        return 0;
    }
    hostServer->OnStart();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_100_MS));
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr) {
        return 0;
    }
    OHOS::EnableFuzzTest(data, size);
    return 0;
}
