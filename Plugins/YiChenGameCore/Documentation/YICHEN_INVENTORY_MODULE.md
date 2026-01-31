# YiChenInventory 模块技术文档

## 1. 概述

`YiChenInventory` 模块为 `YiChenGameCore` 插件提供了一套通用的、网络同步的库存系统。它采用了数据驱动的 **Definition/Instance** 设计模式，并通过 **Fragment** 系统实现了强大的功能扩展性。该模块不仅负责追踪玩家拥有的物品及其数量，还为每个物品实例提供了独立的状态管理和网络复制能力，是构建复杂游戏经济和玩家成长系统的基础。

## 2. 核心设计

- **Definition/Instance 模式**: 
  - **`FYcInventoryItemDefinition` (定义)**: 这是一个在数据表（DataTable）中定义的 `USTRUCT`，用于描述一个物品的静态模板。它包含了物品的ID、显示名称、以及最重要的——一个用于功能扩展的 **Fragment** 列表。
  - **`UYcInventoryItemInstance` (实例)**: 这是一个在运行时创建的 `UYcReplicableObject`，代表一个实际存在于玩家背包中的物品。它持有对 `ItemDefinition` 的引用，并管理着该物品实例的动态状态（例如，通过 `FYcGameplayTagStackContainer` 存储的弹药数量）。

- **Fragment 扩展系统**: 
  - `FYcInventoryItemFragment` 是一个基类 `USTRUCT`，允许开发者通过组合而非继承的方式，为物品定义添加任意数据和逻辑。例如，`FInventoryFragment_Equippable` 可以让一个物品变得“可装备”，而 `FInventoryFragment_InitialItemStats` 可以在物品实例创建时为其赋予初始的动态属性。

- **FastArray 高效网络同步**: 
  - `UYcInventoryManagerComponent` 使用 `FFastArraySerializer` (`FYcInventoryItemList`) 来同步库存中的物品列表。这确保了只有发生变化的物品（增、删、改）才会被网络同步，极大地优化了网络性能。

## 3. 关键类与系统详解

### 3.1. UYcInventoryManagerComponent

这是一个 `ActorComponent`，是与库存系统交互的主要入口。它应该被添加到任何需要拥有库存的 Actor 上（通常是 `PlayerState`）。

- **核心功能**: 
  - `AddItem()`: [仅服务器] 核心的添加物品函数。它接收一个 `FDataTableRowHandle`（指向 `ItemDefinition`）和数量，然后执行完整的物品添加流程：检查是否可添加 -> 创建 `Instance` -> 将 `Instance` 添加到 `ItemList` 中。
  - `RemoveItemInstance()`: [仅服务器] 核心的移除物品函数。
  - `ConsumeItemsByDefinition()`: [仅服务器] 按类型消耗物品。
- **网络同步**: 内部的 `FYcInventoryItemList` 负责将库存中的 `UYcInventoryItemInstance` 列表从服务器高效地复制到所有客户端。

### 3.2. FYcInventoryItemDefinition

物品的静态数据模板，通常在 `DataTable` 中进行定义。

- **`ItemId`**: 物品的唯一标识符。
- **`ItemInstanceType`**: 指定当这个物品被添加到库存时，应该创建哪个 `UYcInventoryItemInstance` 的子类。
- **`Fragments`**: 核心扩展点。这是一个 `TArray<TInstancedStruct<FYcInventoryItemFragment>>`，允许设计师为物品添加任意自定义的功能片段。

### 3.3. UYcInventoryItemInstance

物品在运行时的具现化对象，继承自 `UYcReplicableObject`，因此支持作为子对象进行网络复制。

- **状态管理**: 每个 `ItemInstance` 都拥有一个 `FYcGameplayTagStackContainer` (`TagsStack`)。这是一个网络同步的 `GameplayTag` 计数容器，非常适合用来存储和同步物品的动态状态，例如：
  - `Item.Stat.Ammo.Reserve`: 备用弹药量
  - `Item.Stat.Durability`: 当前耐久度
