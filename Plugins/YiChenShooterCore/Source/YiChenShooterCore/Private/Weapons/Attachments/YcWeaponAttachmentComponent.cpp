// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Weapons/Attachments/YcWeaponAttachmentComponent.h"

#include "YiChenShooterCore.h"
#include "DataRegistrySubsystem.h"
#include "Weapons/Attachments/YcFragment_WeaponAttachments.h"
#include "Weapons/YcHitScanWeaponInstance.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcWeaponAttachmentComponent)

UYcWeaponAttachmentComponent::UYcWeaponAttachmentComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

void UYcWeaponAttachmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UYcWeaponAttachmentComponent, InstalledAttachments);
}

void UYcWeaponAttachmentComponent::Initialize(UYcHitScanWeaponInstance* InWeaponInstance, 
	const FYcFragment_WeaponAttachments& AttachmentsFragment)
{
	WeaponInstance = InWeaponInstance;
	AttachmentsConfig = &AttachmentsFragment;
	bInitialized = true;
	
	// 预分配槽位
	InstalledAttachments.SetNum(AttachmentsFragment.AttachmentSlots.Num());
	
	// 初始化每个槽位的SlotType
	for (int32 i = 0; i < AttachmentsFragment.AttachmentSlots.Num(); ++i)
	{
		InstalledAttachments[i].SlotType = AttachmentsFragment.AttachmentSlots[i].SlotType;
	}
	
	// 收集所有要安装的默认配件
	TArray<TPair<int32, FDataRegistryId>> DefaultAttachmentsToInstall;
	for (int32 i = 0; i < AttachmentsFragment.AttachmentSlots.Num(); ++i)
	{
		const FYcAttachmentSlotDef& SlotDef = AttachmentsFragment.AttachmentSlots[i];
		if (SlotDef.DefaultAttachment.IsValid())
		{
			DefaultAttachmentsToInstall.Add(TPair<int32, FDataRegistryId>(i, SlotDef.DefaultAttachment));
		}
	}
	
	// 按顺序安装默认配件，检查互斥和前置条件
	for (const auto& Pair : DefaultAttachmentsToInstall)
	{
		const int32 SlotIndex = Pair.Key;
		const FDataRegistryId& AttachmentId = Pair.Value;
		const FGameplayTag SlotType = AttachmentsFragment.AttachmentSlots[SlotIndex].SlotType;
		
		// 获取配件定义
		const FYcAttachmentDefinition* AttachmentDef = GetAttachmentDefinition(AttachmentId);
		if (!AttachmentDef)
		{
			UE_LOG(LogYcShooterCore, Warning, TEXT("Initialize: Cannot find default attachment '%s' in slot '%s'"), 
				*AttachmentId.ToString(), 
				*SlotType.ToString());
			continue;
		}
		
		FText Reason;
		if (CanInstallAttachment(AttachmentId, Reason))
		{
			FYcAttachmentInstance& Instance = InstalledAttachments[SlotIndex];
			Instance.AttachmentRegistryId = AttachmentId;
			Instance.CachedDef = AttachmentDef;
			
			// 初始化调校值
			if (AttachmentDef->bSupportsTuning)
			{
				for (const FYcAttachmentTuningParam& Param : AttachmentDef->TuningParams)
				{
					Instance.TuningValues.Add(Param.StatTag, Param.DefaultValue);
				}
			}
			
			// 添加配件提供的动态槽位
			if (AttachmentDef->ProvidedSlots.Num() > 0)
			{
				AddDynamicSlots(AttachmentDef, SlotType);
			}
		}
		else
		{
			UE_LOG(LogYcShooterCore, Warning, TEXT("Initialize: Cannot install default attachment '%s' in slot '%s': %s"), 
				*AttachmentId.ToString(), 
				*SlotType.ToString(),
				*Reason.ToString());
		}
	}
}


