# YiChenGameplay 模块技术文档

## 1. 概述

`YiChenGameplay` 模块是 `YiChenGameCore` 插件的“心脏”和“大脑”。它负责将所有其他模块（如 `Ability`, `Equipment`, `Inventory`）的功能组织在一起，形成一个完整、连贯的游戏框架。该模块深度整合了 Unreal Engine 的 **Modular Gameplay** 和 **Game Features** 插件，提供了一套以“游戏体验”（Experience）为核心的、高度数据驱动的游戏流程管理方案。

## 2. 核心设计

- **体验驱动 (Experience Driven)**: 游戏的核心玩法不再硬编码于 `AGameMode` 中，而是被抽象为 `UYcExperienceDefinition` 数据资产。一个“体验”定义了该玩法所需加载的 `Game Feature` 插件、默认的玩家类型 (`UYcPawnData`)、以及一系列在体验生命周期中执行的动作 (`UGameFeatureAction`)。

- **模块化 Actor**: 核心的 `AActor` 类，如 `AYcCharacter`, `AYcPlayerState`, `AYcGameMode`，都继承自 Epic 的模块化基类（如 `AModularCharacter`）。这使得开发者可以通过向这些 Actor 添加组件（Component）和 `Game Feature` Action 来扩展其功能，而无需修改基类代码。

- **GAS 驱动的交互**: 游戏世界中的交互行为（如拾取物品、打开门）被实现为一套基于 GAS 的系统。可交互对象提供“交互选项”（`FYcInteractionOption`），而交互的发起者（通常是玩家）通过一个异步的 `GameplayAbilityTask` 来扫描并呈现这些选项。

## 3. 关键类与系统详解

### 3.1. 游戏体验 (Experience) 系统

这是 `YiChenGameplay` 模块的基石，扩展了传统的、固化的 `GameMode` 驱动模式。

- **`UYcExperienceDefinition`**: 一个 `UPrimaryDataAsset`，定义了一场游戏的全部内容。
  
  - `GameFeaturesToEnable`: 指定该体验需要激活哪些 `Game Feature` 插件。
  - `DefaultPawnData`: 指定玩家默认的 `Pawn` 类型。`UYcPawnData` 是另一个数据资产，定义了 `Pawn` 的类、要授予的技能、输入配置等。
  - `Actions`: 一个 `UGameFeatureAction` 数组，这是体验的核心。当体验被加载时，这个数组中的所有 Action 都会被执行。例如，`UGameFeatureAction_AddWidgets` 可以为该体验添加特定的 UI 布局，`UGameFeatureAction_AddInputBinding` 可以添加特定的输入映射。

- **`AYcGameMode`**: 其核心职责简化为在游戏开始时，根据URL参数或世界设置，找到并加载对应的 `UYcExperienceDefinition`。一旦体验加载完成，`AYcGameMode` 会使用体验中定义的 `DefaultPawnData` 来生成玩家的 `Pawn`。

### 3.2. 核心 Gameplay Actor

- **`AYcPlayerState`**: 继承自 `AModularPlayerState`，是玩家的“逻辑”代表。
  
  - **ASC 持有者**: 最重要的设计决策是，`UYcAbilitySystemComponent` (ASC) 被放在了 `AYcPlayerState` 上，而不是 `AYcCharacter` 上。这确保了玩家的核心能力和状态（如等级、技能、核心Buff）在 `Pawn` 死亡和重生后依然能够保留。
  - `PawnData`: 持有对当前 `UYcPawnData` 的引用，并在数据变化时（`OnRep_PawnData`）负责授予或移除 `PawnData` 中定义的技能集。

- **`AYcCharacter`**: 继承自 `AModularCharacter`，是玩家的“物理”代表。
  
  - **ASC 代理**: 它本身不持有 ASC，而是通过 `GetYcPlayerState()->GetYcAbilitySystemComponent()` 来获取其 ASC 的引用。它实现了 `IAbilitySystemInterface` 接口，并将所有调用转发给 `PlayerState` 的 ASC。
  - **Pawn 扩展组件 (`UYcPawnExtensionComponent`)**: 负责在 `Pawn` 上响应和处理来自 `PlayerState` 的 `PawnData` 设置，例如初始化 ASC 的 `ActorInfo`，并将输入绑定到 `EnhancedInputComponent`。

