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

#include "fusion_ranging_fuzzer.h"

#include <thread>
#include <securec.h>
#include "fusion_ranging_service.h"
#include "ranging_params.h"
#include "ranging_result.h"
#include "ranging_adapter_factory.h"

using namespace std;
using namespace OHOS::FusionRanging;
namespace OHOS {

namespace {
constexpr int FUZZ_DELAY_100_MS = 100;
constexpr int MAX_DATA_LEN = 1000;
}

void DoStartRangingFuzzTest(const uint8_t *fuzzData, size_t size)
{
    if (fuzzData == nullptr || size == 0 || size > MAX_DATA_LEN) {
        return;
    }

    auto service = FusionRangingService::GetInstance();
    if (service == nullptr) {
        return;
    }

    std::string deviceId(reinterpret_cast<const char*>(fuzzData), 
                         std::min(size, static_cast<size_t>(64)));
    
    RangingParams params(deviceId, RangingRole::ROLE_INITIATOR, RangingTypes::NEARLINK_HADM);
    
    auto callback = [](const RangingResult &result) {};
    
    service->StartRanging(params, callback);
}

void DoStopRangingFuzzTest(const uint8_t *fuzzData, size_t size)
{
    if (fuzzData == nullptr || size == 0 || size > MAX_DATA_LEN) {
        return;
    }

    auto service = FusionRangingService::GetInstance();
    if (service == nullptr) {
        return;
    }

    std::string deviceId(reinterpret_cast<const char*>(fuzzData), 
                         std::min(size, static_cast<size_t>(64)));
    
    service->StopRanging(deviceId);
}

void DoPauseRangingFuzzTest(const uint8_t *fuzzData, size_t size)
{
    if (fuzzData == nullptr || size == 0 || size > MAX_DATA_LEN) {
        return;
    }

    auto service = FusionRangingService::GetInstance();
    if (service == nullptr) {
        return;
    }

    std::string deviceId(reinterpret_cast<const char*>(fuzzData), 
                         std::min(size, static_cast<size_t>(64)));
    
    service->PauseRanging(deviceId);
}

void DoResumeRangingFuzzTest(const uint8_t *fuzzData, size_t size)
{
    if (fuzzData == nullptr || size == 0 || size > MAX_DATA_LEN) {
        return;
    }

    auto service = FusionRangingService::GetInstance();
    if (service == nullptr) {
        return;
    }

    std::string deviceId(reinterpret_cast<const char*>(fuzzData), 
                         std::min(size, static_cast<size_t>(64)));
    
    service->ResumeRanging(deviceId);
}

void DoGetRangingDataFuzzTest(const uint8_t *fuzzData, size_t size)
{
    if (fuzzData == nullptr || size == 0 || size > MAX_DATA_LEN) {
        return;
    }

    auto service = FusionRangingService::GetInstance();
    if (service == nullptr) {
        return;
    }

    std::string deviceId(reinterpret_cast<const char*>(fuzzData), 
                         std::min(size, static_cast<size_t>(64)));
    
    int32_t distance = 0;
    int32_t rssi = 0;
    service->GetRangingData(deviceId, distance, rssi);
}

void DoOnRangingResultFuzzTest(const uint8_t *fuzzData, size_t size)
{
    if (fuzzData == nullptr || size == 0 || size > MAX_DATA_LEN) {
        return;
    }

    auto service = FusionRangingService::GetInstance();
    if (service == nullptr) {
        return;
    }

    std::string deviceId(reinterpret_cast<const char*>(fuzzData), 
                         std::min(size, static_cast<size_t>(64)));
    
    RangingResult result;
    result.SetDeviceId(deviceId);
    result.SetRssi(static_cast<int32_t>(fuzzData[0]));
    
    service->OnRangingResult(result);
}

void DoAdapterFactoryFuzzTest(const uint8_t *fuzzData, size_t size)
{
    if (fuzzData == nullptr || size == 0) {
        return;
    }

    auto &factory = RangingAdapterFactory::Instance();
    
    int typeValue = static_cast<int>(fuzzData[0]);
    RangingTypes type = static_cast<RangingTypes>(typeValue);
    
    factory.IsRangingAdapterSupported(type);
    factory.CreateRangingAdapter(type);
}

}  // namespace OHOS

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    
    auto service = FusionRangingService::GetInstance();
    if (service == nullptr) {
        return 0;
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::FUZZ_DELAY_100_MS));
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }

    OHOS::DoStartRangingFuzzTest(data, size);
    OHOS::DoStopRangingFuzzTest(data, size);
    OHOS::DoPauseRangingFuzzTest(data, size);
    OHOS::DoResumeRangingFuzzTest(data, size);
    OHOS::DoGetRangingDataFuzzTest(data, size);
    OHOS::DoOnRangingResultFuzzTest(data, size);
    OHOS::DoAdapterFactoryFuzzTest(data, size);
    
    return 0;
}