// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "AbilitySystemComponent.h"
#include "Abilities/YcGameplayAbility.h"
#include "YcAbilitySystemComponent.generated.h"

class UYcGameplayAbility;
class UYcAbilityTagRelationshipMapping;
class USkeletalMeshComponent;

/**
 * 本地蒙太奇信息结构体（针对特定骨骼网格）
 * 
 * 存储在本地发起蒙太奇播放的端（服务器存储所有，客户端存储预测的）
 * 不进行网络复制，仅用于本地追踪蒙太奇播放状态
 * 
 * 使用场景：
 * - 服务器：追踪所有骨骼网格上播放的蒙太奇
 * - 客户端：追踪预测播放的蒙太奇，用于服务器拒绝时的回滚
 */
USTRUCT()
struct YICHENABILITY_API FGameplayAbilityLocalAnimMontageForMesh
{
	GENERATED_BODY();

public:
	/** 播放蒙太奇的目标骨骼网格组件 */
	UPROPERTY()
	USkeletalMeshComponent* Mesh;

	/** 本地蒙太奇详细信息，包含AnimMontage、AnimatingAbility、PlayInstanceId等 */
	UPROPERTY()
	FGameplayAbilityLocalAnimMontage LocalMontageInfo;

	/** 默认构造函数 */
	FGameplayAbilityLocalAnimMontageForMesh() : Mesh(nullptr), LocalMontageInfo()
	{
	}

	/** 
	 * 带Mesh参数的构造函数
	 * @param InMesh 目标骨骼网格
	 */
	FGameplayAbilityLocalAnimMontageForMesh(USkeletalMeshComponent* InMesh)
		: Mesh(InMesh), LocalMontageInfo()
	{
	}

	/**
	 * 完整构造函数
	 * @param InMesh 目标骨骼网格
	 * @param InLocalMontageInfo 本地蒙太奇信息
	 */
	FGameplayAbilityLocalAnimMontageForMesh(USkeletalMeshComponent* InMesh, FGameplayAbilityLocalAnimMontage& InLocalMontageInfo)
		: Mesh(InMesh), LocalMontageInfo(InLocalMontageInfo)
	{
	}
};

/**
 * 复制蒙太奇信息结构体（针对特定骨骼网格）
 * 
 * 用于将蒙太奇信息从服务器复制到模拟客户端
 * 包含蒙太奇播放所需的所有同步数据
 * 
 * 复制内容（通过FGameplayAbilityRepAnimMontage）：
 * - Animation: 正在播放的蒙太奇资源
 * - PlayRate: 播放速率
 * - Position: 当前播放位置
 * - BlendTime: 混出时间
 * - NextSectionID: 下一个Section的ID
 * - IsStopped: 是否已停止
 * - PlayInstanceId: 播放实例ID（用于检测是否需要重新播放）
 */
USTRUCT()
struct YICHENABILITY_API FGameplayAbilityRepAnimMontageForMesh
{
	GENERATED_BODY();

public:
	/** 播放蒙太奇的目标骨骼网格组件 */
	UPROPERTY()
	USkeletalMeshComponent* Mesh;

	/** 复制的蒙太奇信息，包含所有需要同步的播放状态 */
	UPROPERTY()
	FGameplayAbilityRepAnimMontage RepMontageInfo;

	/** 默认构造函数 */
	FGameplayAbilityRepAnimMontageForMesh() : Mesh(nullptr), RepMontageInfo()
	{
	}

	/**
	 * 带Mesh参数的构造函数
	 * @param InMesh 目标骨骼网格
	 */
	FGameplayAbilityRepAnimMontageForMesh(USkeletalMeshComponent* InMesh)
		: Mesh(InMesh), RepMontageInfo()
	{
	}
};

/**
 * 插件提供的技能组件基类
 */
UCLASS(ClassGroup=(YiChenAbility), meta=(BlueprintSpawnableComponent))
class YICHENABILITY_API UYcAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()
public:
	/**
	 * 构造函数
	 * @param ObjectInitializer 对象初始化器
	 */
	UYcAbilitySystemComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	/**
	 * 从Actor中获取UYcAbilitySystemComponent组件
	 * @param Actor 要查询的Actor
	 * @param LookForComponent 是否主动查找组件。如果为false，则只查找已缓存的组件；如果为true，则会遍历所有组件
	 * @return 找到的技能系统组件，如果不存在则返回nullptr
	 */
	static UYcAbilitySystemComponent* GetAbilitySystemComponentFromActor(const AActor* Actor, bool LookForComponent = false);
	
	/**
	 * 初始化ASC的ActorInfo并通知所有技能
	 * 在设置新的Owner和Avatar时调用，会通知所有已授予的技能Avatar已变化
	 * 如果Avatar是Pawn类型且与之前不同，会触发所有技能的OnPawnAvatarSet回调
	 * 并尝试激活所有激活策略为OnSpawn的技能
	 * 该函数可被外部调用以实现重定向ASC的所有者
	 * @param InOwnerActor ASC的所有者Actor
	 * @param InAvatarActor ASC的Avatar Actor（通常是Pawn及其子类）
	 */
	virtual void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;
	
	/**
	 * 设置标签关系映射
	 * 用于配置技能激活时的标签依赖和阻挡关系。如果传入nullptr则清除当前映射。
	 * @param NewMapping 新的标签关系映射对象
	 */
	void SetTagRelationshipMapping(UYcAbilityTagRelationshipMapping* NewMapping);
	
	/**
	 * 获取技能激活所需的额外标签要求
	 * 根据技能标签查询标签关系映射，获取激活时必需的标签和被阻挡的标签。
	 * @param AbilityTags 技能的标签容器
	 * @param OutActivationRequired 输出：激活时必需的标签列表
	 * @param OutActivationBlocked 输出：激活时被阻挡的标签列表
	 */
	void GetAdditionalActivationTagRequirements(const FGameplayTagContainer& AbilityTags, FGameplayTagContainer& OutActivationRequired, FGameplayTagContainer& OutActivationBlocked) const;

	/**
	 * 获取与指定技能句柄和激活信息关联的目标数据
	 * 从技能目标数据缓存映射表中查找对应的目标数据，这些数据通常在技能激活时通过网络复制存储
	 * 常用于判断技能是否命中目标、获取命中结果等信息
	 * @param AbilityHandle 技能规格句柄
	 * @param ActivationInfo 技能激活信息，包含激活预测键等
	 * @param OutTargetDataHandle 输出：找到的目标数据句柄，如果未找到则为空
	 */
	void GetAbilityTargetData(const FGameplayAbilitySpecHandle AbilityHandle, FGameplayAbilityActivationInfo ActivationInfo, FGameplayAbilityTargetDataHandle& OutTargetDataHandle);
	
