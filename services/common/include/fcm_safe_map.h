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

#ifndef FCM_SAFE_MAP_H
#define FCM_SAFE_MAP_H

#include <map>
#include <mutex>

namespace OHOS {
namespace FusionConnectivity {

/**
 * @brief A thread-safe map.
 */
template <typename K, typename V>
class FcmSafeMap {
public:
    FcmSafeMap() {}
    ~FcmSafeMap() {}

    /**
     * @brief Get the number of elements in the map.
     *
     * @return Returns the number of elements in the map.
     */
    int Size()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return map_.size();
    }

    /**
     * @brief Checks if the map has no elements.
     *
     * @return Returns true if the map is empty, false otherwise.
     */
    bool IsEmpty()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return map_.empty();
    }

    /**
     * @brief Inserts element.

     * @param key The key to be inserted.
     * @param value The value to be inserted.
     * @return Returns true if the insertion succeeded, false otherwise.
     */
    bool Insert(const K& key, const V& value)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto ret = map_.insert(std::pair<K, V>(key, value));
        return ret.second;
    }

    /**
     * @brief Ensure inserts element.
     *
     * @param key The key to be inserted.
     * @param value The value to be inserted.
     * If the key already exists, delete the element and insert a new element.
     */
    void EnsureInsert(const K& key, const V& value)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto ret = map_.insert(std::pair<K, V>(key, value));
        // key is exist.
        if (!ret.second) {
            map_.erase(ret.first);
            map_.insert(std::pair<K, V>(key, value));
        }
    }

    /**
     * @brief Get the value by a key.
     *
     * @param key The key to be found.
     * @param value The value corresponding to the key.
     * You need to define the value and transfer it to this function by reference.
     * @return Returns true if the key is found, otherwise false.
     */
    bool GetValue(const K& key, V& value)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto iter = map_.find(key);
        if (iter != map_.end()) {
            value = iter->second;
            return true;
        }
        return false;
    }

    bool GetValueAndOpt(const K& key, const std::function<void(const K, V&)>& optFunc)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto iter = map_.find(key);
        if (iter != map_.end()) {
            optFunc(iter->first, iter->second);
            return true;
        }
        return false;
    }

    void FindAndRmv(const std::function<bool(const K, V&)>& checkFunc)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (map_.empty()) {
            return;
        }
        auto it = map_.begin();
        while (it != map_.end()) {
            if (checkFunc(it->first, it->second)) {
                // Found the element and remove it.
                map_.erase(it++);
            } else {
                it++;
            }
        }
    }

    /**
     * @brief Erases the element by a key.
     *
     * @param key The key to be deleted.
     */
    void Erase(const K& key)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        map_.erase(key);
    }

    /**
     * @brief Erases all elements from the map.
     */
    void Clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        map_.clear();
        return;
    }

    /**
     * @brief Iterate through the elements in the map.
     *
     * @param func The specific function that performs custom operations on each element.
     */
    void Iterate(const std::function<void(const K, V&)>& callback)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (map_.empty()) {
            return;
        }
        for (auto it = map_.begin(); it != map_.end(); it++) {
            callback(it->first, it->second);
        }
    }

    /**
     * @brief Find element that satisfies the condition in the map.
     *
     * @param func The specific function that performs custom operations on each element.
     * The "func" returns true if an element is found that satisfies the condition, otherwise false.
     * If you need to return element, you can do it in the custom function "func".
     *
     * @return Returns true if an element is found that satisfies the condition, otherwise false.
     */
    bool Find(const std::function<bool(const K, V&)>& checkFunc)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (map_.empty()) {
            return false;
        }
        for (auto it = map_.begin(); it != map_.end(); it++) {
            if (checkFunc(it->first, it->second)) {
                // Found the element meets the condition.
                return true;
            }
        }
        // No element that meets the condition.
        return false;
    }

    /**
     * @brief Check whether the key-value pair exists.
     *
     * @param key The key to be found.
     *
     * @return Returns true if an element is found, false otherwise.
     */
    bool FindIf(const K& key)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto iter = map_.find(key);
        if (iter != map_.end()) {
            return true;
        }
        return false;
    }

private:
    std::mutex mutex_;
    std::map<K, V> map_;
};
} // namespace FusionConnectivity
} // namespace OHOS

#endif // FCM_SAFE_MAP_H