bool UYcWeaponAttachmentComponent::InstallAttachment(const FDataRegistryId& AttachmentId, 
	FGameplayTag SlotType)
{
	if (!AttachmentId.IsValid())
	{
		UE_LOG(LogYcShooterCore, Warning, TEXT("InstallAttachment: Invalid AttachmentId"));
		return false;
	}
	
	// 获取配件定义
	const FYcAttachmentDefinition* AttachmentDef = GetAttachmentDefinition(AttachmentId);
	if (!AttachmentDef)
	{
		UE_LOG(LogYcShooterCore, Warning, TEXT("InstallAttachment: Attachment not found: %s"), 
			*AttachmentId.ToString());
		return false;
	}
	
	// 如果没有指定槽位，使用配件定义的槽位
	if (!SlotType.IsValid())
	{
		SlotType = AttachmentDef->SlotType;
	}
	
	// 检查是否可以安装
	FText Reason;
	if (!CanInstallAttachment(AttachmentId, Reason))
	{
		UE_LOG(LogYcShooterCore, Warning, TEXT("Cannot install attachment: %s"), *Reason.ToString());
		return false;
	}
	
	// 查找槽位
	const int32 SlotIndex = FindSlotIndex(SlotType);
	if (SlotIndex == INDEX_NONE)
	{
		UE_LOG(LogYcShooterCore, Warning, TEXT("Slot not found: %s"), *SlotType.ToString());
		return false;
	}
	
	// 如果槽位已有配件，先卸载
	if (InstalledAttachments[SlotIndex].IsValid())
	{
		UninstallAttachment(SlotType);
	}
	
	// 安装新配件
	FYcAttachmentInstance& Instance = InstalledAttachments[SlotIndex];
	Instance.AttachmentRegistryId = AttachmentId;
	Instance.CachedDef = AttachmentDef;
	Instance.SlotType = SlotType;
	Instance.TuningValues.Empty();
	
	// 初始化调校值为默认值
	if (AttachmentDef->bSupportsTuning)
	{
		for (const FYcAttachmentTuningParam& Param : AttachmentDef->TuningParams)
		{
			Instance.TuningValues.Add(Param.StatTag, Param.DefaultValue);
		}
	}
	
	// 添加配件提供的动态槽位
	if (AttachmentDef->ProvidedSlots.Num() > 0)
	{
		AddDynamicSlots(AttachmentDef, SlotType);
	}
	
	// 广播事件
	OnAttachmentInstalled.Broadcast(SlotType, AttachmentId);
	
	// 通知属性变化
	NotifyStatsChanged();
	
	return true;
}

bool UYcWeaponAttachmentComponent::UninstallAttachment(FGameplayTag SlotType)
{
	const int32 SlotIndex = FindSlotIndex(SlotType);
	if (SlotIndex == INDEX_NONE)
	{
		return false;
	}
	
	FYcAttachmentInstance& Instance = InstalledAttachments[SlotIndex];
	if (!Instance.IsValid())
	{
		return false;
	}
	
	// 保存配件ID用于后续处理
	const FDataRegistryId OldAttachmentId = Instance.AttachmentRegistryId;
	const FYcAttachmentDefinition* OldDef = Instance.GetDefinition();
	
	// 先移除该配件提供的动态槽位（会级联卸载子配件）
	if (OldDef && OldDef->ProvidedSlots.Num() > 0)
	{
		RemoveDynamicSlots(SlotType);
	}
	
	// 清空槽位
	Instance.Reset();
	Instance.SlotType = SlotType; // 保留槽位类型
	
	// 广播事件（注意：此时配件已从组件中移除，监听者无法通过 GetAttachmentDefInSlot 获取配件定义）
	// 隐藏槽位的恢复应由监听者在收到事件前自行处理，或者通过 RefreshAllAttachmentVisuals 刷新
	OnAttachmentUninstalled.Broadcast(SlotType, OldAttachmentId);
	
	// 通知属性变化
	NotifyStatsChanged();
	
	return true;
}