protected:
	/** 是否启用服务器RPC批处理，启用时可将多个技能相关的RPC合并为一个，减少网络流量 */
	virtual bool ShouldDoServerAbilityRPCBatch() const override { return true; }
	
	/** 应用TagRelationshipMapping中配置的Block和Cancel标签 */
	virtual void ApplyAbilityBlockAndCancelTags(const FGameplayTagContainer& AbilityTags, UGameplayAbility* RequestingAbility, bool bEnableBlockTags, const FGameplayTagContainer& BlockTags,
												bool bExecuteCancelTags, const FGameplayTagContainer& CancelTags) override;
	/** 处理技能可取消状态变化时的特殊逻辑 */
	virtual void HandleChangeAbilityCanBeCanceled(const FGameplayTagContainer& AbilityTags, UGameplayAbility* RequestingAbility, bool bCanBeCanceled) override;
	
	/** 尝试激活所有OnSpawn激活策略的技能 */
	void TryActivateAbilitiesOnSpawn();
	
	///////////////// 基于玩家输入+GameplayTag驱动的技能激活/取消功能 /////////////////
public:
	/**
	 * 响应玩家输入按下事件
	 * 玩家输入被转化为InputTag，根据技能的DynamicSpecSourceTags匹配相应的技能
	 * 将匹配的技能句柄添加到输入按下列表中，以便后续在ProcessAbilityInput中激活
	 * 输入与Tag的映射由数据驱动的配置决定（通常在PlayerController中配置）
	 * @param InputTag 输入对应的GameplayTag
	 */
	void AbilityInputTagPressed(const FGameplayTag& InputTag);
	
	/**
	 * 响应玩家输入释放事件
	 * 当玩家释放输入时调用，将匹配的技能从输入按下列表中移除
	 * 并将其添加到输入释放列表中，以便在ProcessAbilityInput中触发WaitInputRelease事件
	 * @param InputTag 输入对应的GameplayTag
	 */
	void AbilityInputTagReleased(const FGameplayTag& InputTag);
	
	/**
	 * 处理技能输入的主函数
	 * 由AYcPlayerController的PostProcessInput()调用，负责每帧处理所有收集到的输入
	 * 流程：
	 *   1. 检查是否被TAG_Gameplay_AbilityInputBlocked标签阻止
	 *   2. 处理持续输入的技能（WhileInputActive）
	 *   3. 处理单次按下触发的技能（OnInputTriggered）
	 *   4. 激活所有待激活的技能
	 *   5. 处理输入释放事件（触发WaitInputRelease）
	 *   6. 清理本帧的输入缓存
	 * @param DeltaTime 帧时间差
	 * @param bGamePaused 游戏是否暂停
	 */
	void ProcessAbilityInput(float DeltaTime, bool bGamePaused);
	
	
	/**
	 * 清理所有输入缓存
	 * 重置所有输入相关的列表，用于强制清空输入状态
	 * 常用于游戏暂停、菜单打开等需要中断输入处理的场景
	 */
	void ClearAbilityInput();
	
protected:
	
	/**
	 * 处理技能输入按下事件（覆盖父类）
	 * 当技能已激活时调用，用于在技能激活期间响应新的输入按下
	 * 触发WaitInputPress事件，允许技能在激活期间响应输入
	 * 不直接支持bReplicateInputDirectly，而是使用复制事件确保WaitInputPress能正常工作
	 * @param Spec 技能规格
	 */
	virtual void AbilitySpecInputPressed(FGameplayAbilitySpec& Spec) override;
	
	/**
	 * 处理技能输入释放事件（覆盖父类）
	 * 当技能已激活时调用，用于在技能激活期间响应输入释放
	 * 触发WaitInputRelease事件，允许技能在激活期间响应输入释放
	 * 不直接支持bReplicateInputDirectly，而是使用复制事件确保WaitInputRelease能正常工作
	 * @param Spec 技能规格
	 */
	virtual void AbilitySpecInputReleased(FGameplayAbilitySpec& Spec) override;
	