- **`AYcPlayerController`**: 继承自 `ACommonPlayerController`。
  
  - **输入处理**: 在 `PostProcessInput` 中，它负责将 `EnhancedInput` 系统触发的 `GameplayTag` 传递给 `PlayerState` 上的 ASC，从而驱动技能的激活。

### 3.3. 交互系统

- **`IYcInteractableTarget` (接口)**: 任何希望被交互的 `Actor` 或 `Component` 都必须实现这个接口。
  
  - `GatherInteractionOptions()`: 核心函数。当一个交互发起者（如玩家）扫描到这个对象时，该函数被调用。实现者需要根据传入的查询信息（`FYcInteractionQuery`），决定提供哪些交互选项，并通过 `FYcInteractionOptionBuilder` 将它们添加进去。

- **`UYcInteractableComponent`**: 一个实现了 `IYcInteractableTarget` 接口的 `ActorComponent`，提供了一种将交互能力附加到任意 `Actor` 上的便捷方式。

- **`UAbilityTask_WaitForInteractableTargets`**: 这是一个 `GameplayAbilityTask`，是交互系统的核心驱动力。当一个“扫描交互”的技能被激活时，这个 Task 会被创建。它负责周期性地从玩家的视角进行扫描（如射线检测），找到所有实现了 `IYcInteractableTarget` 接口的对象，调用它们的 `GatherInteractionOptions`，并将最终的交互选项列表呈现给玩家（例如，通过UI显示）。

## 4. 使用指南

1. **创建 PawnData**: 创建一个继承自 `UYcPawnData` 的数据资产，在其中配置好你的玩家 `Pawn` 类、初始技能、输入配置等。
2. **创建体验**: 创建一个 `UYcExperienceDefinition` 数据资产。在其中指定要启用的 `Game Feature`，并设置 `DefaultPawnData` 为上一步创建的资产。
3. **配置世界**: 在你的地图的世界设置（World Settings）中，将 `Default Gameplay Experience` 设置为你创建的体验资产。
4. **设置 GameMode**: 确保世界设置中的 `GameMode` 设置为 `AYcGameMode` 或其子类。
5. **实现交互**: 要使一个 `Actor` 可交互，只需为其添加 `UYcInteractableComponent`，或让其直接实现 `IYcInteractableTarget` 接口，并实现 `GatherInteractionOptions` 函数来提供交互选项。
6. **触发交互**: 创建一个 `GameplayAbility`，在其中创建一个 `UAbilityTask_WaitForInteractableTargets` 任务来扫描可交互对象，并根据任务的输出（如 `OnInteractableObjectsFound`）来更新UI或执行交互逻辑。

## 5. 改进建议

- **1. 玩家初始化流程的可视化**
  
  - **建议**: 当前的玩家初始化流程（`Experience` 加载 -> `GameMode` 生成 `Pawn` -> `PawnExtensionComponent` 初始化）虽然功能强大，但对于新用户来说可能不够直观。可以考虑创建一个自定义的编辑器窗口或调试工具，用于可视化显示当前玩家的初始化状态、`PawnData` 的来源、以及已执行的 `Game Feature` Action，以帮助开发者快速定位问题。

- **2. 交互系统的性能优化**
  
  - **建议**: 当前的交互系统依赖于周期性的扫描。对于拥有大量可交互对象的场景，这可能会成为性能瓶颈。可以探索更高效的方案，例如基于碰撞体积的触发系统（当玩家进入/离开一个“交互区域”时注册/反注册可交互对象），或者使用空间划分数据结构（如八叉树）来加速扫描过程。

- **3. 机器人/AI 的支持**
  
  - **建议**: 当前的框架主要围绕 `AYcPlayerController` 和 `AYcPlayerState` 设计。可以进一步扩展，提供一套并行的 `AYcAIController` 和 `AYcAIState` 基类，并确保 `Experience` 和 `PawnData` 系统能够无缝地支持AI角色的生成和初始化。