bool UYcWeaponAttachmentComponent::CanInstallAttachment(const FDataRegistryId& AttachmentId, 
	FText& OutReason) const
{
	if (!AttachmentId.IsValid())
	{
		OutReason = FText::FromString(TEXT("配件ID无效"));
		return false;
	}
	
	// 获取配件定义
	const FYcAttachmentDefinition* AttachmentDef = GetAttachmentDefinition(AttachmentId);
	if (!AttachmentDef)
	{
		OutReason = FText::FromString(TEXT("配件定义未找到"));
		return false;
	}
	
	if (!bInitialized || !AttachmentsConfig)
	{
		OutReason = FText::FromString(TEXT("配件系统未初始化"));
		return false;
	}
	
	// 检查槽位是否存在（基础槽位或动态槽位）
	const FGameplayTag SlotType = AttachmentDef->SlotType;
	if (!IsSlotAvailable(SlotType))
	{
		OutReason = FText::FromString(TEXT("武器不支持此槽位"));
		return false;
	}
	
	// 检查配件数量限制
	if (AttachmentsConfig->MaxAttachments > 0)
	{
		const int32 CurrentCount = GetInstalledAttachmentCount();
		const bool bSlotHasAttachment = HasAttachmentInSlot(SlotType);
		
		// 如果槽位已有配件，替换不增加数量
		if (!bSlotHasAttachment && CurrentCount >= AttachmentsConfig->MaxAttachments)
		{
			OutReason = FText::FromString(FString::Printf(
				TEXT("已达到最大配件数量限制(%d)"), AttachmentsConfig->MaxAttachments));
			return false;
		}
	}
	
	// 检查互斥
	const TArray<FGameplayTag> InstalledIds = GetInstalledAttachmentIds();
	for (const FGameplayTag& InstalledId : InstalledIds)
	{
		if (AttachmentDef->IsIncompatibleWith(InstalledId))
		{
			OutReason = FText::FromString(TEXT("与已安装的配件互斥"));
			return false;
		}
	}
	
	// 检查前置配件
	if (!AttachmentDef->HasRequiredAttachments(InstalledIds))
	{
		OutReason = FText::FromString(TEXT("缺少前置配件"));
		return false;
	}
	
	return true;
}

void UYcWeaponAttachmentComponent::ClearAllAttachments()
{
	for (FYcAttachmentInstance& Instance : InstalledAttachments)
	{
		if (Instance.IsValid())
		{
			const FDataRegistryId OldAttachmentId = Instance.AttachmentRegistryId;
			const FGameplayTag SlotType = Instance.SlotType;
			
			Instance.Reset();
			Instance.SlotType = SlotType; // 保留槽位类型
			
			OnAttachmentUninstalled.Broadcast(SlotType, OldAttachmentId);
		}
	}
	
	// 清空动态槽位
	DynamicSlots.Empty();
	
	NotifyStatsChanged();
}

const FYcAttachmentDefinition* UYcWeaponAttachmentComponent::GetAttachmentDefInSlot(FGameplayTag SlotType) const
{
	const int32 SlotIndex = FindSlotIndex(SlotType);
	if (SlotIndex == INDEX_NONE || !InstalledAttachments[SlotIndex].IsValid()) return nullptr;
	return InstalledAttachments[SlotIndex].GetDefinition();
}

bool UYcWeaponAttachmentComponent::K2_GetAttachmentDefInSlot(FGameplayTag SlotType, FYcAttachmentDefinition& OutDefinition) const
{
	const FYcAttachmentDefinition* Def = GetAttachmentDefInSlot(SlotType);
	if (Def)
	{
		OutDefinition = *Def;
		return true;
	}
	return false;
}

FYcAttachmentInstance UYcWeaponAttachmentComponent::GetAttachmentInSlot(FGameplayTag SlotType) const
{
	const int32 SlotIndex = FindSlotIndex(SlotType);
	return SlotIndex != INDEX_NONE ? InstalledAttachments[SlotIndex] : FYcAttachmentInstance();
}



