// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Weapons/YcWeaponStateComponent.h"
#include "NativeGameplayTags.h"
#include "YcEquipmentManagerComponent.h"
#include "YcTeamSubsystem.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Physics/YcPhysicalMaterialWithTags.h"
#include "Weapons/YcHitScanWeaponInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcWeaponStateComponent)

// 定义命中区域相关的 GameplayTag
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Gameplay_Zone, "Gameplay.Zone");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Gameplay_Character_Zone, "Gameplay.Character.Zone");

UYcWeaponStateComponent::UYcWeaponStateComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 启用网络复制
	SetIsReplicatedByDefault(true);

	// 启用 Tick 更新
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.bCanEverTick = true;
}

void UYcWeaponStateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 获取控制器拥有的 Pawn
	if (APawn* Pawn = GetPawn<APawn>())
	{
		// 查找装备管理组件
		if (UYcEquipmentManagerComponent* EquipmentManager = Pawn->FindComponentByClass<UYcEquipmentManagerComponent>())
		{
			// 获取当前装备的射线武器实例并驱动其 Tick 更新
			// 用于更新散布恢复、后坐力恢复等时间相关的武器状态
			if (UYcHitScanWeaponInstance* CurrentWeapon = Cast<UYcHitScanWeaponInstance>(EquipmentManager->GetFirstInstanceOfType(UYcHitScanWeaponInstance::StaticClass())))
			{
				CurrentWeapon->Tick(DeltaTime);
			}
		}
	}
}

void UYcWeaponStateComponent::ClientConfirmTargetData_Implementation(uint16 UniqueId, bool bSuccess, const TArray<uint8>& HitReplaces)
{
	// 遍历所有未确认的命中批次，查找匹配的 UniqueId
	for (int i = 0; i < UnconfirmedServerSideHitMarkers.Num(); i++)
	{
		FYcServerSideHitMarkerBatch& Batch = UnconfirmedServerSideHitMarkers[i];
		if (Batch.UniqueId == UniqueId)
		{
			// 服务器确认成功且有需要保留的命中（HitReplaces 包含需要移除的索引）
			if (bSuccess && (HitReplaces.Num() != Batch.Markers.Num()))
			{
				UWorld* World = GetWorld();
				bool bFoundShowAsSuccessHit = false;

				int32 HitLocationIndex = 0;
				for (const FYcScreenSpaceHitLocation& Entry : Batch.Markers)
				{
					// 如果该命中不在替换列表中且标记为成功，则添加到已确认列表
					if (!HitReplaces.Contains(HitLocationIndex) && Entry.bShowAsSuccess)
					{
						// 只需要在第一次找到成功命中时更新时间
						if (!bFoundShowAsSuccessHit)
						{
							ActuallyUpdateDamageInstigatedTime();
						}

						bFoundShowAsSuccessHit = true;

						// 将确认的命中添加到显示列表
						LastWeaponDamageScreenLocations.Add(Entry);
					}
					++HitLocationIndex;
				}
			}

			// 无论成功与否，都从未确认列表中移除该批次
			UnconfirmedServerSideHitMarkers.RemoveAt(i);
			break;
		}
	}
}

void UYcWeaponStateComponent::AddUnconfirmedServerSideHitMarkers(const FGameplayAbilityTargetDataHandle& InTargetData, const TArray<FHitResult>& FoundHits)
{
	// 1. 创建新的未确认命中批次，使用目标数据的唯一标识符
	FYcServerSideHitMarkerBatch& NewUnconfirmedHitMarker = UnconfirmedServerSideHitMarkers.Emplace_GetRef(InTargetData.UniqueId);
	
	const APlayerController* OwnerPC = GetController<APlayerController>();
	if (OwnerPC == nullptr) return;

	// 2. 处理每个命中结果
	for (const FHitResult& Hit : FoundHits)
	{
		// 3. 将世界坐标转换为屏幕坐标
		FVector2D HitScreenLocation;
		if (!UGameplayStatics::ProjectWorldToScreen(OwnerPC, Hit.Location, /*out*/ HitScreenLocation, /*bPlayerViewportRelative=*/ false)) continue;
		
		// 4. 创建屏幕空间命中标记
		FYcScreenSpaceHitLocation& Entry = NewUnconfirmedHitMarker.Markers.AddDefaulted_GetRef();
		Entry.Location = HitScreenLocation;
		Entry.bShowAsSuccess = ShouldShowHitAsSuccess(Hit);

		// 5. 从物理材质中提取命中区域信息
		const UYcPhysicalMaterialWithTags* PhysMatWithTags = Cast<const UYcPhysicalMaterialWithTags>(Hit.PhysMaterial.Get());
		if (PhysMatWithTags == nullptr) continue;
		
		// 遍历物理材质的标签，查找命中区域标签
		for (const FGameplayTag MaterialTag : PhysMatWithTags->Tags)
		{
			// 支持两种 HitZone 标签路径：Gameplay.Zone 和 Gameplay.Character.Zone
			if (MaterialTag.MatchesTag(TAG_Gameplay_Zone) || MaterialTag.MatchesTag(TAG_Gameplay_Character_Zone))
			{
				Entry.HitZone = MaterialTag;
				break;
			}
		}
	}
}

void UYcWeaponStateComponent::UpdateDamageInstigatedTime(const FGameplayEffectContextHandle& EffectContext)
{
	// 根据效果上下文判断是否需要更新伤害造成时间
	if (ShouldUpdateDamageInstigatedTime(EffectContext))
	{
		ActuallyUpdateDamageInstigatedTime();
	}
}

double UYcWeaponStateComponent::GetTimeSinceLastHitNotification() const
{
	const UWorld* World = GetWorld();
	return World->TimeSince(LastWeaponDamageInstigatedTime);
}

bool UYcWeaponStateComponent::ShouldShowHitAsSuccess(const FHitResult& Hit) const
{
	AActor* HitActor = Hit.GetActor();

	// TODO: 对于因无敌或类似原因未造成伤害的命中，不应视为成功
	UWorld* World = GetWorld();

	// 使用队伍子系统判断是否可以对目标造成伤害
	if (const UYcTeamSubsystem* TeamSubsystem = UWorld::GetSubsystem<UYcTeamSubsystem>(GetWorld()))
	{
		return TeamSubsystem->CanCauseDamage(GetController<APlayerController>(), Hit.GetActor());
	}

	return true;
}

bool UYcWeaponStateComponent::ShouldUpdateDamageInstigatedTime(const FGameplayEffectContextHandle& EffectContext) const
{
	// TODO: 实现更精确的过滤逻辑
	// 对于此组件，我们只关心武器或武器发射的投射物造成的伤害
	// 应该过滤到这些情况（或者检查造成者是否也是当前准星配置的来源）
	return EffectContext.GetEffectCauser() != nullptr;
}

void UYcWeaponStateComponent::ActuallyUpdateDamageInstigatedTime()
{
	const UWorld* World = GetWorld();
	
	// 如果距离上次伤害造成时间超过 0.1 秒，清除之前的命中位置数组
	// 这样可以确保命中标记只在短时间内显示
	if (World->GetTimeSeconds() - LastWeaponDamageInstigatedTime > 0.1)
	{
		LastWeaponDamageScreenLocations.Reset();
	}
	
	// 更新最后伤害造成时间为当前时间
	LastWeaponDamageInstigatedTime = World->GetTimeSeconds();
}
