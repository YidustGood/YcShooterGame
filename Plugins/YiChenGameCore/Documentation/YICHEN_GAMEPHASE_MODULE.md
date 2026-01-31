# YiChenGamePhase 模块技术文档

## 1. 概述

`YiChenGamePhase` 模块提供了一套强大而灵活的、基于 **Gameplay Ability System (GAS)** 的游戏流程管理系统。它将游戏的生命周期（如大厅、等待玩家、回合开始、战斗、回合结束、计分板等）抽象为一个个独立的“游戏阶段”，并通过 Gameplay Ability 的激活与结束来驱动状态的转换。这种设计使得游戏流程的管理变得数据驱动、网络同步且高度可扩展。

## 2. 核心设计

- **GAS 即状态机**: 系统的核心思想是将 GAS 用作一个网络化的状态机。每一个游戏阶段都被实现为一个 `UYcGamePhaseAbility`。整个游戏的当前状态由哪个阶段的 Ability 正在 `GameState` 的 `AbilitySystemComponent` 上激活来表示。

- **分层与嵌套阶段**: 系统通过 `GameplayTag` 的层级关系来支持嵌套的游戏阶段。例如，`Game.Playing` 是一个父阶段，而 `Game.Playing.SuddenDeath` 是其子阶段。当启动一个新阶段时，系统会自动结束所有与新阶段互斥的“兄弟”阶段，但会保留其“父”阶段。这允许在保持一个宏观状态（如“游戏中”）的同时，切换内部的子状态（如“正常游戏” -> “突然死亡”）。

- **服务器权威**: 整个游戏阶段的流转完全由服务器控制。`UYcGamePhaseSubsystem` 和 `UYcGamePhaseComponent` 的核心逻辑仅在服务器上运行，并将必要的状态同步给客户端。

## 3. 关键类与系统详解

### 3.1. UYcGamePhaseAbility

这是所有游戏阶段逻辑的基类，继承自 `UYcGameplayAbility`。

- **`GamePhaseTag`**: 每个 `UYcGamePhaseAbility` 都必须定义一个唯一的 `FGameplayTag`，用于标识该阶段。这个 Tag 是整个系统进行状态识别和转换的核心。
- **生命周期**: 当一个 `UYcGamePhaseAbility` 被激活（`ActivateAbility`）或结束（`EndAbility`）时，它会自动通知 `UYcGamePhaseSubsystem`，从而触发全局的阶段变化事件。开发者可以在这些函数中编写该阶段开始和结束时的具体游戏逻辑（如生成玩家、开启/关闭伤害等）。

### 3.2. UYcGamePhaseSubsystem

这是一个 `UWorldSubsystem`，充当了游戏阶段的中央管理器，仅在服务器上存在。

- **核心功能**: 
  - `StartPhase()`: 核心接口，用于启动一个新的游戏阶段。它会获取 `GameState` 的 ASC，并授予、激活指定的 `UYcGamePhaseAbility`。
  - **观察者模式**: 提供了 `RegisterPhaseStartedObserver` 和 `RegisterPhaseEndedObserver` 两个函数，允许游戏中的任何其他系统（如UI、AI、任务系统）监听特定游戏阶段的开始和结束事件，从而实现逻辑的解耦。
  - `IsPhaseActive()`: 查询一个特定的阶段标签当前是否处于激活状态。

### 3.3. UYcGamePhaseComponent

这是一个 `UGamestateComponent`，是游戏阶段流转的“驱动器”。它负责定义一场游戏的具体阶段序列，并按顺序推进它们。

- **`GamePhases`**: 一个 `TArray<FYcGamePhaseDefinition>`，这是该组件的核心配置。开发者需要在这个数组中，按照游戏进行的顺序，定义好每个阶段的 `GamePhaseTag` 和对应的 `UYcGamePhaseAbility` 类。
- **自动启动**: 该组件会自动监听 `Experience` 加载完成的消息，并在加载完成后自动启动 `GamePhases` 数组中的第一个阶段。
- **`StartNextPhase()`**: 提供了一个简单的接口，用于按顺序启动列表中的下一个游戏阶段。这个函数通常由当前阶段的 `UYcGamePhaseAbility` 在满足特定条件（如计时器结束、某队达到分数上限）时调用。
- **网络同步**: 该组件会将当前阶段在数组中的索引 (`CurrentPhaseIndex`) 复制给所有客户端，让客户端能够知晓当前的游戏状态。

## 4. 使用指南

1.  **创建阶段Ability**: 为你的每一个游戏阶段（如等待、战斗、结算）创建一个继承自 `UYcGamePhaseAbility` 的蓝图或C++类。
2.  **配置阶段Tag**: 在每个创建的 `UYcGamePhaseAbility` 中，设置其 `GamePhaseTag` 属性。
3.  **配置GameState**: 确保你的 `GameState` 中添加了 `UYcAbilitySystemComponent` 和 `UYcGamePhaseComponent`。
4.  **定义阶段序列**: 在 `GameState` 的 `UYcGamePhaseComponent` 组件详情中，配置 `GamePhases` 数组，按照顺序填入每个阶段的定义。
5.  **驱动流程**: 游戏开始后，第一个阶段会自动启动。在某个阶段的 `UYcGamePhaseAbility` 蓝图或C++逻辑中，当需要进入下一阶段时，获取 `GameState` 上的 `UYcGamePhaseComponent` 并调用 `StartNextPhase()`。
6.  **监听阶段变化**: 在其他需要根据游戏阶段做出反应的系统中（例如，UI只在“结算”阶段显示），通过 `UYcGamePhaseSubsystem::Get(this)->RegisterPhaseStartedObserver(...)` 来注册回调函数。

## 5. 改进建议

- **1. 观察者句柄返回**
  - **建议**: `Register...Observer` 函数应该返回一个句柄（`FDelegateHandle` 或自定义结构体）。这允许调用者保存该句柄，并在需要时（如对象销毁时）主动取消注册，从而避免潜在的内存泄漏和悬空指针问题，这对于一个需要长期稳定运行的服务器来说非常重要。

- **2. 数据驱动的阶段序列**
  - **建议**: 将 `GamePhases` 数组的配置从 `UYcGamePhaseComponent` 的实例属性中移出，改为由一个独立的 `UDataAsset` 来定义。然后，可以通过 `Game Feature` 或 `Experience` 的配置来指定当前游戏模式应该使用哪个阶段序列资产。这将极大地提高系统的灵活性，使得添加新游戏模式无需修改或创建新的 `GameState` 或 `Component` 子类。

- **3. 客户端状态镜像**
  - **建议**: `UYcGamePhaseSubsystem` 目前只在服务器上运行。可以考虑创建一个轻量级的客户端版本，或者在 `UYcGamePhaseComponent` 中增加更多的复制信息（如当前激活的所有阶段标签）。这将为客户端提供一个更统一、更丰富的API来查询和响应游戏阶段的变化，简化客户端UI和逻辑的编写。