bool UYcWeaponAttachmentComponent::HasAttachmentInSlot(FGameplayTag SlotType) const
{
	const int32 SlotIndex = FindSlotIndex(SlotType);
	return SlotIndex != INDEX_NONE && InstalledAttachments[SlotIndex].IsValid();
}

int32 UYcWeaponAttachmentComponent::GetInstalledAttachmentCount() const
{
	int32 Count = 0;
	for (const FYcAttachmentInstance& Instance : InstalledAttachments)
	{
		if (Instance.IsValid())
		{
			++Count;
		}
	}
	return Count;
}

TArray<FGameplayTag> UYcWeaponAttachmentComponent::GetInstalledAttachmentIds() const
{
	TArray<FGameplayTag> Result;
	
	for (const FYcAttachmentInstance& Instance : InstalledAttachments)
	{
		if (Instance.IsValid())
		{
			const FYcAttachmentDefinition* Def = Instance.GetDefinition();
			if (Def)
			{
				// 使用配件的 SlotType 作为标识（因为新结构体没有 AttachmentId 字段）
				Result.Add(Def->SlotType);
			}
		}
	}
	
	return Result;
}

TArray<FYcStatModifier> UYcWeaponAttachmentComponent::GetModifiersForStat(FGameplayTag StatTag) const
{
	TArray<FYcStatModifier> Result;
	
	for (const FYcAttachmentInstance& Instance : InstalledAttachments)
	{
		if (!Instance.IsValid()) continue;
		
		const FYcAttachmentDefinition* Def = Instance.GetDefinition();
		if (!Def) continue;
		
		// 收集配件的修改器
		for (const FYcStatModifier& Modifier : Def->StatModifiers)
		{
			if (Modifier.StatTag.MatchesTag(StatTag))
			{
				Result.Add(Modifier);
			}
		}
		
		// 收集调校修改器
		if (Def->bSupportsTuning)
		{
			for (const FYcAttachmentTuningParam& Param : Def->TuningParams)
			{
				if (Param.StatTag.MatchesTag(StatTag))
				{
					const float* TuningValue = Instance.TuningValues.Find(Param.StatTag);
					if (TuningValue && !FMath::IsNearlyZero(*TuningValue))
					{
						FYcStatModifier TuningModifier;
						TuningModifier.StatTag = Param.StatTag;
						TuningModifier.Operation = Param.Operation;
						TuningModifier.Value = *TuningValue;
						TuningModifier.Priority = 100; // 调校修改器优先级较低，最后应用
						Result.Add(TuningModifier);
					}
				}
			}
		}
	}
	
	// 按优先级排序
	Result.Sort([](const FYcStatModifier& A, const FYcStatModifier& B)
	{
		return A.Priority < B.Priority;
	});
	
	return Result;
}


float UYcWeaponAttachmentComponent::CalculateFinalStatValue(FGameplayTag StatTag, float BaseValue) const
{
	const TArray<FYcStatModifier> Modifiers = GetModifiersForStat(StatTag);
	
	if (Modifiers.Num() == 0)
	{
		return BaseValue;
	}
	
	float FinalValue = BaseValue;
	float AddSum = 0.0f;
	float MultiplySum = 0.0f;
	bool bHasOverride = false;
	float OverrideValue = 0.0f;
	
	// 分类收集修改器
	for (const FYcStatModifier& Modifier : Modifiers)
	{
		switch (Modifier.Operation)
		{
		case EYcStatModifierOp::Override:
			// Override取最后一个
			bHasOverride = true;
			OverrideValue = Modifier.Value;
			break;
			
		case EYcStatModifierOp::Add:
			AddSum += Modifier.Value;
			break;
			
		case EYcStatModifierOp::Multiply:
			MultiplySum += Modifier.Value;
			break;
		}
	}
	
	// 应用修改器
	if (bHasOverride)
	{
		// Override直接覆盖基础值
		FinalValue = OverrideValue;
	}
	else
	{
		// 先加法，后乘法
		FinalValue = BaseValue + AddSum;
		FinalValue = FinalValue * (1.0f + MultiplySum);
	}
	
	return FinalValue;
}

