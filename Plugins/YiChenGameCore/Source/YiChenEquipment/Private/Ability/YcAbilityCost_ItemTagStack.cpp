// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Ability/YcAbilityCost_ItemTagStack.h"
#include "NativeGameplayTags.h"
#include "YcGameplayAbility_FromEquipment.h"
#include "YcInventoryItemInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcAbilityCost_ItemTagStack)

UE_DEFINE_GAMEPLAY_TAG(TAG_ABILITY_FAIL_COST, "Ability.ActivateFail.Cost");

UYcAbilityCost_ItemTagStack::UYcAbilityCost_ItemTagStack()
{
	// 默认消耗1个标签堆栈
	Quantity.SetValue(1.0f);
	// 默认失败标签
	FailureTag = TAG_ABILITY_FAIL_COST;
}

bool UYcAbilityCost_ItemTagStack::CheckCost(const UYcGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags)
{
	// 检查技能是否是从装备物品授予的技能
	const UYcGameplayAbility_FromEquipment* EquipmentAbility = Cast<const UYcGameplayAbility_FromEquipment>(Ability);
	if (!EquipmentAbility) return false;
	
	// 获取关联的物品实例
	const UYcInventoryItemInstance* ItemInstance = EquipmentAbility->GetAssociatedItem();
	if (!ItemInstance) return false;
	
	// 根据技能等级计算需要消耗的堆栈数量
	const int32 AbilityLevel = Ability->GetAbilityLevel(Handle, ActorInfo);
	const float NumStacksReal = Quantity.GetValueAtLevel(AbilityLevel);
	const int32 NumStacks = FMath::TruncToInt(NumStacksReal);
	
	// 检查物品实例上该标签的堆栈数量是否足够
	const bool bCanApplyCost = ItemInstance->GetStatTagStackCount(Tag) >= NumStacks;

	// 如果成本不足，添加失败标签到OptionalRelevantTags，用于通知其他系统失败原因
	if (!bCanApplyCost && OptionalRelevantTags && FailureTag.IsValid())
	{
		OptionalRelevantTags->AddTag(FailureTag);				
	}
	
	return bCanApplyCost;
}

void UYcAbilityCost_ItemTagStack::ApplyCost(const UYcGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	// 仅在服务器端执行
	if (!ActorInfo->IsNetAuthority()) return;
	
	// 检查技能是否是从装备物品授予的技能
	const UYcGameplayAbility_FromEquipment* EquipmentAbility = Cast<const UYcGameplayAbility_FromEquipment>(Ability);
	if (!EquipmentAbility) return;
	
	// 获取关联的物品实例（需要非const版本以修改标签堆栈）
	UYcInventoryItemInstance* ItemInstance = EquipmentAbility->GetAssociatedItem();
	if (!ItemInstance) return;
	
	// 根据技能等级计算需要消耗的堆栈数量
	const int32 AbilityLevel = Ability->GetAbilityLevel(Handle, ActorInfo);
	const float NumStacksReal = Quantity.GetValueAtLevel(AbilityLevel);
	const int32 NumStacks = FMath::TruncToInt(NumStacksReal);

	// 从物品实例上移除指定数量的标签堆栈
	ItemInstance->RemoveStatTagStack(Tag, NumStacks);
}