# YiChenEquipment 模块技术文档

## 1. 概述

`YiChenEquipment` 模块为 `YiChenGameCore` 插件提供了一套完整、高度可扩展的装备系统。该模块构建于 `YiChenInventory` 和 `YiChenAbility` 模块之上，采用了数据驱动的 **Definition/Instance** 设计模式，并通过 **Fragment** 系统实现了强大的功能扩展性。它负责管理角色身上所有可穿戴或持有的物品，处理其视觉表现（生成Actor）、游戏逻辑（授予技能）和生命周期。

## 2. 核心设计

- **Definition/Instance 模式**: 
  - **`FYcEquipmentDefinition` (定义)**: 这是一个在数据表（DataTable）中定义的 `USTRUCT`，用于描述一件装备的静态属性。它包含了装备实例的类型、要生成的Actor、要授予的技能集以及功能片段（Fragments）。
  - **`UYcEquipmentInstance` (实例)**: 这是一个在运行时创建的 `UObject`，代表一件实际装备在角色身上的物品。它负责管理装备的动态状态和生命周期。

- **Fragment 扩展系统**: 
  - `FYcEquipmentFragment` 是一个基类 `USTRUCT`，允许开发者通过组合而非继承的方式，为装备添加任意数据和逻辑。例如，`FEquipmentFragment_WeaponStats` 可以为一件装备添加武器相关的属性（伤害、射速等）。

- **FastArray 高效网络同步**: 
  - `UYcEquipmentManagerComponent` 使用 `FFastArraySerializer` (`FYcEquipmentList`) 来同步已装备的物品列表，确保了在多人网络环境下的高效和稳定。

## 3. 关键类与系统详解

### 3.1. FYcEquipmentDefinition

这是装备系统的“蓝图”。在一个 `DataTable` 中，每一行都是一个 `FYcEquipmentDefinition`，定义了一件装备的所有静态数据。

- **`InstanceType`**: 指定当这件装备被装备时，应该创建哪个 `UYcEquipmentInstance` 的子类。
- **`ActorsToSpawn`**: 一个数组，定义了装备时需要在角色身上生成并附加的Actor（例如武器模型、盔甲部件）。
- **`AbilitySetsToGrant`**: 一个 `UYcAbilitySet` 数组，定义了装备时需要授予给角色的技能、效果和属性集。
- **`Fragments`**: 核心扩展点。这是一个 `TInstancedStruct<FYcEquipmentFragment>` 数组，允许设计师为装备添加任意自定义数据片段，如武器属性、护甲值、消耗品效果等。

### 3.2. UYcEquipmentInstance

装备在运行时的具现化对象。当 `UYcEquipmentManagerComponent` 装备一件物品时，会根据 `FYcEquipmentDefinition` 中的 `InstanceType` 创建一个 `UYcEquipmentInstance` 对象。

- **生命周期**: 提供了 `OnEquipped` 和 `OnUnequipped` 两个核心事件（C++虚函数和蓝图事件），用于处理装备和卸下时的自定义逻辑。
- **Actor 管理**: 负责在装备时生成 `ActorsToSpawn` 中定义的Actor，并在卸下时销毁它们。
- **Fragment 查询**: 提供了 `GetTypedFragment<T>()` 模板函数，允许在C++中高效、类型安全地获取装备定义中的Fragment数据。

### 3.3. UYcEquipmentManagerComponent

这是一个 `PawnComponent`，是角色与装备系统交互的唯一入口。它应该被添加到需要装备功能的 `Pawn` 或 `Character` 上。

- **核心功能**: 
  - `EquipItemByEquipmentDef()`: [仅服务器] 核心的装备函数。它接收一个 `FYcEquipmentDefinition`，然后执行完整的装备流程：创建 `Instance` -> 授予 `AbilitySets` -> 生成 `Actors` -> 调用 `OnEquipped`。
  - `UnequipItem()`: [仅服务器] 核心的卸下函数，执行与装备相反的流程。
- **网络同步**: 内部的 `FYcEquipmentList` 负责将已装备的 `UYcEquipmentInstance` 列表从服务器复制到所有客户端。

### 3.4. UYcQuickBarComponent

这是一个 `ControllerComponent`，用于实现快捷栏功能（如FPS游戏中的武器切换）。