private:
	/**
	 * 当前帧被按下的技能句柄列表
	 * 存储本帧新按下的输入对应的技能句柄
	 * 在ProcessAbilityInput中处理后会被清空
	 */
	TArray<FGameplayAbilitySpecHandle> InputPressedSpecHandles;

	/**
	 * 当前帧被释放的技能句柄列表
	 * 存储本帧释放的输入对应的技能句柄
	 * 在ProcessAbilityInput中处理后会被清空
	 */
	TArray<FGameplayAbilitySpecHandle> InputReleasedSpecHandles;

	/**
	 * 标记已处理的按下输入
	 * 用于区分OnInputTriggered类型的输入是否已被处理
	 * 避免在松开按键时下一帧再次添加到InputPressedSpecHandles，导致重复处理
	 * 当输入释放时会从此Map中移除
	 */
	TMap<FGameplayAbilitySpecHandle, bool> InputPressedToReleasedSpecHandles;

	/**
	 * 当前帧保持按住的技能句柄列表
	 * 存储输入持续按住对应的技能句柄
	 * 用于WhileInputActive激活策略的技能
	 * 当输入释放时会从此列表中移除
	 */
	TArray<FGameplayAbilitySpecHandle> InputHeldSpecHandles;
	
	/**
	 * 取消由玩家输入激活的所有技能
	 * 取消所有激活策略为OnInputTriggered或WhileInputActive的已激活技能
	 * 常用于游戏暂停、菜单打开或需要中断所有输入激活技能的场景
	 * @param bReplicateCancelAbility 是否复制取消事件到网络
	 */
	void CancelInputActivatedAbilities(bool bReplicateCancelAbility);
	///////////////// ~基于玩家输入+GameplayTag驱动的技能激活/取消功能 /////////////////
	
	///////////////// 技能激活策略分组相关 /////////////////
public:
	/**
	 * 检查特定激活组是否处于阻止激活状态
	 * @param Group 要检查的激活组
	 * @return 如果该组被阻止则返回true，否则返回false
	 */
	bool IsActivationGroupBlocked(EYcAbilityActivationGroup Group) const;
	
	/**
	 * 将技能添加到激活组
	 * 增加该组的计数，并根据组类型执行相应的逻辑（如取消可替换的排他性技能）
	 * @param Group 目标激活组
	 * @param YcAbility 要添加的技能
	 */
	void AddAbilityToActivationGroup(EYcAbilityActivationGroup Group, UYcGameplayAbility* YcAbility);
	
	/**
	 * 从激活组中移除技能
	 * 减少该组的计数
	 * @param Group 目标激活组
	 * @param YcAbility 要移除的技能
	 */
	void RemoveAbilityFromActivationGroup(EYcAbilityActivationGroup Group, const UYcGameplayAbility* YcAbility);
	
	/**
	 * 取消特定激活组的所有技能
	 * @param Group 目标激活组
	 * @param IgnoreAbility 要忽略的技能，不会被取消
	 * @param bReplicateCancelAbility 是否复制取消事件到网络
	 */
	void CancelActivationGroupAbilities(EYcAbilityActivationGroup Group, UYcGameplayAbility* IgnoreAbility, bool bReplicateCancelAbility);
private:
	/** 每个激活组中正在运行的技能数量 */
	int32 ActivationGroupCounts[static_cast<uint8>(EYcAbilityActivationGroup::MAX)];
	///////////////// ~技能激活策略分组相关 /////////////////

	///////////////// ~技能激活情况通知相关 /////////////////
	
	virtual void NotifyAbilityActivated(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability) override;
	
	/**
	 * 通知技能激活失败（覆盖父类）
	 * 当技能激活失败时调用，会根据网络情况决定处理方式：
	 *   - 如果Avatar不是本地控制且技能支持网络，则通过ClientRPC通知客户端
	 *   - 否则直接在本地调用HandleAbilityFailed处理失败逻辑
	 * 最终会调用技能的OnAbilityFailedToActivate，触发失败消息广播和蓝图事件
	 * @param Handle 技能规格句柄
	 * @param Ability 激活失败的技能
	 * @param FailureReason 导致激活失败的标签容器
	 */
	virtual void NotifyAbilityFailed(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason) override;
	
	virtual void NotifyAbilityEnded(FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, bool bWasCancelled) override;
	
	/**
	 * 客户端RPC：通知客户端技能激活失败
	 * 当服务器检测到技能激活失败且Avatar不是本地控制时，通过此RPC通知客户端
	 * 客户端收到后会调用技能的OnAbilityFailedToActivate，触发失败消息广播和蓝图事件
	 * @param Ability 激活失败的技能
	 * @param FailureReason 导致激活失败的标签容器
	 */
	UFUNCTION(Client, Unreliable)
	void ClientNotifyAbilityFailed(const UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason);

	/**
	 * 处理技能激活失败
	 * 直接调用技能的OnAbilityFailedToActivate，触发失败消息广播和蓝图事件
	 * 如果技能配置了FailureTagToUserFacingMessages映射表，会通过GameplayMessageSubsystem广播用户友好的失败消息
	 * @param Ability 激活失败的技能
	 * @param FailureReason 导致激活失败的标签容器
	 */
	void HandleAbilityFailed(const UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason);
	
	///////////////// ~技能激活情况通知相关 /////////////////
	
