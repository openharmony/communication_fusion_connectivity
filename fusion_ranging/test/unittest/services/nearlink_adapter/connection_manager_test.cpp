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
#define LOG_TAG "ConnectionManagerTest"
#endif

#include <gtest/gtest.h>

#include "connection_manager.cpp"
#include "ranging_manager.h"
#include "fcm_thread_util.h"
#include "fusion_ranging_errorcode.h"
#include "log_utils.h"

using namespace OHOS;
using namespace OHOS::FusionRanging;
using namespace OHOS::Nearlink;
using namespace testing::ext;

class ConnectionManagerTest : public testing::Test {
public:
    ConnectionManagerTest() = default;
    ~ConnectionManagerTest() override = default;

    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void ConnectionManagerTest::SetUpTestCase(void)
{
    OHOS::FusionConnectivity::FcmThreadUtil::GetInstance().InitThreadStateMap();
}

void ConnectionManagerTest::TearDownTestCase(void)
{
    OHOS::FusionConnectivity::FcmThreadUtil::GetInstance().ClearThreadStateMap();
}

void ConnectionManagerTest::SetUp(void) {}

void ConnectionManagerTest::TearDown(void)
{
    ConnectionManager::GetInstance()->pimpl->deviceClients_.Clear();
}

/*
 * Feature: ConnectionManager
 * SubFeature: Singleton Pattern & IsConnected
 * Function: GetInstance / IsConnected
 * Condition: No active connections
 * Test Steps: Call GetInstance twice, call IsConnected for non-existent device
 * Expected Result: Same instance returned; IsConnected returns false
 */
HWTEST_F(ConnectionManagerTest, GetInstanceAndIsConnected_WhenNoDevice_ShouldReturnSingletonAndFalse, TestSize.Level0)
{
    auto *instance1 = ConnectionManager::GetInstance();
    auto *instance2 = ConnectionManager::GetInstance();
    EXPECT_EQ(instance1, instance2);
    EXPECT_NE(instance1, nullptr);

    bool isConnected = instance1->IsConnected("11:22:33:44:55:66");
    EXPECT_FALSE(isConnected);
}

/*
 * Feature: ConnectionManager
 * SubFeature: IsConnected
 * Function: IsConnected
 * Condition: Device exists in deviceClients_ but not connected
 * Test Steps: Insert fake SsapClientInfo into pimpl->deviceClients_, call IsConnected
 * Expected Result: Returns false (isConnected is false, SDK IsAcbConnected also fails)
 */
HWTEST_F(ConnectionManagerTest, IsConnected_WhenDeviceInListButNotConnected_ShouldReturnFalse, TestSize.Level1)
{
    auto *manager = ConnectionManager::GetInstance();
    ASSERT_NE(manager, nullptr);
    ASSERT_NE(manager->pimpl, nullptr);

    std::string deviceId = "AA:BB:CC:DD:EE:FF";
    auto clientInfo = std::make_shared<SsapClientInfo>(nullptr, nullptr);
    clientInfo->SetConnected(false);
    manager->pimpl->deviceClients_.EnsureInsert(deviceId, clientInfo);

    bool isConnected = manager->IsConnected(deviceId);
    EXPECT_FALSE(isConnected);

    manager->pimpl->deviceClients_.Erase(deviceId);
}

/*
 * Feature: ConnectionManager
 * SubFeature: Disconnect
 * Function: Disconnect
 * Condition: Device not in client list
 * Test Steps: Call Disconnect for non-existent device, verify error code
 * Expected Result: Return DEVICE_NOT_INITIATED error
 */
HWTEST_F(ConnectionManagerTest, Disconnect_WhenDeviceNotInList_ShouldReturnNotInitiated, TestSize.Level1)
{
    auto *manager = ConnectionManager::GetInstance();
    ASSERT_NE(manager, nullptr);

    std::string deviceId = "11:22:33:44:55:66";
    int ret = manager->Disconnect(deviceId);
    EXPECT_EQ(ret, RANGING_ERR_DEVICE_NOT_INITIATED);

    ret = manager->Disconnect("AA:BB:CC:DD:EE:FF");
    EXPECT_EQ(ret, RANGING_ERR_DEVICE_NOT_INITIATED);
}

/*
 * Feature: ConnectionManager
 * SubFeature: Disconnect
 * Function: Disconnect
 * Condition: Device exists but not self-initiated
 * Test Steps: Insert SsapClientInfo with isSelfInitiated=false, call Disconnect
 * Expected Result: Returns RANGING_NO_ERROR (early return: "not connect by FusionRanging")
 */
HWTEST_F(ConnectionManagerTest, Disconnect_WhenNotSelfInitiated_ShouldReturnSuccess, TestSize.Level1)
{
    auto *manager = ConnectionManager::GetInstance();
    ASSERT_NE(manager, nullptr);
    ASSERT_NE(manager->pimpl, nullptr);

    std::string deviceId = "AA:BB:CC:DD:EE:FF";
    auto clientInfo = std::make_shared<SsapClientInfo>(nullptr, nullptr);
    clientInfo->SetSelfInitiated(false);
    manager->pimpl->deviceClients_.EnsureInsert(deviceId, clientInfo);

    int ret = manager->Disconnect(deviceId);
    EXPECT_EQ(ret, RANGING_NO_ERROR);

    manager->pimpl->deviceClients_.Erase(deviceId);
}

/*
 * Feature: ConnectionManager
 * SubFeature: Disconnect
 * Function: Disconnect
 * Condition: Device exists, self-initiated, but client_ is null
 * Test Steps: Insert SsapClientInfo with isSelfInitiated=true, client_=nullptr, call Disconnect
 * Expected Result: Returns CONN_MGR_POINTER_NULL (-1)
 */
HWTEST_F(ConnectionManagerTest, Disconnect_WhenSelfInitiatedButNullClient_ShouldReturnError, TestSize.Level1)
{
    auto *manager = ConnectionManager::GetInstance();
    ASSERT_NE(manager, nullptr);
    ASSERT_NE(manager->pimpl, nullptr);

    std::string deviceId = "AA:BB:CC:DD:EE:FF";
    auto clientInfo = std::make_shared<SsapClientInfo>(nullptr, nullptr);
    clientInfo->SetSelfInitiated(true);
    manager->pimpl->deviceClients_.EnsureInsert(deviceId, clientInfo);

    int ret = manager->Disconnect(deviceId);
    EXPECT_EQ(ret, CONN_MGR_POINTER_NULL);

    manager->pimpl->deviceClients_.Erase(deviceId);
}

/*
 * Feature: ConnectionManager
 * SubFeature: Connect / StartPair / ConnectWithSsap
 * Function: Multiple interfaces
 * Condition: Empty device ID or SDK unavailable
 * Test Steps: Call Connect with empty ID, ConnectWithSsap with valid ID, StartPair with empty ID
 * Expected Result: All return error (device creation or SDK call fails)
 */
HWTEST_F(ConnectionManagerTest, ConnectAndPair_WithInvalidConditions_ShouldReturnError, TestSize.Level1)
{
    auto *manager = ConnectionManager::GetInstance();
    ASSERT_NE(manager, nullptr);

    EXPECT_NE(manager->Connect(""), RANGING_NO_ERROR);
    EXPECT_NE(manager->ConnectWithSsap("11:22:33:44:55:66"), RANGING_NO_ERROR);
    EXPECT_NE(manager->StartPair(""), RANGING_NO_ERROR);
}

/*
 * Feature: ConnectionManager
 * SubFeature: OnPairStateChanged
 * Function: OnPairStateChanged
 * Condition: Unknown device and known device with paired state
 * Test Steps: Call OnPairStateChanged with non-existent device (no crash);
 *             Insert SsapClientInfo, call with SLE_PAIR_PAIRED state, verify isPaired_ set
 * Expected Result: Unknown device handled gracefully; known device sets isPaired=true
 */
HWTEST_F(ConnectionManagerTest, OnPairStateChanged_WithUnknownAndPairedState_ShouldHandleGracefully, TestSize.Level1)
{
    auto *manager = ConnectionManager::GetInstance();
    ASSERT_NE(manager, nullptr);
    ASSERT_NE(manager->pimpl, nullptr);

    manager->OnPairStateChanged("11:22:33:44:55:66", 0);
    std::shared_ptr<SsapClientInfo> clientInfoUnused;
    EXPECT_EQ(manager->pimpl->deviceClients_.Find("11:22:33:44:55:66", clientInfoUnused), false);

    std::string deviceId = "AA:BB:CC:DD:EE:FF";
    auto clientInfo = std::make_shared<SsapClientInfo>(nullptr, nullptr);
    manager->pimpl->deviceClients_.EnsureInsert(deviceId, clientInfo);

    manager->OnPairStateChanged(deviceId, static_cast<int32_t>(SlePairState::SLE_PAIR_PAIRED));
    EXPECT_EQ(clientInfo->IsPaired(), true);

    manager->pimpl->deviceClients_.Erase(deviceId);
}

/*
 * Feature: ConnectionManager
 * SubFeature: OnAcbStateChanged / OnSsapConnectionStateChanged
 * Function: Callback handlers
 * Condition: Unknown device
 * Test Steps: Call OnAcbStateChanged and OnSsapConnectionStateChanged with non-existent device
 * Expected Result: Both handle gracefully (FCM_CHECK_RETURN, no crash)
 */
HWTEST_F(ConnectionManagerTest, CallbackHandlers_WithUnknownDevice_ShouldNotCrash, TestSize.Level1)
{
    auto *manager = ConnectionManager::GetInstance();
    ASSERT_NE(manager, nullptr);
    ASSERT_NE(manager->pimpl, nullptr);
    std::string deviceId = "11:22:33:44:55:66";

    manager->OnAcbStateChanged(deviceId, 0);
    EXPECT_EQ(manager->pimpl->deviceClients_.Size(), 0u);

    manager->OnSsapConnectionStateChanged(deviceId, 0, 0);
    EXPECT_EQ(manager->pimpl->deviceClients_.Size(), 0u);
}

/*
 * Feature: ConnectionManager
 * SubFeature: OnSsapConnectionStateChanged
 * Function: OnSsapConnectionStateChanged
 * Condition: Device exists, DISCONNECTED state
 * Test Steps: Insert SsapClientInfo, call OnSsapConnectionStateChanged with DISCONNECTED
 * Expected Result: deviceClients_ entry erased
 */
HWTEST_F(ConnectionManagerTest, OnSsapConnectionStateChanged_WithDisconnected_ShouldEraseDevice, TestSize.Level1)
{
    auto *manager = ConnectionManager::GetInstance();
    ASSERT_NE(manager, nullptr);
    ASSERT_NE(manager->pimpl, nullptr);

    std::string deviceId = "AA:BB:CC:DD:EE:FF";
    auto clientInfo = std::make_shared<SsapClientInfo>(nullptr, nullptr);
    manager->pimpl->deviceClients_.EnsureInsert(deviceId, clientInfo);
    EXPECT_EQ(manager->pimpl->deviceClients_.Size(), 1u);

    manager->OnSsapConnectionStateChanged(deviceId, static_cast<int>(SleConnectState::DISCONNECTED), 0);
    EXPECT_EQ(manager->pimpl->deviceClients_.Size(), 0u);
}

/*
 * Feature: ConnectionManager
 * SubFeature: OnSsapServiceDiscovery
 * Function: OnSsapServiceDiscovery
 * Condition: UUID mismatch with non-existent device
 * Test Steps: Call OnSsapServiceDiscovery with non-ranging UUID
 * Expected Result: Handle gracefully (UUID mismatch, early return, no crash)
 */
HWTEST_F(ConnectionManagerTest, OnSsapServiceDiscovery_WithMismatchedUuid_ShouldNotCrash, TestSize.Level1)
{
    auto *manager = ConnectionManager::GetInstance();
    ASSERT_NE(manager, nullptr);
    ASSERT_NE(manager->pimpl, nullptr);
    std::string deviceId = "11:22:33:44:55:66";

    Nearlink::UUID mismatchedUuid = Nearlink::UUID::FromString("00000000-0000-0000-0000-000000000000");
    manager->OnSsapServiceDiscovery(deviceId, 0, mismatchedUuid);
    EXPECT_EQ(manager->pimpl->deviceClients_.Size(), 0u);
}

/*
 * Feature: ConnectionManager
 * SubFeature: OnSsapServiceDiscovery
 * Function: OnSsapServiceDiscovery
 * Condition: UUID matches RANGING_SERVICE_UUID but device not in deviceClients_
 * Test Steps: Call OnSsapServiceDiscovery with matching UUID for non-existent device
 * Expected Result: Handle gracefully (FCM_CHECK_RETURN on null clientInfo, no crash)
 */
HWTEST_F(ConnectionManagerTest, OnSsapServiceDiscovery_WithMatchingUuidButUnknownDevice_ShouldNotCrash, TestSize.Level1)
{
    auto *manager = ConnectionManager::GetInstance();
    ASSERT_NE(manager, nullptr);
    ASSERT_NE(manager->pimpl, nullptr);
    std::string deviceId = "11:22:33:44:55:66";

    Nearlink::UUID matchingUuid = Nearlink::UUID::FromString(RANGING_SERVICE_UUID);
    manager->OnSsapServiceDiscovery(deviceId, 0, matchingUuid);
    EXPECT_EQ(manager->pimpl->deviceClients_.Size(), 0u);
}

/*
 * Feature: ConnectionManager
 * SubFeature: StartConnectTimeout / StopConnectTimeout (private)
 * Function: Timeout management
 * Condition: No active timeout task
 * Test Steps: Call StartConnectTimeout and StopConnectTimeout for non-existent device
 * Expected Result: No crash (task posted/removed gracefully)
 */
HWTEST_F(ConnectionManagerTest, ConnectTimeout_WithUnknownDevice_ShouldNotCrash, TestSize.Level1)
{
    auto *manager = ConnectionManager::GetInstance();
    ASSERT_NE(manager, nullptr);
    ASSERT_NE(manager->pimpl, nullptr);
    std::string deviceId = "11:22:33:44:55:66";

    manager->StartConnectTimeout(deviceId);
    EXPECT_EQ(manager->pimpl->deviceClients_.Size(), 0u);

    manager->StopConnectTimeout(deviceId);
    EXPECT_EQ(manager->pimpl->deviceClients_.Size(), 0u);
}

/*
 * Feature: ConnectionManager
 * SubFeature: OnSsapConnectionStateChanged
 * Function: OnSsapConnectionStateChanged
 * Condition: Device exists, CONNECTED state with null client
 * Test Steps: Insert SsapClientInfo with null client, call OnSsapConnectionStateChanged with CONNECTED
 * Expected Result: SetConnected(true) called, then FCM_CHECK_RETURN on null client (no crash)
 */
HWTEST_F(ConnectionManagerTest, OnSsapConnectionStateChanged_WithConnectedAndNullClient_ShouldSetConnected,
         TestSize.Level1)
{
    auto *manager = ConnectionManager::GetInstance();
    ASSERT_NE(manager, nullptr);
    ASSERT_NE(manager->pimpl, nullptr);

    std::string deviceId = "AA:BB:CC:DD:EE:FF";
    auto clientInfo = std::make_shared<SsapClientInfo>(nullptr, nullptr);
    clientInfo->SetConnected(false);
    manager->pimpl->deviceClients_.EnsureInsert(deviceId, clientInfo);

    manager->OnSsapConnectionStateChanged(deviceId, static_cast<int>(SleConnectState::CONNECTED), 0);
    EXPECT_EQ(clientInfo->IsConnected(), true);

    manager->pimpl->deviceClients_.Erase(deviceId);
}

/*
 * Feature: ConnectionManager
 * SubFeature: OnAcbStateChanged
 * Function: OnAcbStateChanged
 * Condition: Device exists, ENCRYPTED state with null client
 * Test Steps: Insert SsapClientInfo with null client, call OnAcbStateChanged with ENCRYPTED
 * Expected Result: SetConnected(true) and SetPaired(true) called, then FCM_CHECK_RETURN on null client (no crash)
 */
HWTEST_F(ConnectionManagerTest, OnAcbStateChanged_WithEncryptedAndNullClient_ShouldSetConnectedAndPaired,
         TestSize.Level1)
{
    auto *manager = ConnectionManager::GetInstance();
    ASSERT_NE(manager, nullptr);
    ASSERT_NE(manager->pimpl, nullptr);

    std::string deviceId = "AA:BB:CC:DD:EE:FF";
    auto clientInfo = std::make_shared<SsapClientInfo>(nullptr, nullptr);
    clientInfo->SetConnected(false);
    clientInfo->SetPaired(false);
    manager->pimpl->deviceClients_.EnsureInsert(deviceId, clientInfo);

    manager->OnAcbStateChanged(deviceId, static_cast<int>(SleConnState::SLE_CONNECTION_STATE_ENCRYPTED));
    EXPECT_EQ(clientInfo->IsConnected(), true);
    EXPECT_EQ(clientInfo->IsPaired(), true);

    manager->pimpl->deviceClients_.Erase(deviceId);
}