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

#ifndef NAPI_HA_EVENT_UTILS_TEST_H
#define NAPI_HA_EVENT_UTILS_TEST_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "napi/native_api.h"
#include "napi_ha_event_utils.h"

using namespace OHOS;
using namespace OHOS::FusionConnectivity;
using namespace testing;
using namespace testing::ext;

class NapiHaEventUtilsTest : public testing::Test {
public:
    NapiHaEventUtilsTest() = default;
    ~NapiHaEventUtilsTest() override = default;

    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void NapiHaEventUtilsTest::SetUpTestCase(void)
{}

void NapiHaEventUtilsTest::TearDownTestCase(void)
{}

void NapiHaEventUtilsTest::SetUp()
{}

void NapiHaEventUtilsTest::TearDown()
{}

/**
 * @tc.name: NapiHaEventUtils_WriteErrCode_001
 * @tc.desc: Test WriteErrCode with int32_t error code
 * @tc.type: FUNC
 */
HWTEST_F(NapiHaEventUtilsTest, NapiHaEventUtils_WriteErrCode_001, TestSize.Level1)
{
    HILOGI("NapiHaEventUtils_WriteErrCode_001 enter");
    napi_env env = nullptr;
    std::string apiName = "test.api";
    NapiHaEventUtils haUtils(env, apiName);
    int32_t errCode = 0;
    haUtils.WriteErrCode(errCode);
    HILOGI("NapiHaEventUtils_WriteErrCode_001 end");
}

/**
 * @tc.name: NapiHaEventUtils_WriteErrCode_002
 * @tc.desc: Test WriteErrCode with different error codes
 * @tc.type: FUNC
 */
HWTEST_F(NapiHaEventUtilsTest, NapiHaEventUtils_WriteErrCode_002, TestSize.Level1)
{
    HILOGI("NapiHaEventUtils_WriteErrCode_002 enter");
    napi_env env = nullptr;
    std::string apiName = "test.api.error";
    NapiHaEventUtils haUtils(env, apiName);
    int32_t errCode = FCM_ERR_INVALID_PARAM;
    haUtils.WriteErrCode(errCode);
    HILOGI("NapiHaEventUtils_WriteErrCode_002 end");
}

/**
 * @tc.name: NapiHaEventUtils_WriteErrCode_003
 * @tc.desc: Test static WriteErrCode function
 * @tc.type: FUNC
 */
HWTEST_F(NapiHaEventUtilsTest, NapiHaEventUtils_WriteErrCode_003, TestSize.Level1)
{
    HILOGI("NapiHaEventUtils_WriteErrCode_003 enter");
    napi_env env = nullptr;
    int32_t errCode = FCM_ERR_INTERNAL_ERROR;
    NapiHaEventUtils::WriteErrCode(env, errCode);
    HILOGI("NapiHaEventUtils_WriteErrCode_003 end");
}

/**
 * @tc.name: NapiHaEventUtils_Constructor_001
 * @tc.desc: Test NapiHaEventUtils constructor and destructor
 * @tc.type: FUNC
 */
HWTEST_F(NapiHaEventUtilsTest, NapiHaEventUtils_Constructor_001, TestSize.Level1)
{
    HILOGI("NapiHaEventUtils_Constructor_001 enter");
    napi_env env = nullptr;
    {
        NapiHaEventUtils haUtils(env, "test.constructor");
        haUtils.WriteErrCode(FCM_NO_ERROR);
    }
    HILOGI("NapiHaEventUtils_Constructor_001 end");
}

/**
 * @tc.name: NapiHaEventUtils_ApiName_001
 * @tc.desc: Test NapiHaEventUtils with different API names
 * @tc.type: FUNC
 */
HWTEST_F(NapiHaEventUtilsTest, NapiHaEventUtils_ApiName_001, TestSize.Level1)
{
    HILOGI("NapiHaEventUtils_ApiName_001 enter");
    napi_env env = nullptr;
    std::string apiName = "fusion.BindDevice";
    NapiHaEventUtils haUtils(env, apiName);
    haUtils.WriteErrCode(FCM_NO_ERROR);
    HILOGI("NapiHaEventUtils_ApiName_001 end");
}

/**
 * @tc.name: NapiHaEventUtils_ApiName_002
 * @tc.desc: Test NapiHaEventUtils with extension API name
 * @tc.type: FUNC
 */
HWTEST_F(NapiHaEventUtilsTest, NapiHaEventUtils_ApiName_002, TestSize.Level1)
{
    HILOGI("NapiHaEventUtils_ApiName_002 enter");
    napi_env env = nullptr;
    std::string apiName = "extension.JsOnDeviceDiscovered";
    NapiHaEventUtils haUtils(env, apiName);
    haUtils.WriteErrCode(FCM_NO_ERROR);
    HILOGI("NapiHaEventUtils_ApiName_002 end");
}

/**
 * @tc.name: NapiHaEventUtils_ApiName_003
 * @tc.desc: Test NapiHaEventUtils with empty API name
 * @tc.type: FUNC
 */
HWTEST_F(NapiHaEventUtilsTest, NapiHaEventUtils_ApiName_003, TestSize.Level1)
{
    HILOGI("NapiHaEventUtils_ApiName_003 enter");
    napi_env env = nullptr;
    std::string apiName = "";
    NapiHaEventUtils haUtils(env, apiName);
    haUtils.WriteErrCode(FCM_NO_ERROR);
    HILOGI("NapiHaEventUtils_ApiName_003 end");
}

#endif // NAPI_HA_EVENT_UTILS_TEST_H