public:
	/////////// 增强蓝图使用的函数 ////////////
	
	/**
	 * 根据技能类查找技能规格句柄
	 * 遍历已激活的技能列表，找到指定类型的技能规格
	 * @param AbilityClass 要查找的技能类
	 * @param OptionalSourceObject 可选的源对象。如果指定，则只返回来自该源对象的技能规格
	 * @return 找到的技能规格句柄，如果未找到则返回无效句柄
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	FGameplayAbilitySpecHandle FindAbilitySpecHandleForClass(TSubclassOf<UGameplayAbility> AbilityClass, UObject* OptionalSourceObject=nullptr);
	
	/**
	 * 尝试取消指定类型的技能
	 * 查找并取消所有正在激活的指定类型的技能
	 * @param InAbilityToCancel 要取消的技能类
	 * @return 如果成功取消至少一个技能则返回true，否则返回false
	 */
	UFUNCTION(BlueprintCallable, Category="AbilitySystem")
	bool TryCancelAbilityByClass(TSubclassOf<UGameplayAbility> InAbilityToCancel);
	
	/**
	 * 获取指定GameplayTag的计数（蓝图接口）
	 * 返回该标签在ASC中的堆叠计数
	 * @param TagToCheck 要查询的GameplayTag
	 * @return 该标签的计数，如果标签不存在则返回0
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities", Meta = (DisplayName = "GetTagCount"))
	int32 K2_GetTagCount(FGameplayTag TagToCheck) const;
	
	/**
	 * 添加一个Loose GameplayTag（蓝图接口）
	 * Loose Tag不会被网络复制，仅在本地存在
	 * @param GameplayTag 要添加的GameplayTag
	 * @param Count 添加的数量，默认为1。如果Count > 1，则该标签会被堆叠
	 */
	UFUNCTION(BlueprintCallable, Category = "Abilities", Meta = (DisplayName = "YcAddLooseGameplayTag"))
	void K2_YcAddLooseGameplayTag(const FGameplayTag& GameplayTag, int32 Count = 1);

	/**
	 * 批量添加Loose GameplayTags（蓝图接口）
	 * Loose Tags不会被网络复制，仅在本地存在
	 * @param GameplayTags 要添加的GameplayTag容器
	 * @param Count 每个标签添加的数量，默认为1
	 */
	UFUNCTION(BlueprintCallable, Category = "Abilities", Meta = (DisplayName = "YcAddLooseGameplayTags"))
	void K2_YcAddLooseGameplayTags(const FGameplayTagContainer& GameplayTags, int32 Count = 1);

	/**
	 * 移除一个Loose GameplayTag（蓝图接口）
	 * Loose Tag不会被网络复制，仅在本地存在
	 * @param GameplayTag 要移除的GameplayTag
	 * @param Count 移除的数量，默认为1。如果该标签的计数大于Count，则只减少计数；否则完全移除该标签
	 */
	UFUNCTION(BlueprintCallable, Category = "Abilities", Meta = (DisplayName = "YcRemoveLooseGameplayTag"))
	void K2_YcRemoveLooseGameplayTag(const FGameplayTag& GameplayTag, int32 Count = 1);

	/**
	 * 批量移除Loose GameplayTags（蓝图接口）
	 * Loose Tags不会被网络复制，仅在本地存在
	 * @param GameplayTags 要移除的GameplayTag容器
	 * @param Count 每个标签移除的数量，默认为1
	 */
	UFUNCTION(BlueprintCallable, Category = "Abilities", Meta = (DisplayName = "YcRemoveLooseGameplayTags"))
	void K2_YcRemoveLooseGameplayTags(const FGameplayTagContainer& GameplayTags, int32 Count = 1);
	
	/**
	 * 在本地执行一次性的GameplayCue效果
	 * 该效果仅在本地客户端执行，不会被网络复制
	 * @param GameplayCueTag 要执行的GameplayCue标签
	 * @param GameplayCueParameters GameplayCue的参数，包含位置、方向、强度等信息
	 */
	UFUNCTION(BlueprintCallable, Category = "GameplayCue", Meta = (AutoCreateRefTerm = "GameplayCueParameters", GameplayTagFilter = "GameplayCue"))
	void ExecuteGameplayCueLocal(const FGameplayTag GameplayCueTag, const FGameplayCueParameters& GameplayCueParameters);

	/**
	 * 在本地添加一个持续的GameplayCue效果
	 * 该效果会触发OnActive和WhileActive事件，仅在本地客户端执行
	 * @param GameplayCueTag 要添加的GameplayCue标签
	 * @param GameplayCueParameters GameplayCue的参数，包含位置、方向、强度等信息
	 */
	UFUNCTION(BlueprintCallable, Category = "GameplayCue", Meta = (AutoCreateRefTerm = "GameplayCueParameters", GameplayTagFilter = "GameplayCue"))
	void AddGameplayCueLocal(const FGameplayTag GameplayCueTag, const FGameplayCueParameters& GameplayCueParameters);

	/**
	 * 在本地移除一个GameplayCue效果
	 * 该效果会触发Removed事件，仅在本地客户端执行
	 * @param GameplayCueTag 要移除的GameplayCue标签
	 * @param GameplayCueParameters GameplayCue的参数，包含位置、方向、强度等信息
	 */
	UFUNCTION(BlueprintCallable, Category = "GameplayCue", Meta = (AutoCreateRefTerm = "GameplayCueParameters", GameplayTagFilter = "GameplayCue"))
	void RemoveGameplayCueLocal(const FGameplayTag GameplayCueTag, const FGameplayCueParameters& GameplayCueParameters);
	
	/**
	 * 尝试激活技能并批处理所有RPC
	 * 将同一帧内的多个技能相关RPC合并为一个，以减少网络流量
	 * 最优情况：ActivateAbility、SendTargetData和EndAbility合并为一个RPC（而不是三个）
	 * 次优情况：ActivateAbility和SendTargetData合并为一个RPC，EndAbility单独发送
	 * 如果技能包含延迟任务（如等待输入），则无法完全批处理
	 * 示例：
	 *   - 单发射击：ActivateAbility + SendTargetData + EndAbility 合并为1个RPC
	 *   - 连射：首发合并ActivateAbility + SendTargetData，后续每发1个RPC，最后1个RPC结束技能
	 * @param InAbilityHandle 要激活的技能规格句柄
	 * @param EndAbilityImmediately 是否立即结束技能。如果为true，激活后会立即调用ExternalEndAbility()
	 * @return 如果技能成功激活则返回true，否则返回false
	 */
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	virtual bool BatchRPCTryActivateAbility(FGameplayAbilitySpecHandle InAbilityHandle, bool EndAbilityImmediately);
	
	/**
	 * 对自身应用GameplayEffect，支持预测
	 * 如果ASC拥有有效的预测键，则使用预测模式应用效果，以实现客户端预测
	 * 如果预测键无效，则以普通模式应用效果
	 * 常用于交互系统的Pre/PostInteract()中
	 * @param GameplayEffectClass 要应用的GameplayEffect类
	 * @param Level 效果等级，影响效果的强度
	 * @param EffectContext 效果上下文，包含源对象、目标对象等信息。如果无效则会自动创建
	 * @return 应用的GameplayEffect句柄，用于后续移除或修改效果
	 */
	UFUNCTION(BlueprintCallable, Category = "GameplayEffects", Meta = (DisplayName = "ApplyGameplayEffectToSelfWithPrediction"))
	FActiveGameplayEffectHandle K2_ApplyGameplayEffectToSelfWithPrediction(TSubclassOf<UGameplayEffect> GameplayEffectClass, float Level, FGameplayEffectContextHandle EffectContext);

	/**
	 * 对目标应用GameplayEffect，支持预测
	 * 如果ASC拥有有效的预测键，则使用预测模式应用效果，以实现客户端预测
	 * 如果预测键无效，则以普通模式应用效果
	 * 常用于交互系统的Pre/PostInteract()中
	 * @param GameplayEffectClass 要应用的GameplayEffect类
	 * @param Target 目标ASC。如果为nullptr则返回无效句柄
	 * @param Level 效果等级，影响效果的强度
	 * @param Context 效果上下文，包含源对象、目标对象等信息
	 * @return 应用的GameplayEffect句柄，用于后续移除或修改效果
	 */
	UFUNCTION(BlueprintCallable, Category = "GameplayEffects", Meta = (DisplayName = "ApplyGameplayEffectToTargetWithPrediction"))
	FActiveGameplayEffectHandle K2_ApplyGameplayEffectToTargetWithPrediction(TSubclassOf<UGameplayEffect> GameplayEffectClass, UAbilitySystemComponent* Target, float Level, FGameplayEffectContextHandle Context);
	
	/**
	 * 获取当前预测键的状态信息
	 * 返回预测键的字符串表示和是否有效的状态
	 * @return 包含预测键信息和有效性的字符串
	 */
	UFUNCTION(BlueprintCallable, Category = "Ability")
	virtual FString GetCurrentPredictionKeyStatus();
	
	/////////// ~增强蓝图使用的函数 ////////////
