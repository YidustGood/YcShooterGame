# YiChenGameCore (核心) 模块技术文档

## 1. 概述

`YiChenGameCore` 模块是整个 `YiChenGameCore` 插件的基石。它不包含任何高层级的游戏逻辑，而是提供了一系列通用的、底层的核心功能、数据结构和基类，作为所有其他模块的依赖基础。该模块的核心职责是定义一套稳固、高效且网络优化的基础框架，让上层模块（如 `Inventory`, `Equipment`, `Ability`）可以专注于实现具体的游戏功能。

## 2. 核心功能

- **可复制UObject (`UYcReplicableObject`)**: 提供了一个 `UObject` 的基类，使其能够作为 Actor 的子对象被高效地网络复制。
- **GameplayTag 堆叠容器 (`FYcGameplayTagStackContainer`)**: 一个基于 `FFastArraySerializer` 的高性能数据结构，用于网络同步 `GameplayTag` 的堆叠数量。
- **原生 GameplayTag 定义 (`YcGameplayTags`)**: 集中定义了整个插件使用的核心 `GameplayTag`，避免了“字符串式”标签，提高了代码的安全性和可维护性。
- **游戏消息系统 (`GameplayMessage`)**: 提供了轻量级的消息收发功能，用于模块间和UI的解耦通信。

## 3. 关键类与系统详解

### 3.1. UYcReplicableObject

在 Unreal Engine 中，默认只有 Actor 及其组件（ActorComponent）支持直接的网络复制。然而，在许多游戏系统（如库存、装备）中，使用 `UObject` 来表示实例（如 `UYcInventoryItemInstance`）会比使用 Actor 更轻量、更高效。`UYcReplicableObject` 就是为了解决 `UObject` 网络复制的问题而创建的基类。

- **实现原理**: 它利用了 Unreal Engine 提供的 `Replicated Subobjects` 机制。一个 `UYcReplicableObject` 实例必须被一个 Actor（通常是 `PlayerState` 或 `Character`）所持有，并通过 `AddReplicatedSubObject()` 方法注册到该 Actor 的复制列表中。这样，当 Actor 进行网络复制时，引擎会自动同步这个 `UObject` 的 `UPROPERTY(Replicated)` 属性。

- **核心功能**:
  - `IsSupportedForNetworking()`: 返回 `true`，声明该对象支持网络复制。
  - `GetActorOuter()`: 获取持有该对象的外部 Actor，这是调用RPC和进行网络判断的基础。
  - `CallRemoteFunction()`: 重写了该函数，使得在该 `UObject` 上定义的 RPC（如 `UFUNCTION(Client, Reliable)`）可以通过其外部 Actor 的网络通道正确地发送。
  - `Destroy()`: 提供了安全的销毁方法，确保只有服务器可以销毁网络复制的对象，并通过 `NetMulticast` 通知所有客户端。

### 3.2. FYcGameplayTagStackContainer

这是一个高度优化的、用于网络同步 `GameplayTag` 计数的容器。在需要追踪资源数量、Buff/Debuff层数等场景下非常有用。

- **实现原理**: 该结构体继承自 `FFastArraySerializer`，这是一个引擎提供的、用于高效同步数组变化的系统。它只同步数组的增、删、改，而不是每次都同步整个数组，极大地节省了网络带宽。
  - **`Stacks`**: 一个 `TArray<FYcGameplayTagStack>`，是实际进行网络复制的数据。
  - **`TagToCountMap`**: 一个 `TMap<FGameplayTag, int32>`，它**不进行网络复制**。它在服务器端和客户端本地都存在，作为一个快速查询的缓存。它的数据一致性是通过在 `FFastArraySerializer` 的回调函数（`PostReplicatedAdd`, `PreReplicatedRemove` 等）中手动更新来保证的。

- **核心接口**:
  - `AddStack(Tag, Count)`: 增加一个 Tag 的计数值。
  - `RemoveStack(Tag, Count)`: 减少一个 Tag 的计数值。
  - `GetStackCount(Tag)`: O(1) 的时间复杂度快速获取一个 Tag 的计数值。

### 3.3. YcGameplayTags

这是一个 `namespace`，通过 `UE_DECLARE_GAMEPLAY_TAG_EXTERN` 宏定义了插件中所有硬编码的 `GameplayTag`。

- **目的**: 将所有重要的、代码逻辑依赖的 `GameplayTag` 集中管理，有以下好处：
  - **避免拼写错误**: 在代码中使用 `YcGameplayTags::Status_Death` 而不是 `FGameplayTag::RequestGameplayTag(FName("Status.Death"))`，可以在编译期就发现错误。
  - **便于查找和重构**: 所有核心标签都定义在一个地方，方便查找引用和进行重命名。
  - **性能**: Native Gameplay Tag 的查找速度比动态请求的 Tag 更快。

- **内容**: 定义了输入、角色状态、游戏事件等多种类型的核心标签。

## 4. 使用指南

- **创建可复制对象**: 当你需要一个可被网络复制的、非Actor/Component的运行时对象时，让它继承自 `UYcReplicableObject`。确保它的创建和销毁由服务器控制，并且其 Outer 是一个可复制的 Actor。

- **管理Tag计数**: 当你需要在 Actor 或组件中同步一组带计数的 Tag 时，声明一个 `UPROPERTY(Replicated)` 的 `FYcGameplayTagStackContainer` 成员变量。所有对该容器的修改都应通过 `AddStack` 和 `RemoveStack` 进行，以确保网络同步的正确性。

- **使用核心Tag**: 在C++代码中，总是通过 `YcGameplayTags::` 来访问插件定义的核心标签，以保证代码的健壮性。

## 5. 改进建议

- **1. 扩展 `UYcReplicableObject` 功能**
  - **建议**: 可以为 `UYcReplicableObject` 增加更多的网络便利功能，例如一个内建的 `GameplayMessage` 发送器，使其能够方便地向全游戏或其Owner广播消息。此外，可以考虑增加一个 `bOnlyRelevantToOwner` 的复制选项，用于优化那些只需要同步给所属玩家的子对象（如仅本地玩家可见的UI数据对象）。

- **2. 提供 `FYcGameplayTagStackContainer` 的蓝图接口**
  - **建议**: 目前 `FYcGameplayTagStackContainer` 主要在C++中使用。可以创建一个 `UBlueprintFunctionLibrary`，提供一系列静态函数来操作 `FYcGameplayTagStackContainer`（通过传递结构体引用），例如 `AddStackToContainer`、`GetStackCountFromContainer` 等。这将使得蓝图开发者也能利用这个高效的数据结构。

- **3. 完善 `GameplayMessage` 系统**
  - **建议**: 当前的 `GameplayMessage` 系统虽然轻量，但可以进一步增强。可以考虑增加对带负载消息（Message with Payload）的更强类型支持，例如通过模板或 `TInstancedStruct` 来定义消息体，从而在保证解耦的同时，提供更好的类型安全和数据传递能力。
