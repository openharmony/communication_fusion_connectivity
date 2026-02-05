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

#ifndef FCM_THREAD_UTIL_H
#define FCM_THREAD_UTIL_H

#include <functional>
#include "fcm_safe_map.h" // SafeMap
#include <mutex>

namespace OHOS {
namespace FusionConnectivity {

enum ThreadId {
    THREAD_ID_MAIN = 0,
    // please add before this.
    THREAD_ID_BUTT
};

using ThreadUtilFunc = std::function<void(void)>;


void DoInMainThread(const ThreadUtilFunc &func, uint64_t delayTime = 0);

class FcmThreadUtil {
public:
    void PostTask(
        int threadId, const ThreadUtilFunc &func, uint64_t delayTime = 0, const std::string &name = std::string());
    /**
     * Remove a task.
     *
     * @param name Name of the task.
     */
    void RemoveTask(int threadId, const std::string &name);
    void ClearThreadStateMap();
    void InitThreadStateMap();

    static FcmThreadUtil &GetInstance();

private:
    void CheckThreadStateReturn(int threadId, const ThreadUtilFunc &func);
    enum ThreadState : int {
        ENABLED = 0,  // The task function is switched normally.
        DISABLED,  // The task function is not executed.
        NOT_SWITCH_THREAD,  // The task functions is executed in the same thread.
    };
    // threadId <-> thread state
    FcmSafeMap<int, ThreadState> threadStateMap_ {};

    FcmThreadUtil();
    ~FcmThreadUtil();

    struct impl;
    std::unique_ptr<impl> pimpl;
};
}  // namespace FusionConnectivity
}  // namespace OHOS

#endif  // FCM_THREAD_UTIL_H