public:
	/**
	 * 技能取消判定函数类型
	 * 用于CancelAbilitiesByFunc中判断是否应该取消指定的技能
	 * @param Ability 要判定的技能CDO
	 * @param Handle 技能的规格句柄
	 * @return 如果返回true则取消该技能，否则保留
	 */
	typedef TFunctionRef<bool(const UYcGameplayAbility* Ability, FGameplayAbilitySpecHandle Handle)> TShouldCancelAbilityFunc;
	
	/**
	 * 根据自定义条件取消技能
	 * 遍历所有已激活的技能，根据提供的判定函数决定是否取消
	 * 支持Instanced和NonInstanced两种技能实例化策略
	 * 对于Instanced技能，会取消所有生成的实例；对于NonInstanced技能，会取消CDO
	 * @param ShouldCancelFunc 判定函数，返回true表示取消该技能
	 * @param bReplicateCancelAbility 是否复制取消事件到网络
	 */
	void CancelAbilitiesByFunc(const TShouldCancelAbilityFunc& ShouldCancelFunc, bool bReplicateCancelAbility);
	
private:
	// 标签关系映射表，用于查询技能激活时的标签依赖和阻挡关系
	UPROPERTY()
	TObjectPtr<UYcAbilityTagRelationshipMapping> TagRelationshipMapping;
	
