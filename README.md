# communication_fusion_connectivity

1、基于蓝牙通信技术，为应用提供设备回连/断连的通知功能，主要功能特性包括：
  1）动态监听并发现应用预先注册的蓝牙设备（以蓝牙设备地址的方式）；
  2）采用进程拉起机制，当目标设备连接时自动拉起应用的PartnerAgentExtensionAbility进程。
  3）采用进程销毁机制，当目标设备断连时自动销毁应用的PartnerAgentExtensionAbility进程。