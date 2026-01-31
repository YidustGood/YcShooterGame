# YiChenTeams 模块技术文档

## 1. 概述

`YiChenTeams` 模块为 `YiChenGameCore` 插件提供了一套完整的、数据驱动的团队管理系统。它负责处理多人游戏中的队伍创建、玩家分配、敌我关系判断（Attitude System）以及团队范围内的状态共享。该系统被设计为服务器权威，并与 Unreal Engine 的 `GameplayTag` 系统和 `GenericTeamAgentInterface` 紧密集成，为实现复杂的团队对抗玩法（如团队死亡竞赛、夺旗等）提供了坚实的基础。

## 2. 核心设计

- **子系统驱动 (Subsystem Driven)**: 模块的核心是一个 `UWorldSubsystem` (`UYcTeamSubsystem`)，它充当了所有团队信息的中央管理器。这种设计使得游戏中的任何 Actor 都可以方便地查询团队信息，而无需获取对特定 `GameState` 或 `PlayerState` 的引用。

- **接口驱动 (Interface Driven)**: 一个 Actor 如何“加入”团队系统，是通过实现 `IYcTeamAgentInterface` 接口来定义的。这种方式极大地提高了系统的灵活性，任何 Actor（无论是 `APlayerState`, `ACharacter` 还是 `AController`）都可以通过实现该接口来拥有团队属性。

- **数据驱动的团队创建**: 游戏对局中需要创建哪些队伍，是通过 `UYcTeamCreationComponent` 的属性 (`TeamsToCreate`) 来数据驱动配置的。这使得策划可以轻松地为不同的游戏模式配置不同的队伍设置（如2队、4队或自由混战）。

## 3. 关键类与系统详解

### 3.1. UYcTeamSubsystem

这是一个 `UWorldSubsystem`，是团队系统的核心中枢，仅在服务器上运行其核心逻辑。

- **职责**: 
  - **团队注册与管理**: 负责注册（`RegisterTeamInfo`）和注销（`UnregisterTeamInfo`）游戏中的所有队伍实例 (`AYcTeamInfoBase`)。
  - **团队关系查询**: 提供了 `CompareTeams` 和 `CanCauseDamage` 等核心函数，用于判断两个 Actor 之间的敌我关系，是实现友军伤害（Friendly Fire）逻辑的基础。
  - **团队成员管理**: 提供了 `ChangeTeamForActor` 函数，用于在服务器上改变一个 Actor 的队伍归属。
  - **团队状态共享**: 提供了 `AddTeamTagStack`, `RemoveTeamTagStack` 等函数，允许开发者在团队层面上管理和同步 `GameplayTag` 的计数值，可用于实现团队Buff、团队任务进度等功能。

### 3.2. IYcTeamAgentInterface

这是一个接口，任何希望参与团队系统的 Actor 都应该实现它。

- **核心函数**: 
  - `GetTeamId()`: 返回该 Actor 当前所属队伍的数字ID。这是团队系统识别 Actor 身份的基础。
  - `GetOnTeamIndexChangedDelegate()`: 返回一个委托，当该 Actor 的队伍ID发生变化时，这个委托会被广播。UI 和其他游戏系统可以绑定这个委托来响应玩家队伍的变化。
- **推荐实践**: 插件推荐将 `IYcTeamAgentInterface` 在 `APlayerState` 上实现，因为 `PlayerState` 代表了玩家的逻辑身份，在 `Pawn` 死亡后依然存在。

### 3.3. UYcTeamCreationComponent

这是一个 `UGamestateComponent`，负责在游戏开始时自动创建队伍并为玩家分配队伍。