public:
	//////////////// 蒙太奇动画播放扩展功能 ////////////////
	/**
	 * 以下功能扩展了UAbilitySystemComponent的蒙太奇播放能力，
	 * 支持在AvatarActor的多个USkeletalMeshComponent上播放蒙太奇动画, 并且支持预测和复制。
	 * 
	 * 网络复制机制：
	 * - LocalAnimMontageInfoForMeshes: 存储本地蒙太奇信息（服务器存储所有，客户端存储预测的）
	 * - RepAnimMontageInfoForMeshes: 复制属性，服务器更新后自动同步到所有客户端
	 * 
	 * 使用场景：
	 * - 角色有多个骨骼网格（如FPS游戏的第一人称角色Mesh）
	 * - 需要在特定Mesh上播放动画（如在FPS游戏第一人称角色Mesh上播放动画）
	 * - 需要网络同步的蒙太奇播放
	 * 
	 * 调试：
	 * - 适用控制台命令 net.Montage.Debug 可以在执行的时候将蒙太奇复制信息打印到日志
	 */
	
	// ----------------------------------------------------------------------------------------------------------------
	//	多骨骼网格蒙太奇支持 (AnimMontage Support for multiple USkeletalMeshComponents)
	//  支持在AvatarActor的多个骨骼网格上播放蒙太奇动画
	//  注意：同一时间只能有一个技能在播放动画
	// ----------------------------------------------------------------------------------------------------------------	

	/**
	 * 在指定骨骼网格上播放蒙太奇
	 * 
	 * 核心网络复制函数，处理蒙太奇的播放和网络同步：
	 * - 服务器(Authority)：播放蒙太奇并更新RepAnimMontageInfoForMeshes复制属性
	 * - 主控端(AutonomousProxy)：本地预测播放，注册PredictionKey拒绝回调
	 * - 模拟端(SimulatedProxy)：不直接调用，通过OnRep_ReplicatedAnimMontageForMesh接收复制数据
	 * 
	 * @param AnimatingAbility 正在播放动画的技能
	 * @param InMesh 目标骨骼网格，必须属于AvatarActor
	 * @param ActivationInfo 技能激活信息，包含预测键
	 * @param Montage 要播放的蒙太奇
	 * @param InPlayRate 播放速率
	 * @param StartSectionName 起始Section名称
	 * @param bReplicateMontage 是否复制到其他客户端
	 * @return 蒙太奇时长，-1表示播放失败
	 */
	virtual float PlayMontageForMesh(UGameplayAbility* AnimatingAbility, class USkeletalMeshComponent* InMesh, FGameplayAbilityActivationInfo ActivationInfo, UAnimMontage* Montage, float InPlayRate, FName StartSectionName = NAME_None, bool bReplicateMontage = true);

	/**
	 * 在模拟端播放蒙太奇（不更新复制/预测结构）
	 * 
	 * 由OnRep_ReplicatedAnimMontageForMesh调用，用于模拟代理接收复制数据后播放蒙太奇
	 * 不会更新任何复制属性，仅在本地播放动画
	 * 
	 * @param InMesh 目标骨骼网格
	 * @param Montage 要播放的蒙太奇
	 * @param InPlayRate 播放速率
	 * @param StartSectionName 起始Section名称
	 * @return 蒙太奇时长，-1表示播放失败
	 */
	virtual float PlayMontageSimulatedForMesh(USkeletalMeshComponent* InMesh, UAnimMontage* Montage, float InPlayRate, FName StartSectionName = NAME_None);

	/**
	 * 停止指定骨骼网格上当前播放的蒙太奇
	 * 
	 * 调用者应确保自己是当前播放动画的技能（或有充分理由不检查）
	 * 服务器调用时会更新复制属性，同步停止状态到所有客户端
	 * 
	 * @param InMesh 目标骨骼网格
	 * @param OverrideBlendOutTime 混出时间覆盖，-1使用蒙太奇默认值
	 */
	virtual void CurrentMontageStopForMesh(USkeletalMeshComponent* InMesh, float OverrideBlendOutTime = -1.0f);

	/**
	 * 停止所有骨骼网格上当前播放的蒙太奇
	 * @param OverrideBlendOutTime 混出时间覆盖，-1使用蒙太奇默认值
	 */
	virtual void StopAllCurrentMontages(float OverrideBlendOutTime = -1.0f);

	/**
	 * 如果指定蒙太奇是当前正在播放的，则停止它
	 * @param InMesh 目标骨骼网格
	 * @param Montage 要检查并停止的蒙太奇
	 * @param OverrideBlendOutTime 混出时间覆盖
	 */
	virtual void StopMontageIfCurrentForMesh(USkeletalMeshComponent* InMesh, const UAnimMontage& Montage, float OverrideBlendOutTime = -1.0f);

	/**
	 * 清除所有骨骼网格上指定技能的AnimatingAbility引用
	 * @param Ability 要清除的技能
	 */
	virtual void ClearAnimatingAbilityForAllMeshes(UGameplayAbility* Ability);

	/**
	 * 跳转到指定骨骼网格上蒙太奇的指定Section
	 * 
	 * 网络行为：
	 * - 服务器：本地跳转并更新复制属性
	 * - 客户端：本地跳转并发送ServerRPC通知服务器
	 * 
	 * @param InMesh 目标骨骼网格
	 * @param SectionName 目标Section名称
	 */
	virtual void CurrentMontageJumpToSectionForMesh(USkeletalMeshComponent* InMesh, FName SectionName);

	/**
	 * 设置指定骨骼网格上蒙太奇的下一个Section
	 * 
	 * 用于实现蒙太奇的分支播放，如连击系统
	 * 
	 * @param InMesh 目标骨骼网格
	 * @param FromSectionName 当前Section名称
	 * @param ToSectionName 下一个Section名称
	 */
	virtual void CurrentMontageSetNextSectionNameForMesh(USkeletalMeshComponent* InMesh, FName FromSectionName, FName ToSectionName);

	/**
	 * 设置指定骨骼网格上蒙太奇的播放速率
	 * @param InMesh 目标骨骼网格
	 * @param InPlayRate 新的播放速率
	 */
	virtual void CurrentMontageSetPlayRateForMesh(USkeletalMeshComponent* InMesh, float InPlayRate);

	/**
	 * 检查指定技能是否正在任意骨骼网格上播放动画
	 * @param Ability 要检查的技能
	 * @return 如果技能正在播放动画返回true
	 */
	bool IsAnimatingAbilityForAnyMesh(UGameplayAbility* Ability) const;

	/**
	 * 获取任意骨骼网格上当前正在播放动画的技能
	 * @return 正在播放动画的技能，如果没有则返回nullptr
	 */
	UGameplayAbility* GetAnimatingAbilityFromAnyMesh();

	/**
	 * 获取所有骨骼网格上当前正在播放的蒙太奇列表
	 * @return 正在播放的蒙太奇数组
	 */
	TArray<UAnimMontage*> GetCurrentMontages() const;

	/**
	 * 获取指定骨骼网格上当前正在播放的蒙太奇
	 * @param InMesh 目标骨骼网格
	 * @return 正在播放的蒙太奇，如果没有则返回nullptr
	 */
	UAnimMontage* GetCurrentMontageForMesh(USkeletalMeshComponent* InMesh);

	/**
	 * 获取指定骨骼网格上当前蒙太奇的Section ID
	 * @param InMesh 目标骨骼网格
	 * @return Section ID，INDEX_NONE表示无效
	 */
	int32 GetCurrentMontageSectionIDForMesh(USkeletalMeshComponent* InMesh);

	/**
	 * 获取指定骨骼网格上当前蒙太奇的Section名称
	 * @param InMesh 目标骨骼网格
	 * @return Section名称，NAME_None表示无效
	 */
	FName GetCurrentMontageSectionNameForMesh(USkeletalMeshComponent* InMesh);

	/**
	 * 获取指定骨骼网格上当前Section的时长
	 * @param InMesh 目标骨骼网格
	 * @return Section时长（秒），0表示无效
	 */
	float GetCurrentMontageSectionLengthForMesh(USkeletalMeshComponent* InMesh);

	/**
	 * 获取指定骨骼网格上当前Section的剩余时间
	 * @param InMesh 目标骨骼网格
	 * @return 剩余时间（秒），-1表示无效
	 */
	float GetCurrentMontageSectionTimeLeftForMesh(USkeletalMeshComponent* InMesh);
	
