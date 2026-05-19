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

#include "fusionranging_fuzzer.h"

#include <thread>
#include <securec.h>
#include "fusion_ranging_server.h"
#include "fusion_ranging_service.h"
#include "ranging_params.h"
#include "ranging_result.h"
#include "ranging_capability_supported.h"
#include "fusion_ranging_types.h"
#include "ranging_observer_stub.h"

using namespace std;
using namespace OHOS::FusionRanging;
namespace OHOS {

namespace {
constexpr int FUZZ_DELAY_100_MS = 100;
constexpr int MAX_DATA_LEN = 1000;
constexpr size_t MAX_DEVICE_ID_LEN = 64;
constexpr size_t FUZZ_HANDLE_OFFSET = 0;
constexpr size_t FUZZ_TYPE_OFFSET = 4;
}  // namespace

class MockRangingObserverForFuzz : public IRangingObserver {
public:
    ErrCode OnRangingResult(const RangingResult &result) override
    {
        return 0;
    }

    ErrCode OnRangingStateChanged(const RangingStateChangeInfo &info) override
    {
        return 0;
    }

    sptr<IRemoteObject> AsObject() override
    {
        return nullptr;
    }
};

static sptr<MockRangingObserverForFuzz> g_mockObserver = nullptr;

static sptr<MockRangingObserverForFuzz> GetOrCreateObserver()
{
    if (g_mockObserver == nullptr) {
        g_mockObserver = new MockRangingObserverForFuzz();
    }
    return g_mockObserver;
}

static std::string ExtractDeviceId(const uint8_t *fuzzData, size_t size)
{
    size_t len = std::min(size, MAX_DEVICE_ID_LEN);
    return std::string(reinterpret_cast<const char *>(fuzzData), len);
}

static int32_t ExtractHandle(const uint8_t *fuzzData, size_t size)
{
    if (size < FUZZ_HANDLE_OFFSET + sizeof(int32_t)) {
        return 0;
    }
    int32_t handle = 0;
    (void)memcpy_s(&handle, sizeof(handle), fuzzData + FUZZ_HANDLE_OFFSET, sizeof(handle));
    return handle;
}

static int32_t ExtractCapabilityType(const uint8_t *fuzzData, size_t size)
{
    if (size < FUZZ_TYPE_OFFSET + sizeof(int32_t)) {
        return static_cast<int32_t>(RangingTypes::NEARLINK_HADM);
    }
    int32_t type = 0;
    (void)memcpy_s(&type, sizeof(type), fuzzData + FUZZ_TYPE_OFFSET, sizeof(type));
    return type;
}

void DoGetRangingCapabilityFuzzTest(const uint8_t *fuzzData, size_t size)
{
    if (fuzzData == nullptr || size == 0) {
        return;
    }

    auto server = FusionRangingServer::GetInstance();
    if (server == nullptr) {
        return;
    }

    RangingCapabilitySupported capability;
    server->GetRangingCapability(capability);
}

void DoStartRangingFuzzTest(const uint8_t *fuzzData, size_t size)
{
    if (fuzzData == nullptr || size == 0 || size > MAX_DATA_LEN) {
        return;
    }

    auto server = FusionRangingServer::GetInstance();
    if (server == nullptr) {
        return;
    }

    server->RegisterObserver(GetOrCreateObserver());
    std::string deviceId = ExtractDeviceId(fuzzData, size);
    RangingParams params(deviceId, RangingTypes::NEARLINK_HADM);
    server->StartRanging(params);
}

void DoStopRangingFuzzTest(const uint8_t *fuzzData, size_t size)
{
    if (fuzzData == nullptr || size == 0 || size > MAX_DATA_LEN) {
        return;
    }

    auto server = FusionRangingServer::GetInstance();
    if (server == nullptr) {
        return;
    }

    std::string deviceId = ExtractDeviceId(fuzzData, size);
    RangingParams params(deviceId, RangingTypes::NEARLINK_HADM);
    server->StopRanging(params);
}

void DoStartPassiveRangingFuzzTest(const uint8_t *fuzzData, size_t size)
{
    if (fuzzData == nullptr || size == 0 || size > MAX_DATA_LEN) {
        return;
    }

    auto server = FusionRangingServer::GetInstance();
    if (server == nullptr) {
        return;
    }

    server->RegisterObserver(GetOrCreateObserver());
    int32_t handle = ExtractHandle(fuzzData, size);
    int32_t capType = ExtractCapabilityType(fuzzData, size);
    server->StartPassiveRanging(capType, handle);
}

void DoStopPassiveRangingFuzzTest(const uint8_t *fuzzData, size_t size)
{
    if (fuzzData == nullptr || size == 0 || size > MAX_DATA_LEN) {
        return;
    }

    auto server = FusionRangingServer::GetInstance();
    if (server == nullptr) {
        return;
    }

    int32_t handle = ExtractHandle(fuzzData, size);
    int32_t capType = ExtractCapabilityType(fuzzData, size);
    server->StopPassiveRanging(capType, handle);
}

void DoHandleProcessDeathFuzzTest(const uint8_t *fuzzData, size_t size)
{
    if (fuzzData == nullptr || size == 0) {
        return;
    }

    auto server = FusionRangingServer::GetInstance();
    if (server == nullptr) {
        return;
    }

    int32_t uid = 0;
    if (size >= sizeof(int32_t)) {
        (void)memcpy_s(&uid, sizeof(uid), fuzzData, sizeof(uid));
    }
    server->HandleProcessDeath(uid);
}

}  // namespace OHOS

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;

    auto server = FusionRangingServer::GetInstance();
    if (server == nullptr) {
        return 0;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::FUZZ_DELAY_100_MS));
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }

    OHOS::DoGetRangingCapabilityFuzzTest(data, size);
    OHOS::DoStartRangingFuzzTest(data, size);
    OHOS::DoStopRangingFuzzTest(data, size);
    OHOS::DoStartPassiveRangingFuzzTest(data, size);
    OHOS::DoStopPassiveRangingFuzzTest(data, size);
    OHOS::DoHandleProcessDeathFuzzTest(data, size);

    return 0;
}