- **`ItemDef` 引用**: 持有对 `FYcInventoryItemDefinition` 的引用，允许在运行时查询物品的静态数据。
- **Fragment 查询**: 提供了 `GetTypedFragment<T>()` 模板函数，允许在C++中高效、类型安全地从其 `ItemDef` 中获取Fragment数据。

### 3.4. Fragment 系统

这是 `YiChenInventory` 模块最强大的功能之一，它使得物品系统变得极其灵活。

- **`FYcInventoryItemFragment`**: 所有物品功能片段的基类。它提供了一个 `OnInstanceCreated` 的虚函数，允许 Fragment 在其所属的物品实例被创建时，执行一次性的初始化逻辑。
- **`FInventoryFragment_InitialItemStats`**: 一个内置的 Fragment 示例。它允许设计师在 `ItemDefinition` 中配置一组初始的 `GameplayTag` 键值对。当物品实例被创建时，`OnInstanceCreated` 函数会将这些键值对添加到 `ItemInstance` 的 `TagsStack` 中，从而实现了物品动态属性的初始化。

## 4. 使用指南

1.  **创建 Fragment**: 如果你需要为物品添加新的静态数据或初始化逻辑，创建一个继承自 `FYcInventoryItemFragment` 的新 `USTRUCT`。
2.  **创建物品定义**: 在 `DataTable` 中创建新的一行，其行结构为 `FYcInventoryItemDefinition`。
3.  **配置物品**: 在数据表中，为你的新物品设置 `ItemId`，选择 `ItemInstanceType`，并最重要的——在 `Fragments` 数组中添加你所需要的功能片段（如 `InitialItemStats`），并配置其属性。
4.  **添加组件**: 确保你的 `PlayerState` 或其他需要背包的 Actor 上添加了 `UYcInventoryManagerComponent`。
5.  **操作物品**: 在服务器端的代码中，通过获取 `UYcInventoryManagerComponent` 的引用，调用 `AddItem`, `RemoveItemInstance` 等函数来管理库存。
6.  **查询物品状态**: 在任何需要读取物品动态状态的地方（客户端或服务器），获取 `UYcInventoryItemInstance` 的引用，并调用 `GetStatTagStackCount` 来查询其 `TagsStack` 中的数据。

## 5. 改进建议

- **1. 库存容量限制**
  - **建议**: 在 `UYcInventoryManagerComponent` 中增加库存容量（按格子或重量）的限制功能。可以添加一个 `MaxSlots` 或 `MaxWeight` 的属性，并在 `CanAddItemDefinition` 事件中进行检查。物品的重量可以由一个新的 `FInventoryFragment_Weight` Fragment 来定义。

- **2. 物品查询优化**
  - **建议**: 为 `UYcInventoryManagerComponent` 增加更多、更高效的物品查询函数。例如，提供一个 `FindFirstItemInstanceByFragment<T>()` 函数，用于快速查找第一个拥有特定 Fragment 类型的物品实例。这在实现“查找钥匙”或“查找任务物品”等逻辑时会非常有用。

- **3. 蓝图友好性增强**
  - **建议**: 为 `FYcInventoryItemDefinition` 和 `UYcInventoryItemInstance` 创建一个 `UBlueprintFunctionLibrary`，提供更多的蓝图可调用函数来安全地访问其内部数据。特别是对于 Fragment 的查询，可以参考 `YcEquipmentLibrary` 中的 `GetEquipmentFragment` (CustomThunk) 实现一个高效的蓝图版本，允许蓝图开发者在不产生额外拷贝的情况下查询 Fragment 数据。

- **4. 物品堆叠与拆分**
  - **建议**: 增加明确的物品堆叠和拆分逻辑。可以在 `FYcInventoryItemDefinition` 中添加一个 `MaxStackCount` 属性。在 `AddItem` 时，系统应优先尝试堆叠到已有的、未满的 `ItemInstance` 上，只有在所有实例都满了之后才创建新的实例。同时，提供 `SplitStack` 和 `MergeStacks` 这样的接口，以支持更复杂的库存管理操作。
