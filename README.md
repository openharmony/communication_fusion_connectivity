# 融合短距服务

## 简介

该仓主要存放融合短距服务的源码信息。融合短距服务为应用提供设备发现与设备下线的通知功能。

### 内容介绍

本模块基于蓝牙通信技术，为应用提供设备发现与设备下线的通知功能，主要功能特性包括：

- 动态监听并发现应用预先注册的蓝牙设备。
- 采用进程拉起机制，当目标设备出现时自动拉起应用的[PartnerAgentExtensionAbility](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-connectivity-kit/js-apis-fusionConnectivity-partnerAgentExtensionAbility.md)进程。
- 采用进程销毁机制，当所有设备下线时自动销毁应用的[PartnerAgentExtensionAbility](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-connectivity-kit/js-apis-fusionConnectivity-partnerAgentExtensionAbility.md)进程。
- 通过[PartnerAgentExtensionAbility](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-connectivity-kit/js-apis-fusionConnectivity-partnerAgentExtensionAbility.md)的接口通知应用发现已注册设备。

### 框架图

![融合短距服务框架原理图](figures/fusionConnectivity-architecture.png)

如上图所示，融合短距服务主要包含设备管理、extension管理、SA动态启停、代理通知四个模块。
- 设备管理模块主要通过接口注册设备，当设备被蓝牙服务发现时，可以通知三方应用的extension进程，确保三方应用可以感知对应设备的数据活动。
- extension管理模块主要负责提供设备发现与设备下线的通知功能，需要应用继承实现。应用模块级配置文件module.json5 中的extensionabilities的type属性应该配置为partnerAgent。
- SA动态启停模块提供本服务的动态启停，防止资源浪费。
- 代理通知模块主要提供发送通知的能力，该通知的生命周期和extension一致，为了给用户通知系统中的extension活动。所有的配置项均支持用户级绑定与持久化存储。
- 有关融合短距服务及其他相关模块的详细介绍可参见[融合短距服务概述](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-connectivity-kit/js-apis-fusionConnectivity-partnerAgent.md)。


## 目录

```
/foundation/communication
├── fusion_connectivity
│   ├── frameworks                                       # 公共代码
│   │   ├── ets                                          # 静态接口的代码实现
│   │   ├── extension                                    # extension拉起的JS接口实现
│   │   ├── inner                                        # 融合短距服务inner接口
│   │   └── napi                                         # 融合短距服务napi接口
│   ├── idl                                              # 接口idl实现
│   ├── interfaces                                       # 融合短距服务interfaces接口定义
│   │   ├── extension                                    # extension接口定义
│   │   └── inner_api                                    # 融合短距服务inner_api接口定义
│   ├── sa_profile                                       # 融合短距服务定义目录
│   ├── services                                         # 融合短距服务代码目录
│   │   ├── common                                       # 融合短距服务公共实现
│   │   ├── etc                                          # 融合短距服务配置目录
│   │   └── server                                       # 融合短距服务
│   ├── test                                             # 接口测试目录
│   │   ├── fuzztest                                     # fuzz测试
│   │   └── unitest                                      # 接口的单元测试
```

## 约束

- 支持手机、PC、平板[PC/2in1/tablet](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/quick-start/module-configuration-file.md#devicetypes标签)产品。


## 编译构建

- 全量编译

    修改build.gn文件后编译命令
    ```
    $ ./build.sh --product-name rk3568 --ccache
    ```
    未修改build.gn文件编译命令
    ```
    $ ./build.sh --product-name rk3568 --ccache --fast-rebuild
    ```

- 单独编译

    ```
    $ ./build.sh --product-name rk3568 --ccache --build-target fusion_connectivity
    ```


## 说明

### 接口说明

融合短距服务提供了[partnerAgent](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-connectivity-kit/js-apis-fusionConnectivity-partnerAgent.md)、[PartnerAgentExtensionAbility](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-connectivity-kit/js-apis-fusionConnectivity-partnerAgentExtensionAbility.md)、[PartnerAgentExtensionContext](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-connectivity-kit/js-apis-fusionConnectivity-partnerAgentExtensionContext.md)三个模块的接口供开发者使用。其中，PartnerAgentExtensionAbility模块负责设备状态通知的ExtensionAbility组件的生命周期管理；selectionExtensionContext模块负责设备状态通知能力的上下文管理；partnerAgent模块用于管理应用注册的设备。


| 名称 | 描述 |
| ---- | ---- |
| bindDevice(deviceAddress: PartnerDeviceAddress, deviceCapability: DeviceCapability, businessCapability: BusinessCapability, partnerAgentExtensionAbilityName: string): Promise\<void\> | 应用注册设备。 |
| unbindDevice(deviceAddress: PartnerDeviceAddress): Promise\<void\> |  应用解注册设备。 |
| onDestroyWithReason(resaon: PartnerAgentExtensionAbilityDestroyReason): Promise\<void\> | 外设互通扩展能力被销毁时触发的方法回调。 |
| onDeviceDiscovered(deviceAddress: PartnerDeviceAddress): Promise\<void\> | 当已注册的设备被发现时，系统会调用此回调方法。 |

### 使用说明

具体使用方法请参考[实现一个extension生命周期管理的扩展能力](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-connectivity-kit/js-apis-fusionConnectivity-partnerAgentExtensionAbility.md)。


## 相关仓

[融合短距服务](https://gitcode.com/openharmony-sig/communication_fusion_connectivity)
