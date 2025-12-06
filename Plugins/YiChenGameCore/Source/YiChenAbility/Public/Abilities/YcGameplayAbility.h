// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Abilities/GameplayAbility.h"
#include "YcGameplayAbility.generated.h"

/**
 * 技能激活策略枚举
 * 定义技能的激活时机和方式
 */
UENUM(BlueprintType)
enum class EYcAbilityActivationPolicy : uint8
{
	/**
	 * 当输入被触发时激活技能
	 * 行为说明：
	 *   - 输入按下：激活技能。按下后不松开，技能执行完也不会再次激活
	 *   - 输入松开：如果技能还在激活状态，触发WaitInputRelease事件
	 *   - 单次点击（按下+松开）：
	 *     * 若技能已激活：触发WaitInputPress事件
	 *     * 若技能未激活：尝试激活技能，并触发WaitInputRelease事件
	 */
	OnInputTriggered,

	/**
	 * 当输入保持时持续尝试激活技能
	 * 行为说明：
	 *   - 输入按住：每一帧都检测技能是否激活，如果未激活则激活
	 *   - 输入松开：如果技能还在激活状态，触发WaitInputRelease事件
	 *   - 特点：会在输入按住期间反复激活技能。技能执行完后，下一帧会再次尝试激活
	 *   - 注意：该策略不会触发WaitInputPress事件，只有WaitInputRelease会被触发
	 */
	WhileInputActive,

	/**
	 * 当Avatar被分配时激活技能
	 * 行为说明：
	 *   - 在Pawn Avatar生成或分配给ASC时自动激活
	 *   - 激活前会检查网络执行策略和权限
	 *   - 仅在本地控制的客户端或服务器上激活（取决于NetExecutionPolicy）
	 */
	OnSpawn
};

/**
 * 插件提供的技能基类, 与插件提供的YcAbilitySystemComponent配合使用
 */
UCLASS(Abstract, HideCategories = Input, Meta = (ShortTooltip = "The base gameplay ability class used by this project."))
class YICHENABILITY_API UYcGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
	friend class UYcAbilitySystemComponent;
	
public:
	UYcGameplayAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	/**
	 * 从ActorInfo中获取UYcAbilitySystemComponent
	 * 将父类的AbilitySystemComponent转换为UYcAbilitySystemComponent类型
	 * @return 当前技能的ASC，如果ActorInfo无效则返回nullptr
	 */
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|Ability")
	UYcAbilitySystemComponent* GetYcAbilitySystemComponentFromActorInfo() const;
	
	/**
	 * 从ActorInfo中获取Controller
	 * 首先尝试获取PlayerController，如果不存在则在所有者链中查找
	 * 查找顺序：PlayerController -> OwnerActor -> OwnerActor的Owner链 -> Pawn的Controller
	 * @return 找到的Controller，如果不存在则返回nullptr
	 */
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|Ability")
	AController* GetControllerFromActorInfo() const;
	
protected:
	//~UGameplayAbility interface

	/** 技能被授予时的回调，触发K2_OnAbilityAdded蓝图事件和尝试在生成时激活技能 */
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	/** 技能被移除时的回调，触发K2_OnAbilityRemoved蓝图事件 */
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	/** 检查技能是否满足标签要求（无阻挡标签、有所有必需标签） */
	virtual bool DoesAbilitySatisfyTagRequirements(const UAbilitySystemComponent& AbilitySystemComponent, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	/** 重写MakeEffectContext, 用我们自定义版本的, 扩展了SourceObject信息 */
	virtual FGameplayEffectContextHandle MakeEffectContext(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const override;
	//~End of UGameplayAbility interface
	
	// 获取技能的源信息（等级、源对象、效果施加者）
	virtual void GetAbilitySource(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, float& OutSourceLevel, const IYcAbilitySourceInterface*& OutAbilitySource, AActor*& OutEffectCauser) const;
	
	/**
	 * 当Pawn Avatar被设置时的回调
	 * 由UYcAbilitySystemComponent在InitAbilityActorInfo中调用
	 * 用于通知技能Avatar已发生变化，会触发K2_OnPawnAvatarSet蓝图事件
	 * 可用于初始化与Avatar相关的技能逻辑
	 */
	virtual void OnPawnAvatarSet();
	
	/**
	 * 尝试在生成时激活技能
	 * 检查激活策略是否为OnSpawn，如果是则根据网络执行策略和权限决定是否激活
	 * 激活前会检查：
	 *   - ActorInfo有效性
	 *   - 技能未处于激活状态
	 *   - 不在预测模式中
	 *   - Avatar未被标记为TearOff且生命周期有效
	 * @param ActorInfo 技能所有者的Actor信息
	 * @param Spec 技能规格
	 */
	void TryActivateAbilityOnSpawn(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) const;
	
	/**
	 * 外部结束技能
	 * 与K2_EndAbility等效，用于RPC批处理系统外部结束技能
	 * 会调用EndAbility并设置bReplicateEndAbility为true，确保网络同步
	 */
	virtual void ExternalEndAbility();
	
	/**
	 * 技能被授予时的蓝图事件
	 * 在OnGiveAbility中调用，允许蓝图响应技能添加事件
	 * 可用于初始化技能相关的UI、音效等
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = Ability, DisplayName = "OnAbilityAdded")
	void K2_OnAbilityAdded();

	/**
	 * 技能被移除时的蓝图事件
	 * 在OnRemoveAbility中调用，允许蓝图响应技能移除事件
	 * 可用于清理技能相关的资源、UI等
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = Ability, DisplayName = "OnAbilityRemoved")
	void K2_OnAbilityRemoved();

	/**
	 * 当Pawn Avatar被设置时的蓝图事件
	 * 在OnPawnAvatarSet中调用，允许蓝图响应Avatar变化事件
	 * 可用于初始化与Avatar相关的技能逻辑、动画、特效等
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = Ability, DisplayName = "OnPawnAvatarSet")
	void K2_OnPawnAvatarSet();
	
private:
	/**
	 * 技能的激活策略
	 * 定义技能的激活时机：OnInputTriggered（输入触发）、WhileInputActive（输入保持）或OnSpawn（生成时）
	 * 在编辑器中可编辑，蓝图只读
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "YcGameCore|Ability Activation", meta=(AllowPrivateAccess="true"))
	EYcAbilityActivationPolicy ActivationPolicy;
	
public:
	/**
	 * 获取技能的激活策略
	 * @return 该技能的激活策略
	 */
	FORCEINLINE EYcAbilityActivationPolicy GetActivationPolicy() const { return ActivationPolicy; }
};