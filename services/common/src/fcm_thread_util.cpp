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

#include "fcm_thread_util.h"

#include "datetime_ex.h"
#include "log.h"
#include "ffrt_inner.h"

const uint64_t DELAY_TIME_MS_MAX = 0xFFFFFFFF;  // Max delay time is 8 years
const uint64_t MILLISEC_TO_MICROSEC = 1000; // convert ms

int GetFfrtQueueId(void)
{
    return ffrt::get_queue_id();
}

namespace OHOS {
namespace FusionConnectivity {

static void PostTaskToThread(int threadId, const ThreadUtilFunc &func, uint64_t delayTime)
{
    FcmThreadUtil::GetInstance().PostTask(threadId, func, delayTime);
}

void DoInMainThread(const ThreadUtilFunc &func, uint64_t delayTime)
{
    PostTaskToThread(THREAD_ID_MAIN, func, delayTime);
}

// Only for test.
void FcmThreadUtil::ClearThreadStateMap()
{
    threadStateMap_.Clear();
}

// Only for test.
void FcmThreadUtil::InitThreadStateMap()
{
    for (int i = 0; i < THREAD_ID_BUTT; i++) {
        threadStateMap_.EnsureInsert(i, ThreadState::ENABLED);
    }
}

static std::string GetThreadName(int threadId)
{
    const std::map<int, std::string> threadNameMap {
        { THREAD_ID_MAIN,                "fcm_main" },
    };

    auto it = threadNameMap.find(threadId);
    if (it == threadNameMap.end()) {
        HILOGE("Not find threadId: %{public}d", threadId);
        return "Unknown";
    }

    return it->second;
}

struct FcmThreadUtil::impl {
    using DelayTaskKey = std::pair<int, std::string>;
    struct DelayTask {
        std::string name = "";
        std::atomic_bool trigger = false;
        ffrt::task_handle taskHandle = nullptr;
    };
    class TaskQueue {
    public:
        explicit TaskQueue(const char *name) : queue_(name, ffrt::queue_attr().qos(ffrt::qos_user_interactive)) {}
        ~TaskQueue() = default;

        void PostTask(const ThreadUtilFunc &func, uint64_t delayTime, const std::string &name);
        void PostDelayTask(const ThreadUtilFunc &func, uint64_t delayTime, const std::string &name);
        void RemoveTask(const std::string &name);
        int GetQueueId(void);

    private:
        ffrt::queue queue_;
        ffrt::mutex delayTaskVecMutex_ {};
        std::vector<std::shared_ptr<DelayTask>> delayTaskVec_ {};
    };

    impl();
    ~impl() = default;
    std::shared_ptr<TaskQueue> CreateTaskQueue(int threadId);