TMap<FGameplayTag, float> UYcWeaponAttachmentComponent::CalculateFinalStatValues(
	const TMap<FGameplayTag, float>& StatBaseValues) const
{
	TMap<FGameplayTag, float> Result;
	
	for (const auto& Pair : StatBaseValues)
	{
		Result.Add(Pair.Key, CalculateFinalStatValue(Pair.Key, Pair.Value));
	}
	
	return Result;
}

bool UYcWeaponAttachmentComponent::SetTuningValue(FGameplayTag SlotType, FGameplayTag StatTag, float Value)
{
	const int32 SlotIndex = FindSlotIndex(SlotType);
	if (SlotIndex == INDEX_NONE) return false;
	
	FYcAttachmentInstance& Instance = InstalledAttachments[SlotIndex];
	if (!Instance.IsValid()) return false;
	
	const FYcAttachmentDefinition* Def = Instance.GetDefinition();
	if (!Def || !Def->bSupportsTuning) return false;
	
	// 查找调校参数定义
	const FYcAttachmentTuningParam* TuningParam = Def->TuningParams.FindByPredicate(
		[&StatTag](const FYcAttachmentTuningParam& Param)
		{
			return Param.StatTag.MatchesTagExact(StatTag);
		});
	
	if (!TuningParam) return false;
	
	// 限制值在范围内
	const float ClampedValue = FMath::Clamp(Value, TuningParam->MinValue, TuningParam->MaxValue);
	Instance.TuningValues.Add(StatTag, ClampedValue);
	
	NotifyStatsChanged();
	return true;
}

float UYcWeaponAttachmentComponent::GetTuningValue(FGameplayTag SlotType, FGameplayTag StatTag) const
{
	const int32 SlotIndex = FindSlotIndex(SlotType);
	if (SlotIndex == INDEX_NONE) return 0.0f;
	
	const float* Value = InstalledAttachments[SlotIndex].TuningValues.Find(StatTag);
	return Value ? *Value : 0.0f;
}

void UYcWeaponAttachmentComponent::ResetTuningValues(FGameplayTag SlotType)
{
	const int32 SlotIndex = FindSlotIndex(SlotType);
	if (SlotIndex == INDEX_NONE) return;
	
	FYcAttachmentInstance& Instance = InstalledAttachments[SlotIndex];
	if (!Instance.IsValid()) return;
	
	const FYcAttachmentDefinition* Def = Instance.GetDefinition();
	if (!Def || !Def->bSupportsTuning) return;
	
	// 重置为默认值
	Instance.TuningValues.Empty();
	for (const FYcAttachmentTuningParam& Param : Def->TuningParams)
	{
		Instance.TuningValues.Add(Param.StatTag, Param.DefaultValue);
	}
	
	NotifyStatsChanged();
}

int32 UYcWeaponAttachmentComponent::FindSlotIndex(FGameplayTag SlotType) const
{
	return InstalledAttachments.IndexOfByPredicate([&SlotType](const FYcAttachmentInstance& Instance)
	{
		return Instance.SlotType.MatchesTagExact(SlotType);
	});
}

void UYcWeaponAttachmentComponent::NotifyStatsChanged()
{
	// 通知武器实例重算属性
	if (UYcHitScanWeaponInstance* Weapon = WeaponInstance.Get())
	{
		Weapon->RecalculateStats();
	}
	
	// 广播事件
	OnStatsRecalculated.Broadcast();
}

void UYcWeaponAttachmentComponent::OnRep_InstalledAttachments()
{
	// 客户端收到配件列表更新，通知属性变化
	NotifyStatsChanged();
}