protected:
	// ----------------------------------------------------------------------------------------------------------------
	//	多骨骼网格蒙太奇支持 - 内部实现
	//  以下是网络复制的核心数据结构和处理函数
	// ----------------------------------------------------------------------------------------------------------------	

	/**
	 * 标记是否有待处理的蒙太奇复制数据
	 * 当收到复制数据但AnimInstance尚未准备好时设为true
	 * 后续AnimInstance准备好后会重新处理
	 */
	UPROPERTY()
	bool bPendingMontageRepForMesh;

	/**
	 * 本地蒙太奇信息数组
	 * 
	 * 存储本地发起的蒙太奇播放信息：
	 * - 服务器：存储所有蒙太奇信息
	 * - 客户端：存储预测播放的蒙太奇信息
	 * 
	 * 每个骨骼网格最多一个元素
	 * 不进行网络复制，仅本地使用
	 */
	UPROPERTY()
	TArray<FGameplayAbilityLocalAnimMontageForMesh> LocalAnimMontageInfoForMeshes;

	/**
	 * 复制蒙太奇信息数组
	 * 
	 * 用于将蒙太奇信息从服务器复制到模拟客户端
	 * 使用ReplicatedUsing指定复制回调函数
	 * 
	 * 每个骨骼网格最多一个元素
	 * 包含：Animation、PlayRate、Position、IsStopped、NextSectionID等
	 */
	UPROPERTY(ReplicatedUsing = OnRep_ReplicatedAnimMontageForMesh)
	TArray<FGameplayAbilityRepAnimMontageForMesh> RepAnimMontageInfoForMeshes;

	/**
	 * 获取或创建指定骨骼网格的本地蒙太奇信息
	 * @param InMesh 目标骨骼网格
	 * @return 本地蒙太奇信息的引用
	 */
	FGameplayAbilityLocalAnimMontageForMesh& GetLocalAnimMontageInfoForMesh(USkeletalMeshComponent* InMesh);
	
	/**
	 * 获取或创建指定骨骼网格的复制蒙太奇信息
	 * @param InMesh 目标骨骼网格
	 * @return 复制蒙太奇信息的引用
	 */
	FGameplayAbilityRepAnimMontageForMesh& GetGameplayAbilityRepAnimMontageForMesh(USkeletalMeshComponent* InMesh);

	/**
	 * 预测蒙太奇被服务器拒绝时的回调
	 * 
	 * 当客户端预测播放的蒙太奇被服务器拒绝时调用
	 * 停止本地播放的预测蒙太奇，实现回滚
	 * 
	 * @param InMesh 目标骨骼网格
	 * @param PredictiveMontage 被拒绝的预测蒙太奇
	 */
	void OnPredictiveMontageRejectedForMesh(USkeletalMeshComponent* InMesh, UAnimMontage* PredictiveMontage);

	/**
	 * 更新指定骨骼网格的复制蒙太奇数据
	 * 
	 * 从LocalAnimMontageInfo复制数据到RepAnimMontageInfo
	 * 仅在服务器(Authority)上调用
	 * 
	 * @param InMesh 目标骨骼网格
	 */
	void AnimMontage_UpdateReplicatedDataForMesh(USkeletalMeshComponent* InMesh);
	
	/**
	 * 更新复制蒙太奇数据（重载版本）
	 * @param OutRepAnimMontageInfo 要更新的复制蒙太奇信息
	 */
	void AnimMontage_UpdateReplicatedDataForMesh(FGameplayAbilityRepAnimMontageForMesh& OutRepAnimMontageInfo);

	/**
	 * 更新复制蒙太奇的强制播放标志
	 * 用于处理重复动画数据的播放标志复制
	 * @param OutRepAnimMontageInfo 要更新的复制蒙太奇信息
	 */
	void AnimMontage_UpdateForcedPlayFlagsForMesh(FGameplayAbilityRepAnimMontageForMesh& OutRepAnimMontageInfo);	

	/**
	 * 复制蒙太奇数据的OnRep回调
	 * 
	 * 当RepAnimMontageInfoForMeshes被复制到客户端时自动调用
	 * 负责在模拟客户端上播放/同步蒙太奇：
	 * 1. 检测是否需要播放新蒙太奇
	 * 2. 同步播放速率
	 * 3. 处理停止状态
	 * 4. 进行位置校正（防止漂移）
	 */
	UFUNCTION()
	virtual void OnRep_ReplicatedAnimMontageForMesh();

	/**
	 * 检查是否准备好处理复制的蒙太奇数据
	 * 子类可以覆盖此函数添加额外检查（如"皮肤是否已应用"）
	 * @return 如果准备好返回true
	 */
	virtual bool IsReadyForReplicatedMontageForMesh();

	///////////////// 蒙太奇操作的Server RPC /////////////////
	// 以下RPC用于客户端通知服务器蒙太奇操作
	// 服务器执行后会更新复制属性，同步到所有模拟客户端

	/**
	 * Server RPC：设置蒙太奇的下一个Section
	 * 从CurrentMontageSetNextSectionName调用，复制到其他客户端
	 */
	UFUNCTION(Reliable, Server, WithValidation)
	void ServerCurrentMontageSetNextSectionNameForMesh(USkeletalMeshComponent* InMesh, UAnimMontage* ClientAnimMontage, float ClientPosition, FName SectionName, FName NextSectionName);
	void ServerCurrentMontageSetNextSectionNameForMesh_Implementation(USkeletalMeshComponent* InMesh, UAnimMontage* ClientAnimMontage, float ClientPosition, FName SectionName, FName NextSectionName);
	bool ServerCurrentMontageSetNextSectionNameForMesh_Validate(USkeletalMeshComponent* InMesh, UAnimMontage* ClientAnimMontage, float ClientPosition, FName SectionName, FName NextSectionName);

	/**
	 * Server RPC：跳转到蒙太奇的指定Section
	 * 从CurrentMontageJumpToSection调用，复制到其他客户端
	 */
	UFUNCTION(Reliable, Server, WithValidation)
	void ServerCurrentMontageJumpToSectionNameForMesh(USkeletalMeshComponent* InMesh, UAnimMontage* ClientAnimMontage, FName SectionName);
	void ServerCurrentMontageJumpToSectionNameForMesh_Implementation(USkeletalMeshComponent* InMesh, UAnimMontage* ClientAnimMontage, FName SectionName);
	bool ServerCurrentMontageJumpToSectionNameForMesh_Validate(USkeletalMeshComponent* InMesh, UAnimMontage* ClientAnimMontage, FName SectionName);

	/**
	 * Server RPC：设置蒙太奇的播放速率
	 * 从CurrentMontageSetPlayRate调用，复制到其他客户端
	 */
	UFUNCTION(Reliable, Server, WithValidation)
	void ServerCurrentMontageSetPlayRateForMesh(USkeletalMeshComponent* InMesh, UAnimMontage* ClientAnimMontage, float InPlayRate);
	void ServerCurrentMontageSetPlayRateForMesh_Implementation(USkeletalMeshComponent* InMesh, UAnimMontage* ClientAnimMontage, float InPlayRate);
	bool ServerCurrentMontageSetPlayRateForMesh_Validate(USkeletalMeshComponent* InMesh, UAnimMontage* ClientAnimMontage, float InPlayRate);
	//////////////// ~蒙太奇动画播放扩展功能 ////////////////
};