    FcmSafeMap<int, std::shared_ptr<TaskQueue>> taskQueueMap_ {};
};

FcmThreadUtil::impl::impl()
{}

FcmThreadUtil::FcmThreadUtil() : pimpl(std::make_unique<impl>())
{
    for (int i = 0; i < THREAD_ID_BUTT; i++) {
        threadStateMap_.EnsureInsert(i, ThreadState::ENABLED);
    }
}

FcmThreadUtil::~FcmThreadUtil()
{
    threadStateMap_.Clear();
}

void FcmThreadUtil::impl::TaskQueue::PostDelayTask(const ThreadUtilFunc &func,
    uint64_t delayTime, const std::string &name)
{
    FCM_CHECK_RETURN(delayTime < DELAY_TIME_MS_MAX, "Invalid delaytime(%{public}lu), taskName(%{public}s)",
        delayTime, name.c_str());

    {
        std::lock_guard<ffrt::mutex> lock(delayTaskVecMutex_);
        // Remove the delayed task if it's triggered, or it's a same task.
        for (auto it = delayTaskVec_.begin(); it != delayTaskVec_.end();) {
            if ((*it)->name == name || (*it)->trigger.load()) {
                queue_.cancel((*it)->taskHandle);
                it = delayTaskVec_.erase(it);
            } else {
                it++;
            }
        }
    }

    // Push a new delayed task
    std::shared_ptr<DelayTask> delayTask = std::make_shared<DelayTask>();
    FCM_CHECK_RETURN(delayTask, "delayTask is nullptr");

    ffrt::task_attr taskAttr;
    taskAttr.name(name.c_str()).delay(delayTime * MILLISEC_TO_MICROSEC);
    auto taskFunc = [delayTask, func]() {
        delayTask->trigger = true;
        func();
    };

    auto taskHandle = queue_.submit_h(taskFunc, taskAttr);
    FCM_CHECK_RETURN(taskHandle, "ffrt submit task failed");

    delayTask->name = name;
    delayTask->taskHandle = std::move(taskHandle);

    std::lock_guard<ffrt::mutex> lock(delayTaskVecMutex_);
    delayTaskVec_.push_back(delayTask);
}

void FcmThreadUtil::impl::TaskQueue::PostTask(const ThreadUtilFunc &func,
    uint64_t delayTime, const std::string &name)
{
    if (delayTime > 0) {
        PostDelayTask(func, delayTime, name);
        return;
    }

    queue_.submit(func);
}

void FcmThreadUtil::impl::TaskQueue::RemoveTask(const std::string &name)
{
    std::lock_guard<ffrt::mutex> lock(delayTaskVecMutex_);
    auto it = std::find_if(
        delayTaskVec_.begin(), delayTaskVec_.end(), [&name](auto &task) { return task->name == name; });
    if (it == delayTaskVec_.end()) {
        return;
    }

    queue_.cancel((*it)->taskHandle);
    delayTaskVec_.erase(it);
}

int FcmThreadUtil::impl::TaskQueue::GetQueueId(void)
{
    // Get the ffrt queue id, for debugging
    int id = -1;
    auto handle = queue_.submit_h([&id]() {
        id = ffrt::get_queue_id();
    });
    queue_.wait(handle);
    return id;
}

void FcmThreadUtil::PostTask(int threadId, const ThreadUtilFunc &func, uint64_t delayTime, const std::string &name)
{
    // Check thread state for unit test.
    ThreadState state = ThreadState::DISABLED;
    threadStateMap_.GetValue(threadId, state);
    if (state == ThreadState::NOT_SWITCH_THREAD) {
        func();
    }
    FCM_CHECK_RETURN(state == ThreadState::ENABLED, "threadId %{public}s is no enabled",
        GetThreadName(threadId).c_str());

    std::shared_ptr<impl::TaskQueue> taskQueue = nullptr;
    if (pimpl->taskQueueMap_.GetValue(threadId, taskQueue) && taskQueue != nullptr) {
        taskQueue->PostTask(func, delayTime, name);
        return;
    }
    // If the thread not found, create it.
    taskQueue = pimpl->CreateTaskQueue(threadId);
    // Execute the first task.
    if (taskQueue) {
        taskQueue->PostTask(func, delayTime, name);
    }
}

void FcmThreadUtil::RemoveTask(int threadId, const std::string &name)
{
    std::shared_ptr<impl::TaskQueue> taskQueue = nullptr;
    if (pimpl->taskQueueMap_.GetValue(threadId, taskQueue) && taskQueue != nullptr) {
        taskQueue->RemoveTask(name);
        return;
    }
}

std::shared_ptr<FcmThreadUtil::impl::TaskQueue> FcmThreadUtil::impl::CreateTaskQueue(int threadId)
{
    std::string threadName = GetThreadName(threadId);
    auto taskQueue = std::make_shared<TaskQueue>(threadName.c_str());
    FCM_CHECK_RETURN_RET(taskQueue, nullptr, "Create %{public}s TaskQueue failed", threadName.c_str());

    int queueId = taskQueue->GetQueueId();
    HILOGI("sle_ffrt_queue: queueId(%{public}d),  name(%{public}s)", queueId, threadName.c_str());

    taskQueueMap_.EnsureInsert(threadId, taskQueue);
    return taskQueue;
}

FcmThreadUtil &FcmThreadUtil::GetInstance()
{
    static FcmThreadUtil instance;
    return instance;
}
}  // namespace FusionConnectivity
}  // namespace OHOS