void UYcWeaponAttachmentComponent::CollectAllModifiers(TArray<FYcStatModifier>& OutModifiers) const
{
	for (const FYcAttachmentInstance& Instance : InstalledAttachments)
	{
		if (!Instance.IsValid()) continue;
		
		const FYcAttachmentDefinition* Def = Instance.GetDefinition();
		if (!Def) continue;
		
		// 收集配件的修改器
		OutModifiers.Append(Def->StatModifiers);
		
		// 收集调校修改器
		if (Def->bSupportsTuning)
		{
			for (const FYcAttachmentTuningParam& Param : Def->TuningParams)
			{
				const float* TuningValue = Instance.TuningValues.Find(Param.StatTag);
				if (TuningValue && !FMath::IsNearlyZero(*TuningValue))
				{
					FYcStatModifier TuningModifier;
					TuningModifier.StatTag = Param.StatTag;
					TuningModifier.Operation = Param.Operation;
					TuningModifier.Value = *TuningValue;
					TuningModifier.Priority = 100;
					OutModifiers.Add(TuningModifier);
				}
			}
		}
	}
}

const FYcAttachmentDefinition* UYcWeaponAttachmentComponent::GetAttachmentDefinition(const FDataRegistryId& AttachmentId) const
{
	if (!AttachmentId.IsValid())
	{
		UE_LOG(LogYcShooterCore, Warning, TEXT("GetAttachmentDefinition: Invalid AttachmentId"));
		return nullptr;
	}

	const UDataRegistrySubsystem* Subsystem = UDataRegistrySubsystem::Get();
	if (!Subsystem)
	{
		UE_LOG(LogYcShooterCore, Error, TEXT("GetAttachmentDefinition: DataRegistrySubsystem not available"));
		return nullptr;
	}

	const FYcAttachmentDefinition* Def = Subsystem->GetCachedItem<FYcAttachmentDefinition>(AttachmentId);
	
	if (!Def)
	{
		UE_LOG(LogYcShooterCore, Warning, 
			TEXT("GetAttachmentDefinition: Attachment not found in cache: %s"),
			*AttachmentId.ToString());
	}
	
	return Def;
}

// ════════════════════════════════════════════════════════════════════════
// 槽位查询实现
// ════════════════════════════════════════════════════════════════════════

TArray<FYcAttachmentSlotDef> UYcWeaponAttachmentComponent::GetAllAvailableSlots() const
{
	TArray<FYcAttachmentSlotDef> Result;
	
	// 添加基础槽位
	if (AttachmentsConfig)
	{
		Result.Append(AttachmentsConfig->AttachmentSlots);
	}
	
	// 添加动态槽位
	Result.Append(DynamicSlots);
	
	return Result;
}

TArray<FYcAttachmentSlotDef> UYcWeaponAttachmentComponent::GetBaseSlots() const
{
	if (AttachmentsConfig)
	{
		return AttachmentsConfig->AttachmentSlots;
	}
	return TArray<FYcAttachmentSlotDef>();
}

TArray<FYcAttachmentSlotDef> UYcWeaponAttachmentComponent::GetDynamicSlots() const
{
	return DynamicSlots;
}

bool UYcWeaponAttachmentComponent::IsSlotAvailable(FGameplayTag SlotType) const
{
	// 检查基础槽位
	if (AttachmentsConfig && AttachmentsConfig->HasSlot(SlotType))
	{
		return true;
	}
	
	// 检查动态槽位
	return DynamicSlots.ContainsByPredicate([&SlotType](const FYcAttachmentSlotDef& Slot)
	{
		return Slot.SlotType.MatchesTagExact(SlotType);
	});
}

