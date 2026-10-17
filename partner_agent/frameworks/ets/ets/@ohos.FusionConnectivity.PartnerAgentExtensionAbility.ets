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

import type PartnerAgentExtensionContext from '@ohos.FusionConnectivity.PartnerAgentExtensionContext';
import partnerAgent from '@ohos.FusionConnectivity.partnerAgent';
import ExtensionAbility from '@ohos.app.ability.ExtensionAbility';

export default class PartnerAgentExtensionAbility extends ExtensionAbility {
  public context: PartnerAgentExtensionContext = {};

  onDestroyWithReason(reason: partnerAgent.PartnerAgentExtensionAbilityDestroyReason): void {
      console.log(' .ETS onDestroyWithReason');
  }

  onDeviceDiscovered(deviceAddress: partnerAgent.PartnerDeviceAddress): void {
      console.log(' .ETS onDeviceDiscovered');
  };
}
