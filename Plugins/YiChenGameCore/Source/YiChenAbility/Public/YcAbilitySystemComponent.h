// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "AbilitySystemComponent.h"
#include "Abilities/YcGameplayAbility.h"
#include "YcAbilitySystemComponent.generated.h"

class UYcGameplayAbility;
class UYcAbilityTagRelationshipMapping;

/**
 * 插件提供的技能组件基类
 */
UCLASS(ClassGroup=(YiChenGameCore), meta=(BlueprintSpawnableComponent))
class YICHENABILITY_API UYcAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()
public:
	/**
	 * 构造函数
	 * @param ObjectInitializer 对象初始化器
	 */
	UYcAbilitySystemComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
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
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities", Meta = (DisplayName = "GetTagCount", ScriptName = "GetTagCount"))
	int32 K2_GetTagCount(FGameplayTag TagToCheck) const;
	
	/**
	 * 添加一个Loose GameplayTag（蓝图接口）
	 * Loose Tag不会被网络复制，仅在本地存在
	 * @param GameplayTag 要添加的GameplayTag
	 * @param Count 添加的数量，默认为1。如果Count > 1，则该标签会被堆叠
	 */
	UFUNCTION(BlueprintCallable, Category = "Abilities", Meta = (DisplayName = "AddLooseGameplayTag"))
	void K2_AddLooseGameplayTag(const FGameplayTag& GameplayTag, int32 Count = 1);

	/**
	 * 批量添加Loose GameplayTags（蓝图接口）
	 * Loose Tags不会被网络复制，仅在本地存在
	 * @param GameplayTags 要添加的GameplayTag容器
	 * @param Count 每个标签添加的数量，默认为1
	 */
	UFUNCTION(BlueprintCallable, Category = "Abilities", Meta = (DisplayName = "AddLooseGameplayTags"))
	void K2_AddLooseGameplayTags(const FGameplayTagContainer& GameplayTags, int32 Count = 1);

	/**
	 * 移除一个Loose GameplayTag（蓝图接口）
	 * Loose Tag不会被网络复制，仅在本地存在
	 * @param GameplayTag 要移除的GameplayTag
	 * @param Count 移除的数量，默认为1。如果该标签的计数大于Count，则只减少计数；否则完全移除该标签
	 */
	UFUNCTION(BlueprintCallable, Category = "Abilities", Meta = (DisplayName = "RemoveLooseGameplayTag"))
	void K2_RemoveLooseGameplayTag(const FGameplayTag& GameplayTag, int32 Count = 1);

	/**
	 * 批量移除Loose GameplayTags（蓝图接口）
	 * Loose Tags不会被网络复制，仅在本地存在
	 * @param GameplayTags 要移除的GameplayTag容器
	 * @param Count 每个标签移除的数量，默认为1
	 */
	UFUNCTION(BlueprintCallable, Category = "Abilities", Meta = (DisplayName = "RemoveLooseGameplayTags"))
	void K2_RemoveLooseGameplayTags(const FGameplayTagContainer& GameplayTags, int32 Count = 1);
	
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
};
