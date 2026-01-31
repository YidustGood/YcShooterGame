# YiChenAbility 模块技术文档

## 1. 概述

`YiChenAbility` 模块是 `YiChenGameCore` 插件中负责实现游戏技能和属性系统的核心。它基于 Epic Games 的 **Gameplay Ability System (GAS)** 进行了深度扩展和封装，旨在提供一套更强大、更易用、更适合多人在线 FPS 游戏的功能集。该模块不仅简化了 GAS 的常规用法，还引入了多项创新功能，如数据驱动的输入绑定、技能集合（Ability Sets）、以及复杂的技能标签关系映射。

## 2. 核心功能

- **增强的技能组件 (`UYcAbilitySystemComponent`)**: 模块的核心，提供了输入处理、技能激活分组、网络RPC批处理和蓝图辅助功能。
- **技能集合 (`UYcAbilitySet`)**: 通过数据资产（Data Asset）来组织和批量授予技能、效果和属性集，极大地方便了角色职业、装备和Buff的设计。
- **标签关系映射 (`UYcAbilityTagRelationshipMapping`)**: 将技能间的复杂交互（如打断、克制、前置条件）逻辑从技能本身剥离，通过数据配置实现，提高了系统的灵活性和可维护性。
- **自定义游戏效果上下文 (`FYcGameplayEffectContext`)**: 扩展了标准的 `FGameplayEffectContext`，允许在技能效果的生命周期中传递自定义的游戏逻辑数据（如子弹ID、伤害来源等）。

## 3. 关键类与系统详解

### 3.1. UYcAbilitySystemComponent

这是对原生 `UAbilitySystemComponent` 的核心扩展类，所有需要使用本插件功能的角色都应使用该组件。其主要增强功能如下：

- **输入与技能激活**: 
  - `AbilityInputTagPressed(FGameplayTag)` / `AbilityInputTagReleased(FGameplayTag)`: 这两个函数是输入系统的核心。Enhanced Input 系统将用户的物理输入（如按下“鼠标左键”）映射为一个 `FGameplayTag`。`UYcAbilitySystemComponent` 接收到这个 Tag 后，会查找所有拥有匹配 `InputTag` 的技能，并根据技能的激活策略（`OnInputTriggered` 或 `WhileInputActive`）来处理激活或取消逻辑。
  - `ProcessAbilityInput()`: 该函数应由 `PlayerController` 每帧调用，负责处理所有待处理的输入事件，包括技能的激活、持续施法和释放。

- **技能激活分组 (`EYcAbilityActivationGroup`)**: 
  - 允许将技能划分为不同的组（如 `Independent`, `Exclusive`, `WhileActive`）。
  - `AddAbilityToActivationGroup()` / `RemoveAbilityFromActivationGroup()`: 提供了在技能激活和结束时，动态维护每个组内正在激活的技能数量。
  - `CancelActivationGroupAbilities()`: 可以一键取消某个组内的所有技能，常用于实现“冲刺时不能开镜”等互斥逻辑。

- **网络优化**: 
  - `BatchRPCTryActivateAbility()`: 这是一个高度优化的技能激活函数。对于那些瞬间完成的技能（如单发射击），它可以将 `ActivateAbility`、`SendTargetData` 和 `EndAbility` 三个网络调用合并成一个 RPC，显著降低网络流量。

- **蓝图便利功能**: 
  - 提供了大量 `K2_` 前缀的函数，如 `K2_GetTagCount`、`K2_AddLooseGameplayTag`、`K2_ApplyGameplayEffectToSelfWithPrediction` 等，简化了在蓝图中使用 GAS 的复杂度。

### 3.2. UYcAbilitySet

`UYcAbilitySet` 是一个 `UPrimaryDataAsset`，用于将多个技能、效果和属性集打包成一个可复用的“技能包”。

- **数据结构**: 
  - `GrantedGameplayAbilities`: 定义要授予的技能列表，每个条目包含技能类、等级和可选的输入标签。
  - `GrantedGameplayEffects`: 定义要授予的被动效果列表。
  - `GrantedAttributes`: 定义要授予的属性集列表。