- **功能**: 它维护一个物品槽位列表 (`Slots`)，并追踪当前激活的槽位 (`ActiveSlotIndex`)。
- **交互流程**: 当玩家通过输入（如滚动鼠标滚轮）请求切换快捷栏时，`UYcQuickBarComponent` 会：
  1.  更新 `ActiveSlotIndex`。
  2.  调用 `UnequipItemInSlot()` 来卸下之前激活的物品。
  3.  调用 `EquipItemInSlot()` 来装备新激活的物品。
  4.  它通过查找Pawn身上的 `UYcEquipmentManagerComponent` 来执行实际的装备/卸下操作。

### 3.5. Fragment 系统

- **`FInventoryFragment_Equippable`**: 这是连接 `YiChenInventory` 和 `YiChenEquipment` 模块的桥梁。一个物品要想变得“可装备”，就必须在其 `FYcInventoryItemDefinition` 的 `Fragments` 数组中添加这个片段。该片段本身包含一个 `FYcEquipmentDefinition`，告诉装备系统当这个物品被请求装备时，应该使用哪个装备定义。

- **`FYcEquipmentFragment`**: 这是装备自身的功能扩展片段。插件内置了 `FEquipmentFragment_WeaponStats` (武器属性), `FEquipmentFragment_ArmorStats` (护甲属性) 等示例，开发者可以根据项目需求创建任意多的自定义Fragment。

## 4. 使用指南

1.  **创建物品**: 在 `YiChenInventory` 模块中创建一个物品定义 (`FYcInventoryItemDefinition`)。
2.  **使其可装备**: 在该物品定义的 `Fragments` 数组中，添加一个 `FInventoryFragment_Equippable` 片段。
3.  **定义装备**: 在 `FInventoryFragment_Equippable` 片段中，详细配置 `FYcEquipmentDefinition`，包括要生成的模型、要授予的技能以及自定义的装备Fragment。
4.  **添加组件**: 确保你的 `Character` 上有 `UYcEquipmentManagerComponent`，并且 `PlayerController` 上有 `UYcQuickBarComponent`。
5.  **装备物品**: 将创建的物品实例添加到 `UYcQuickBarComponent` 的槽位中。当玩家激活该槽位时，`UYcQuickBarComponent` 会自动与 `UYcEquipmentManagerComponent` 通信，完成装备流程。
6.  **获取数据**: 在游戏逻辑中（如在 `UYcGameplayAbility` 中），可以通过 `UYcEquipmentInstance` 的 `GetTypedFragment<T>()` 来获取装备的详细数据（如武器伤害）。

## 5. 改进建议

- **1. 可视化 Fragment 编辑器**
  - **建议**: 为 `FYcEquipmentDefinition` 创建一个自定义的编辑器详情面板。这个面板可以更直观地展示和编辑 `Fragments` 数组，例如使用下拉菜单选择Fragment类型，并以内联的方式显示其属性，而不是默认的 `InstancedStruct` 编辑器。

- **2. 装备外观/幻化系统**
  - **建议**: 在当前系统的基础上，可以轻松地扩展出一套外观/幻化系统。可以创建一个新的 `FEquipmentFragment_Cosmetic`，它只定义 `ActorsToSpawn` 而不包含任何游戏逻辑。在装备时，可以优先应用幻化装备的 `ActorsToSpawn`，同时保留基础装备的 `AbilitySets` 和其他逻辑 `Fragments`。

- **3. 双持与多部位装备**
  - **建议**: 扩展 `UYcEquipmentManagerComponent` 以支持更复杂的装备逻辑。例如，可以引入“装备槽位”（Equipment Slot）的概念（如 `MainHand`, `OffHand`, `Head`, `Chest`）。在 `EquipItem` 时，需要指定目标槽位，并处理双持武器占用两个槽位、或大型装备占用多个身体部位等情况。

- **4. 装备属性的动态修改**
  - **建议**: 当前Fragment中的属性是静态的。可以考虑引入一个系统，允许在运行时通过GameplayEffect等方式动态修改装备实例上的属性（例如武器附魔、宝石镶嵌）。这可能需要在 `UYcEquipmentInstance` 中增加一个可复制的属性容器。