bool UYcWeaponAttachmentComponent::GetSlotDef(FGameplayTag SlotType, FYcAttachmentSlotDef& OutSlotDef) const
{
	// 先检查基础槽位
	if (AttachmentsConfig)
	{
		for (const FYcAttachmentSlotDef& Slot : AttachmentsConfig->AttachmentSlots)
		{
			if (Slot.SlotType.MatchesTagExact(SlotType))
			{
				OutSlotDef = Slot;
				return true;
			}
		}
	}
	
	// 再检查动态槽位
	for (const FYcAttachmentSlotDef& Slot : DynamicSlots)
	{
		if (Slot.SlotType.MatchesTagExact(SlotType))
		{
			OutSlotDef = Slot;
			return true;
		}
	}
	
	return false;
}

TArray<FYcAttachmentSlotDef> UYcWeaponAttachmentComponent::GetSlotsProvidedByAttachment(FGameplayTag SlotType) const
{
	TArray<FYcAttachmentSlotDef> Result;
	
	for (const FYcAttachmentSlotDef& Slot : DynamicSlots)
	{
		if (Slot.ProviderSlotType.MatchesTagExact(SlotType))
		{
			Result.Add(Slot);
		}
	}
	
	return Result;
}

// ════════════════════════════════════════════════════════════════════════
// 动态槽位管理实现
// ════════════════════════════════════════════════════════════════════════

void UYcWeaponAttachmentComponent::AddDynamicSlots(const FYcAttachmentDefinition* AttachmentDef, FGameplayTag ProviderSlotType)
{
	if (!AttachmentDef || AttachmentDef->ProvidedSlots.Num() == 0)
	{
		return;
	}
	
	// 收集需要安装默认配件的槽位
	TArray<TPair<FGameplayTag, FDataRegistryId>> DefaultAttachmentsToInstall;
	
	for (const FYcAttachmentSlotDef& ProvidedSlot : AttachmentDef->ProvidedSlots)
	{
		// 检查是否已存在相同槽位
		bool bExists = false;
		for (const FYcAttachmentSlotDef& ExistingSlot : DynamicSlots)
		{
			if (ExistingSlot.SlotType.MatchesTagExact(ProvidedSlot.SlotType))
			{
				bExists = true;
				break;
			}
		}
		
		if (!bExists)
		{
			// 创建动态槽位副本并标记
			FYcAttachmentSlotDef NewSlot = ProvidedSlot;
			NewSlot.bIsDynamic = true;
			NewSlot.ProviderSlotType = ProviderSlotType;
			
			DynamicSlots.Add(NewSlot);
			
			// 为动态槽位添加配件实例占位
			FYcAttachmentInstance NewInstance;
			NewInstance.SlotType = NewSlot.SlotType;
			InstalledAttachments.Add(NewInstance);
			
			// 广播槽位可用事件
			OnSlotAvailabilityChanged.Broadcast(NewSlot.SlotType, true);
			
			// 记录需要安装的默认配件
			if (ProvidedSlot.DefaultAttachment.IsValid())
			{
				DefaultAttachmentsToInstall.Add(TPair<FGameplayTag, FDataRegistryId>(NewSlot.SlotType, ProvidedSlot.DefaultAttachment));
			}
			
			UE_LOG(LogYcShooterCore, Log, TEXT("AddDynamicSlots: 添加动态槽位 %s (由 %s 提供)"), 
				*NewSlot.SlotType.ToString(), *ProviderSlotType.ToString());
		}
	}
	
	// 安装动态槽位的默认配件
	for (const auto& Pair : DefaultAttachmentsToInstall)
	{
		InstallAttachment(Pair.Value, Pair.Key);
	}
}

