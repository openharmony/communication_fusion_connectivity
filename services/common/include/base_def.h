/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributd under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef BASE_DEF_H
#define BASE_DEF_H

#ifndef FCM_DISALLOW_COPY
#define FCM_DISALLOW_COPY(TypeName) TypeName(const TypeName &) = delete
#endif

#ifndef FCM_DISALLOW_ASSIGN
#define FCM_DISALLOW_ASSIGN(TypeName) TypeName &operator=(const TypeName &) = delete
#endif

#ifndef FCM_DISALLOW_COPY_AND_ASSIGN
#define FCM_DISALLOW_COPY_AND_ASSIGN(TypeName) \
    FCM_DISALLOW_COPY(TypeName);               \
    FCM_DISALLOW_ASSIGN(TypeName)
#endif

#ifndef DECLARE_IMPL
#define DECLARE_IMPL()    \
    struct impl;          \
    std::unique_ptr<impl> pimpl
#endif

#ifndef FCM_DISALLOW_MOVE_AND_ASSIGN
#define FCM_DISALLOW_MOVE_AND_ASSIGN(TypeName)     \
    TypeName(TypeName &&) noexcept = delete;   \
    TypeName &operator=(TypeName &&) noexcept = delete
#endif

#endif  // BASE_DEF_H
