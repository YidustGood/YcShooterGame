// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

/**
 * 修改本列表时，请注意这些信息会随关卡实例被保存
 * 同时需确保 DefaultEngine.ini 中 [/Script/Engine.CollisionProfile] 的配置与此列表保持一致
 **/

// 与实现交互接口的 Actor/Component 进行检测的射线通道
#define Yc_TraceChannel_Interaction					ECC_GameTraceChannel1

// 武器射线：命中物理网格（PhysicsAsset），而非 Capsule
#define Yc_TraceChannel_Weapon						ECC_GameTraceChannel2

// 武器射线：命中角色 Capsule，而非 PhysicsAsset
#define Yc_TraceChannel_Weapon_Capsule				ECC_GameTraceChannel3

// 武器射线：可穿透多个角色，而非首个命中即停止
#define Yc_TraceChannel_Weapon_Multi				ECC_GameTraceChannel4