void UYcWeaponAttachmentComponent::RemoveDynamicSlots(FGameplayTag ProviderSlotType)
{
	// 收集要移除的槽位
	TArray<FGameplayTag> SlotsToRemove;
	for (const FYcAttachmentSlotDef& Slot : DynamicSlots)
	{
		if (Slot.ProviderSlotType.MatchesTagExact(ProviderSlotType))
		{
			SlotsToRemove.Add(Slot.SlotType);
		}
	}
	
	// 先卸载这些槽位上的配件（级联卸载）
	for (const FGameplayTag& SlotType : SlotsToRemove)
	{
		// 检查槽位上是否有配件
		if (HasAttachmentInSlot(SlotType))
		{
			// 卸载配件（这会递归移除该配件提供的动态槽位）
			UninstallAttachment(SlotType);
		}
		
		// 从InstalledAttachments中移除对应的实例
		for (int32 i = InstalledAttachments.Num() - 1; i >= 0; --i)
		{
			if (InstalledAttachments[i].SlotType.MatchesTagExact(SlotType))
			{
				InstalledAttachments.RemoveAt(i);
				break;
			}
		}
		
		// 广播槽位不可用事件
		OnSlotAvailabilityChanged.Broadcast(SlotType, false);
		
		UE_LOG(LogYcShooterCore, Log, TEXT("RemoveDynamicSlots: 移除动态槽位 %s"), *SlotType.ToString());
	}
	
	// 从DynamicSlots中移除
	DynamicSlots.RemoveAll([&ProviderSlotType](const FYcAttachmentSlotDef& Slot)
	{
		return Slot.ProviderSlotType.MatchesTagExact(ProviderSlotType);
	});
}

bool UYcWeaponAttachmentComponent::IsDynamicSlot(FGameplayTag SlotType) const
{
	return DynamicSlots.ContainsByPredicate([&SlotType](const FYcAttachmentSlotDef& Slot)
	{
		return Slot.SlotType.MatchesTagExact(SlotType);
	});
}

// ════════════════════════════════════════════════════════════════════════
// 武器配置/预设系统实现
// ════════════════════════════════════════════════════════════════════════

bool UYcWeaponAttachmentComponent::ApplyLoadout(const FYcWeaponLoadout& Loadout)
{
	// 先清空现有配件
	ClearAllAttachments();
	
	int32 SuccessCount = 0;
	int32 FailCount = 0;
	
	for (const auto& Pair : Loadout.SlotAttachments)
	{
		const FGameplayTag& SlotType = Pair.Key;
		const FDataRegistryId& AttachmentId = Pair.Value;
		
		if (!AttachmentId.IsValid())
		{
			UE_LOG(LogYcShooterCore, Warning, 
				TEXT("ApplyLoadout: Skipping invalid AttachmentId for slot %s"),
				*SlotType.ToString());
			FailCount++;
			continue;
		}
		
		if (InstallAttachment(AttachmentId, SlotType))
		{
			SuccessCount++;
			
			// 应用调校值
			if (const FYcSlotTuningData* TuningData = Loadout.SlotTuningValues.Find(SlotType))
			{
				for (const auto& TuningPair : TuningData->TuningValues)
				{
					SetTuningValue(SlotType, TuningPair.Key, TuningPair.Value);
				}
			}
		}
		else
		{
			UE_LOG(LogYcShooterCore, Warning, 
				TEXT("ApplyLoadout: Failed to install attachment %s to slot %s"),
				*AttachmentId.ToString(), *SlotType.ToString());
			FailCount++;
		}
	}
	
	UE_LOG(LogYcShooterCore, Log, 
		TEXT("ApplyLoadout: Installed %d attachments, %d failed"),
		SuccessCount, FailCount);
	
	return FailCount == 0;
}

FYcWeaponLoadout UYcWeaponAttachmentComponent::CreateLoadoutFromCurrent() const
{
	FYcWeaponLoadout Loadout;
	
	for (const FYcAttachmentInstance& Instance : InstalledAttachments)
	{
		if (!Instance.IsValid())
		{
			continue;
		}
		
		// 添加配件到预设
		Loadout.SlotAttachments.Add(Instance.SlotType, Instance.AttachmentRegistryId);
		
		// 添加调校值（如果有）
		if (Instance.TuningValues.Num() > 0)
		{
			FYcSlotTuningData TuningData;
			TuningData.TuningValues = Instance.TuningValues;
			Loadout.SlotTuningValues.Add(Instance.SlotType, TuningData);
		}
	}
	
	return Loadout;
}