- **授予与移除**: 
  - `GiveToAbilitySystem()`: 将技能集合中定义的所有内容授予给一个 `UYcAbilitySystemComponent`。
  - `FYcAbilitySet_GrantedHandles`: `GiveToAbilitySystem` 会返回一个句柄结构体，其中包含了所有已授予内容的句柄。通过调用句柄的 `RemoveFromAbilitySystem()` 方法，可以干净地移除所有内容，避免了手动追踪和移除的麻烦。

### 3.3. UYcAbilityTagRelationshipMapping

这是一个 `UDataAsset`，用于定义技能标签之间的复杂交互规则，是实现高级技能逻辑的核心。

- **核心关系**: 
  - `AbilityTagsToBlock`: 当一个技能激活时，会**阻止**拥有这些标签的其他技能激活。
  - `AbilityTagsToCancel`: 当一个技能激活时，会**取消**拥有这些标签的、已在运行的其他技能。
  - `ActivationRequiredTags`: 激活一个技能**必须**拥有的前置标签。
  - `ActivationBlockedTags`: 如果拥有这些标签，则**无法**激活该技能。

- **工作流程**: `UYcAbilitySystemComponent` 在尝试激活一个技能时，会查询其持有的 `TagRelationshipMapping` 资产，根据技能的标签来获取所有相关的阻止和依赖关系，并据此判断技能是否能够成功激活。

## 4. 使用指南

1.  **角色设置**: 确保你的角色 `Character` 或 `Pawn` 类中添加了 `UYcAbilitySystemComponent` 组件。
2.  **定义技能**: 创建继承自 `UYcGameplayAbility` 的蓝图或 C++ 类来定义你的技能。在技能的 `ActivationPolicy` 中设置激活方式，并通过 `InputTag` 绑定输入。
3.  **创建技能集合**: 在内容浏览器中创建 `YcAbilitySet` 数据资产，将你的技能、被动效果和属性集配置进去。
4.  **授予技能**: 在合适的时机（如角色出生、装备物品时），调用 `GiveToAbilitySystem()` 将技能集合授予角色。
5.  **配置输入**: 在 `Enhanced Input` 的 `InputMappingContext` 中，将用户的物理输入（如 `W` 键）与一个 `FGameplayTag`（如 `InputTag.Move`）进行映射。
6.  **处理输入**: 在 `PlayerController` 的 `PostProcessInput` 或类似函数中，将触发的 `InputTag` 传递给 `UYcAbilitySystemComponent` 的 `AbilityInputTagPressed` 和 `AbilityInputTagReleased` 函数，并每帧调用 `ProcessAbilityInput`。

## 5. 改进建议

- **1. 技能冷却和消耗的扩展**
  - **建议**: 目前的 `UYcGameplayAbility` 可以进一步扩展，提供更灵活的冷却和消耗（如法力、体力）的蓝图事件和接口。例如，增加 `OnCostNotMet`、`OnCooldownStarted` 等蓝图可覆盖事件，让设计师能更方便地实现自定义的反馈（如播放音效、显示UI提示）。

- **2. 网络预测与回滚的增强**
  - **建议**: 对于一个FPS游戏，精准的客户端预测和服务器回滚至关重要。可以考虑在 `UYcAbilitySystemComponent` 中增加更明确的网络状态调试功能，例如通过控制台指令显示当前的预测键、服务器确认状态等信息，以帮助开发者调试复杂的网络交互。

- **3. 属性系统的便利性封装**
  - **建议**: 围绕 `UAttributeSet` 创建更多的便利性函数和宏。例如，提供一个统一的函数用于安全地获取和修改属性值，并自动处理属性变化的预测和回调逻辑，减少在各个类中重复编写模板代码。

- **4. 与动画系统的深度集成**
  - **建议**: 可以在 `UYcGameplayAbility` 中增加与动画蒙太奇（Montage）更紧密的绑定。例如，提供一个简化的 `PlayMontageAndWait` 节点，该节点能自动处理蒙太奇的播放、打断、结束事件，并与技能的生命周期同步。
