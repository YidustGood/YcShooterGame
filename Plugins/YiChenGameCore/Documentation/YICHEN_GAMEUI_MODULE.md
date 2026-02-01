# YiChenGameUI 模块技术文档

## 1. 概述

`YiChenGameUI` 模块为 `YiChenGameCore` 插件提供了 UI 框架的基础。它深度集成了 Epic Games 的 **CommonUI** 插件，旨在提供一个现代化的、跨平台的、支持手柄和键鼠混合输入的 UI 解决方案。该模块的核心思想是通过 **Game Feature** 插件动态地向玩家屏幕上添加和移除 UI 布局（Layouts）和控件（Widgets），从而实现 UI 的高度模块化和情景化。

## 2. 核心设计

- **基于 CommonUI**: 整个模块构建于 `CommonUI` 之上，继承了其所有优点，包括统一的输入处理、样式系统（Styling）、可激活控件（Activatable Widgets）和跨平台支持。

- **Game Feature 驱动**: 游戏中的 UI，特别是常驻的 HUD 元素，不是被硬编码在某个 `PlayerController` 或 `HUD` 类中，而是通过 `Game Feature` 的 `AddWidgets` Action 来动态添加。这种方式使得 UI 的管理与游戏逻辑完全解耦，不同的游戏体验（Experience）可以拥有完全不同的 HUD 布局。

- **分层与激活**: UI 被组织成不同的层级。`UYcGameHUDLayout` 作为根布局，负责容纳所有其他的 HUD 元素。`UYcActivatableWidget` 作为所有可交互UI（如菜单、弹窗）的基类，管理着自身的可见性、激活状态和输入模式。

## 3. 关键类与系统详解

### 3.1. UYcUIManagerSubsystem

这是一个 `UGameUIManagerSubsystem` 的子类，是整个 UI 框架的中央管理器。

- **职责**: 它的核心职责是监听玩家的加入（`NotifyPlayerAdded`），并在新玩家加入时，为其创建并显示根 UI 布局（通常是 `UYcGameHUDLayout`）。
- **与 GameInstance 的关系**: 该子系统依赖于 `UCommonGameInstance`（在 `YiChenGameplay` 模块中由 `UYcGameInstance` 继承）来正确触发 `NotifyPlayerAdded` 事件。因此，使用本插件时必须确保项目的 `GameInstance` 继承自 `UYcGameInstance`。

### 3.2. AYcGameHUD

这是对原生 `AHUD` 类的简单扩展。

- **职责**: 在 `YiChenGameCore` 的 UI 框架中，`AYcGameHUD` 的作用被大大削弱了。它不再负责主动创建或绘制任何 `UWidget`。其主要职责回归到 `AHUD` 的本源：作为一个调试渲染的画布（例如，通过 `DrawDebugText`）和提供一个获取调试Actor列表的入口。
- **注意**: 开发者通常不需要继承或修改这个类。所有的 UI 元素都应通过 `Game Feature` 的 `AddWidgets` Action 来添加。

### 3.3. UYcGameHUDLayout

这是一个继承自 `UYcActivatableWidget` 的特殊控件，作为所有游戏内 HUD 元素的根容器。

- **添加方式**: 它通常由 `Game Feature` 在游戏体验加载时通过 `AddWidgets` Action 添加到玩家的屏幕上。
- **职责**: 
  - **布局容器**: 在其 Widget 树中，它会定义多个命名插槽（`Named Slot`），如 `TopLeft`, `BottomCenter` 等，供其他 UI 元素（如小地图、血条、弹药显示）通过 `UIExtension` 插件动态添加到指定位置。
  - **全局输入处理**: 它可以处理一些全局性的 UI 输入，例如，默认实现了按下 `ESC` 键时弹出主菜单的功能。

### 3.4. UYcActivatableWidget

这是项目中所有需要接收输入、可以被激活/失活的 UI 面板（如菜单、弹窗、库存界面）的基类，继承自 `UCommonActivatableWidget`。

- **核心功能**: 
  - **输入模式管理**: `GetDesiredInputConfig()` 是其最重要的功能。当一个 `UYcActivatableWidget` 被激活时，`CommonUI` 系统会调用这个函数来决定当前的输入应该如何被处理。
  - **`EYcWidgetInputMode`**: 提供了四种预设的输入模式：
    - `GameAndMenu`: 游戏和UI都接收输入（例如，在一个带可交互UI的第三人称游戏中）。
    - `Game`: 只允许游戏逻辑接收输入，UI不接收（用于纯粹的HUD显示）。
    - `Menu`: 只允许UI接收输入，游戏逻辑被屏蔽（用于全屏菜单）。
    - `Default`: 使用 `CommonUI` 的默认行为。
  - **激活与失活**: 继承了 `OnActivated` 和 `OnDeactivated` 事件，开发者可以在蓝图中方便地处理UI显示和隐藏时的逻辑。

## 4. 使用指南

1.  **创建根布局**: 创建一个继承自 `UYcGameHUDLayout` 的蓝图控件，并在其中设计好你的 HUD 布局结构，预留出用于扩展的命名插槽。
2.  **创建功能UI**: 为你的菜单、计分板等创建继承自 `UYcActivatableWidget` 的蓝图控件。在控件的属性中，设置好期望的 `InputConfig`。
3.  **通过 Game Feature 添加UI**: 创建一个 `Game Feature` 插件。在其 `.uplugin` 文件中添加 `AddWidgets` Action，将你的根布局 (`UYcGameHUDLayout`) 和其他需要在游戏开始时就显示的UI添加进去。
4.  **动态显示UI**: 在游戏逻辑中（例如，当玩家按下“Tab”键时），通过 `UCommonUIActionRouterBase::Get()->PushInputFilter(...)` 来激活你的 `UYcActivatableWidget`（如计分板）。
5.  **扩展HUD**: 对于需要动态添加到HUD上的元素（如拾取提示），创建一个普通的 `UUserWidget`，并通过 `UUIExtensionSubsystem::Get()->AddWidgetToLayoutSlot(...)` 将其添加到 `UYcGameHUDLayout` 中预设的命名插槽里。

## 5. 改进建议

- **1. 完善UI扩展点（Extension Points）**
  - **建议**: 在 `UYcGameHUDLayout` 中预定义一套标准化的、更丰富的命名插槽（Extension Points），并将其作为插件的最佳实践进行文档化。例如，`Slot.InteractionPrompt`, `Slot.TeamInfo`, `Slot.KillFeed` 等。这将使得不同 `Game Feature` 之间能够更容易地共享和扩展UI布局。

- **2. 与GameplayMessage系统深度集成**
  - **建议**: 创建一个 `UYcActivatableWidget_MessageListener` 基类，该基类可以自动注册和反注册 `GameplayMessage` 的监听器。开发者只需在派生类中指定感兴趣的消息通道和实现回调函数，即可响应游戏事件来更新UI，从而进一步降低UI与游戏逻辑的耦合度。

- **3. 提供一套基础UI组件库**
  - **建议**: 在 `YiChenGameUI` 模块中，提供一套预制的、样式化的基础UI组件，例如 `YcButton`, `YcHealthBar`, `YcAmmunitionDisplay` 等。这些组件可以与 `YiChenAbility` 和 `YiChenEquipment` 模块的数据直接绑定，开发者只需将它们拖放到自己的UI中即可使用，极大地提高了开发效率。
