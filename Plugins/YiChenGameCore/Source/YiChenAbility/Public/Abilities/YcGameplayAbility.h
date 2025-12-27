// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "YcAbilitySourceInterface.h"
#include "Abilities/GameplayAbility.h"
#include "YcGameplayAbility.generated.h"

class UYcAbilityCost;

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
 * 技能激活组枚举
 * 定义技能激活时与其他技能的关系
 */
UENUM(BlueprintType)
enum class EYcAbilityActivationGroup : uint8
{
	/** 技能独立运行，不受其他技能影响 */
	Independent,

	/** 技能可被其他排他性技能取消并替换 */
	Exclusive_Replaceable,

	/** 技能会阻止所有其他排他性技能激活 */
	Exclusive_Blocking,

	MAX UMETA(Hidden)
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
	
	/**
	 * 检查技能是否可以激活（覆盖父类）
	 * 在父类检查的基础上，额外检查激活组是否被阻止
	 * @param Handle 技能规格句柄
	 * @param ActorInfo Actor信息
	 * @param SourceTags 源标签容器
	 * @param TargetTags 目标标签容器
	 * @param OptionalRelevantTags 可选的输出相关标签容器
	 * @return 如果技能可以激活则返回true，否则返回false
	 */
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	
	/**
	 * 设置技能是否可被取消（覆盖父类）
	 * 如果技能属于Exclusive_Replaceable组，则不能设置为不可取消
	 * @param bCanBeCanceled 是否可被取消
	 */
	virtual void SetCanBeCanceled(bool bCanBeCanceled) override;
	/**
	 * 技能被授予时的回调（覆盖父类）
	 * 当技能被添加到ASC时调用，会触发K2_OnAbilityAdded蓝图事件
	 * 如果技能的激活策略为OnSpawn，会尝试在生成时激活技能
	 * @param ActorInfo 技能所有者的Actor信息
	 * @param Spec 技能规格
	 */
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	
	/**
	 * 技能被移除时的回调（覆盖父类）
	 * 当技能从ASC中移除时调用，会触发K2_OnAbilityRemoved蓝图事件
	 * 注意：蓝图事件在调用父类之前触发，确保在清理前可以访问技能状态
	 * @param ActorInfo 技能所有者的Actor信息
	 * @param Spec 技能规格
	 */
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	
	/**
	 * 检查技能是否满足标签要求（覆盖父类）
	 * 通过ASC处理死亡排除和能力标签扩展的专用版本
	 * 检查流程：
	 *   1. 检查技能的AssetTags是否被ASC阻止
	 *   2. 通过ASC扩展技能标签，获取额外的必需/阻止标签（来自TagRelationshipMapping）
	 *   3. 检查ASC的标签是否包含所有必需标签且不包含任何阻止标签
	 *   4. 检查源标签（SourceTags）是否满足SourceRequiredTags和SourceBlockedTags要求
	 *   5. 检查目标标签（TargetTags）是否满足TargetRequiredTags和TargetBlockedTags要求
	 *   6. 如果玩家已死亡且因阻止标签被拒绝，会添加Ability_ActivateFail_IsDead标签到OptionalRelevantTags
	 * @param AbilitySystemComponent 技能系统组件
	 * @param SourceTags 可选的源标签容器（技能释放者的标签）
	 * @param TargetTags 可选的目标标签容器（技能作用目标的标签）
	 * @param OptionalRelevantTags 可选的输出相关标签容器，用于添加失败原因标签
	 * @return 如果技能满足所有标签要求则返回true，否则返回false
	 */
	virtual bool DoesAbilitySatisfyTagRequirements(const UAbilitySystemComponent& AbilitySystemComponent, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	
	/**
	 * 创建游戏效果上下文（覆盖父类）
	 * 使用自定义的FYcGameplayEffectContext，扩展了SourceObject信息
	 * 会设置以下信息：
	 *   - 技能源（AbilitySource）和源等级（SourceLevel）
	 *   - 效果施加者（Instigator）和效果原因者（EffectCauser）
	 *   - 源对象（SourceObject）
	 * @param Handle 技能规格句柄
	 * @param ActorInfo Actor信息
	 * @return 创建的游戏效果上下文句柄
	 */
	virtual FGameplayEffectContextHandle MakeEffectContext(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const override;
	/**
	 * 检查技能成本（覆盖父类）
	 * 先调用父类的CheckCost检查基础成本，然后遍历AdditionalCosts检查所有额外成本
	 * 如果任何成本检查失败，则返回false
	 * @param Handle 技能规格句柄
	 * @param ActorInfo Actor信息
	 * @param OptionalRelevantTags 可选的输出相关标签容器，可用于添加失败原因标签
	 * @return 如果所有成本都可以支付则返回true，否则返回false
	 */
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	
	/**
	 * 应用技能成本（覆盖父类）
	 * 先调用父类的ApplyCost应用基础成本，然后遍历AdditionalCosts应用所有额外成本
	 * 对于标记为bOnlyApplyCostOnHit的成本，只有在技能成功命中目标时才会应用
	 * @param Handle 技能规格句柄
	 * @param ActorInfo Actor信息
	 * @param ActivationInfo 技能激活信息
	 */
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	
	/** Call notify callbacks above */
	
	
	//~End of UGameplayAbility interface
	
	/**
	 * 获取技能的源信息（等级、源对象、效果施加者）
	 * @param Handle 技能规格句柄
	 * @param ActorInfo Actor信息
	 * @param OutSourceLevel 输出：源等级
	 * @param OutAbilitySource 输出：技能源接口
	 * @param OutEffectCauser 输出：效果施加者Actor
	 */
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
	
	/**
	 * 检查是否可以切换到指定的激活组
	 * 检查技能是否已实例化、是否处于激活状态，以及目标组是否被阻止
	 * @param NewGroup 目标激活组
	 * @return 如果可以切换则返回true，否则返回false
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "YcGameCore|Ability", Meta = (ExpandBoolAsExecs = "ReturnValue"))
	bool CanChangeActivationGroup(EYcAbilityActivationGroup NewGroup) const;

	/**
	 * 尝试切换激活组
	 * 从当前组移除技能，然后添加到新组
	 * @param NewGroup 目标激活组
	 * @return 如果成功切换则返回true，否则返回false
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "YcGameCore|Ability", Meta = (ExpandBoolAsExecs = "ReturnValue"))
	bool ChangeActivationGroup(EYcAbilityActivationGroup NewGroup);
	
	/** 
	 * 当技能激活失败时的事件处理函数
	 * 由ASC组件在技能激活失败时调用，会通过ClientRPC让这个事件在控制端得到触发
	 * 会依次调用C++原生实现（NativeOnAbilityFailedToActivate）和蓝图实现（K2_OnAbilityFailedToActivate）
	 * C++实现会查找FailureTagToUserFacingMessages映射表，如果找到匹配的失败标签则通过GameplayMessageSubsystem广播用户友好的失败消息
	 * 蓝图实现可用于自定义额外的失败处理逻辑（如播放音效、显示特效等）
	 * @param FailedReason 导致技能激活失败的标签容器
	 */
	void OnAbilityFailedToActivate(const FGameplayTagContainer& FailedReason) const
	{
		NativeOnAbilityFailedToActivate(FailedReason);
		K2_OnAbilityFailedToActivate(FailedReason);
	}
	
	/**
	 * 技能激活失败时的C++原生实现（覆盖此函数以自定义失败处理逻辑）
	 * 会遍历FailedReason中的标签，查找FailureTagToUserFacingMessages映射表
	 * 如果找到匹配的标签，则创建FYcAbilitySimpleFailureMessage消息并通过GameplayMessageSubsystem广播
	 * 消息会包含PlayerController、FailureTags和UserFacingReason信息，供UI系统显示用户友好的提示
	 * @param FailedReason 导致技能激活失败的标签容器
	 */
	virtual void NativeOnAbilityFailedToActivate(const FGameplayTagContainer& FailedReason) const;

	/**
	 * 技能激活失败时的蓝图实现事件
	 * 在NativeOnAbilityFailedToActivate之后调用，允许蓝图响应技能激活失败事件
	 * 可用于实现自定义的失败处理逻辑，如播放音效、显示特效、更新UI等
	 * @param FailedReason 导致技能激活失败的标签容器
	 */
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "OnAbilityFailedToActivate")
	void K2_OnAbilityFailedToActivate(const FGameplayTagContainer& FailedReason) const;
	
protected:
	/**
	 * 技能的激活策略
	 * 定义技能的激活时机：OnInputTriggered（输入触发）、WhileInputActive（输入保持）或OnSpawn（生成时）
	 * 在编辑器中可编辑，蓝图只读
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "YcGameCore|Ability Activation", meta=(AllowPrivateAccess="true"))
	EYcAbilityActivationPolicy ActivationPolicy;
	
	/**
	 * 技能的激活组
	 * 定义该技能激活时与其他技能的关系（独立、可替换的排他性、阻塞性排他性）
	 * 在编辑器中可编辑，蓝图只读
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "YcGameCore|Ability Activation")
	EYcAbilityActivationGroup ActivationGroup;
	
	/**
	 * 激活此技能时必须支付的额外成本列表
	 * 在技能激活前会检查所有额外成本，激活后会应用所有额外成本
	 * 支持多种成本类型（如弹药、充能、体力等），每个成本可以独立配置是否只在命中时应用
	 * 在编辑器中可编辑，支持内联编辑（EditInlineNew）
	 */
	UPROPERTY(EditDefaultsOnly, Instanced, Category = Costs)
	TArray<TObjectPtr<UYcAbilityCost>> AdditionalCosts;
	
	/**
	 * 失败标签到用户友好消息的映射表
	 * 用于将技能激活失败的技术性标签（如Ability.ActivateFail.Cost）映射为面向用户的提示文本（如"弹药不足"）
	 * 当技能激活失败时，NativeOnAbilityFailedToActivate会查找此映射表，如果找到匹配的标签则通过GameplayMessageSubsystem广播消息
	 * 在编辑器中可编辑，支持为每个失败标签配置对应的用户提示文本
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Advanced")
	TMap<FGameplayTag, FText> FailureTagToUserFacingMessages;
	
public:
	/**
	 * 获取技能的激活策略
	 * @return 该技能的激活策略
	 */
	FORCEINLINE EYcAbilityActivationPolicy GetActivationPolicy() const { return ActivationPolicy; }
	
	/**
	 * 获取技能的激活组
	 * @return 该技能的激活组
	 */
	FORCEINLINE EYcAbilityActivationGroup GetActivationGroup() const { return ActivationGroup; }
};