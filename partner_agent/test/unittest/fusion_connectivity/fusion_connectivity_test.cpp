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

#ifndef LOG_TAG
#define LOG_TAG "FusionConnectivityTest"
#endif

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <thread>
#include "log.h"

using namespace OHOS;
using namespace OHOS::FusionConnectivity;
using namespace testing;
using namespace testing::ext;

class FusionConnectivityTest : public testing::Test {
public:
    FusionConnectivityTest() = default;
    ~FusionConnectivityTest() override = default;

    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void FusionConnectivityTest::SetUpTestCase(void)
{}
void FusionConnectivityTest::TearDownTestCase(void)
{}
void FusionConnectivityTest::SetUp()
{}
void FcmReconnManagerTest::TearDown()
{}

/**
 * @tc.name: fcmReconn_module_test_001
 * @tc.desc: 双关->双开，星闪待回连设备类型为上一次激活，蓝牙待回连设备类型为上一次激活，后激活类型为星闪
 * @tc.type: FUNC
 */
HWTEST_F(FcmReconnManagerTest, fcmReconn_module_test_001, TestSize.Level0)
{
    HILOGI("fcm_reconn_manager_test_001 enter");
    HILOGI("fcm_reconn_manager_test_001 end");
}