- **`TeamsToCreate`**: 一个 `TMap<uint8, UYcTeamAsset*>`，用于配置本局游戏需要创建的队伍。`Key` 是队伍ID，`Value` 是一个 `UYcTeamAsset` 数据资产，用于定义队伍的显示信息（如队名、颜色、图标等）。
- **自动创建**: 该组件会自动监听 `Experience` 加载完成的消息，并在加载完成后，根据 `TeamsToCreate` 的配置，在服务器上创建所有队伍的 `AYcTeamPublicInfo` 实例。
- **自动分配**: 当新玩家加入游戏时（通过监听 `FGameModePlayerInitializedMessage` 消息），该组件会调用 `ServerChooseTeamForPlayer`。默认的分配逻辑是 `GetLeastPopulatedTeamID`，即将新玩家分配到当前人数最少的队伍，以维持队伍平衡。

### 3.4. AYcTeamPublicInfo & UYcTeamAsset

- **`AYcTeamPublicInfo`**: 这是一个在运行时创建的、可被网络复制的 `AActor`，代表一个实际存在的队伍。它持有对 `UYcTeamAsset` 的引用，并将这个引用同步给所有客户端。
- **`UYcTeamAsset`**: 这是一个 `UDataAsset`，用于定义队伍的静态显示信息。通过将队伍的视觉表现（颜色、图标）与其实际的逻辑ID解耦，使得UI和美术可以独立于程序进行迭代。

## 4. 使用指南

1.  **创建 TeamAsset**: 在内容浏览器中创建 `YcTeamAsset` 数据资产，为你的每个队伍（如“红队”、“蓝队”）配置好队名和颜色。
2.  **配置 GameState**: 确保你的 `GameState` 上添加了 `UYcTeamCreationComponent`。
3.  **配置队伍创建**: 在 `GameState` 的 `UYcTeamCreationComponent` 详情面板中，配置 `TeamsToCreate` 属性，将你的队伍ID和你创建的 `TeamAsset` 关联起来。
4.  **实现接口**: 在你的 `PlayerState` 类中，继承并实现 `IYcTeamAgentInterface` 接口。最简单的方式是直接添加 `UYcTeamComponent`，它提供了该接口的默认实现。
5.  **查询团队关系**: 在任何需要判断敌我关系的地方（例如，在计算伤害的 `GameplayEffect` 中），通过 `UYcTeamSubsystem::Get(this)->CompareTeams(Instigator, Target)` 来获取两个 Actor 的团队关系。

## 5. 改进建议

- **1. 灵活的队伍分配策略**
  - **建议**: 当前的队伍分配逻辑（`GetLeastPopulatedTeamID`）被硬编码在 `UYcTeamCreationComponent` 中。可以将其抽象为一个可配置的策略。例如，创建一个 `UYcTeamAssignmentStrategy` 基类，并提供 `AssignOnBalance`（平衡）、`AssignByParty`（按队伍/派对分配）、`AssignRandomly`（随机）等不同的子类。然后在 `UYcTeamCreationComponent` 中持有一个该策略类的引用，允许开发者根据游戏模式选择不同的分配方式。

- **2. 团队内部通信**
  - **建议**: 扩展 `UYcTeamSubsystem`，增加团队内部的事件广播功能。例如，提供 `BroadcastMessageToTeam(int32 TeamId, FGameplayMessage Message)` 这样的函数。这将为实现团队聊天、小地图标记共享、战术指令等功能提供一个统一、高效的底层支持。

- **3. 私有团队信息 (`AYcTeamPrivateInfo`)**
  - **建议**: 当前只有 `AYcTeamPublicInfo`，所有信息都是公开的。可以仿照 `PlayerState` 和 `PlayerController` 的设计，创建一个 `AYcTeamPrivateInfo` Actor，它只会被复制给属于该队伍的玩家。这个私有信息对象可以用来同步一些只有队友才能看到的数据，例如队友的精确位置、战术标记、秘密任务目标等。

- **4. 跨队伍的外交关系 (Diplomacy)**
  - **建议**: 对于更复杂的游戏模式（如大逃杀中的临时组队、或MMO中的公会战争），可以扩展 `CompareTeams` 的逻辑，引入一个“外交矩阵”或关系表。这个系统可以定义队伍之间的关系，如 `Neutral`（中立）, `Allied`（同盟）, `Enemy`（敌对），而不仅仅是简单的“同队/不同队”二元关系。
