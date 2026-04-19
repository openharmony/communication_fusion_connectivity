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

#ifndef RANGING_ADAPTER_FACTORY_H
#define RANGING_ADAPTER_FACTORY_H

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "base_ranging_adapter.h"

namespace OHOS {
namespace FusionRanging {

using RangingAdapterInstanceGenerator = std::function<std::shared_ptr<BaseRangingAdapter>()>;
using RangingAdapterSupportChecker = std::function<bool()>;

class RangingAdapterFactory {
public:
    ~RangingAdapterFactory() = default;
    RangingAdapterFactory(const RangingAdapterFactory &) = delete;
    RangingAdapterFactory operator=(const RangingAdapterFactory &) = delete;

    static RangingAdapterFactory &Instance();

    std::shared_ptr<BaseRangingAdapter> CreateRangingAdapter(const RangingTypes type);

    bool IsRangingAdapterSupported(const RangingTypes type);

    template <typename T>
    void RegisterRangingAdapter(const RangingTypes type, const RangingAdapterInstanceGenerator &generator = nullptr)
    {
        RegisterRangingAdapterPriv<T>(type, generator);
    }

    void RegisterChecker(const RangingTypes type, const RangingAdapterSupportChecker &checker = nullptr)
    {
        RegisterCheckerPriv(type, checker);
    }

private:
    RangingAdapterFactory() = default;

    std::shared_ptr<BaseRangingAdapter> CreateRangingAdapterPriv(const RangingTypes type);
    bool IsRangingAdapterSupportedPriv(const RangingTypes type);

    template <typename T>
    void RegisterRangingAdapterPriv(const RangingTypes type, const RangingAdapterInstanceGenerator &generator)
    {
        if (generator == nullptr) {
            auto result = generators_.emplace(type, []() { return std::make_shared<T>(); });
            if (!result.second) {
                result.first->second = generator;
            }
        } else {
            auto result = generators_.emplace(type, generator);
            if (!result.second) {
                result.first->second = generator;
            }
        }
    }

    void RegisterCheckerPriv(const RangingTypes type, const RangingAdapterSupportChecker &checker)
    {
        if (checker == nullptr) {
            auto result = checkers_.emplace(type, []() { return true; });
            if (!result.second) {
                result.first->second = checker;
            }
        } else {
            auto result = checkers_.emplace(type, checker);
            if (!result.second) {
                result.first->second = checker;
            }
        }
    }

    std::unordered_map<RangingTypes, RangingAdapterInstanceGenerator> generators_;
    std::unordered_map<RangingTypes, RangingAdapterSupportChecker> checkers_;
};

template <typename T>
class AutoRegisterRangingAdapter {
public:
    explicit AutoRegisterRangingAdapter(const RangingTypes type)
    {
        RangingAdapterFactory::Instance().RegisterRangingAdapter<T>(type);
        RangingAdapterFactory::Instance().RegisterChecker(type);
    }

    AutoRegisterRangingAdapter(const RangingTypes type, const RangingAdapterInstanceGenerator &generator)
    {
        RangingAdapterFactory::Instance().RegisterRangingAdapter<T>(type, generator);
        RangingAdapterFactory::Instance().RegisterChecker(type);
    }

    AutoRegisterRangingAdapter(const RangingTypes type, const RangingAdapterInstanceGenerator &generator,
                               const RangingAdapterSupportChecker &checker)
    {
        RangingAdapterFactory::Instance().RegisterRangingAdapter<T>(type, generator);
        RangingAdapterFactory::Instance().RegisterChecker(type, checker);
    }

    ~AutoRegisterRangingAdapter() = default;
};
}  // namespace FusionRanging
}  // namespace OHOS
#endif  // RANGING_ADAPTER_FACTORY_H