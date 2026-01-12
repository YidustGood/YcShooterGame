// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Weapons/YcWeaponActor.h"

#include "YcEquipmentActorComponent.h"
#include "Weapons/YcHitScanWeaponInstance.h"
#include "Weapons/Attachments/YcWeaponAttachmentComponent.h"
#include "Weapons/Attachments/YcFragment_WeaponAttachments.h"
#include "Weapons/Attachments/YcAttachmentTypes.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "NativeGameplayTags.h"
#include "YiChenShooterCore.h"

// 定义槽位Tag（需要在项目的GameplayTags中注册这些Tag）
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Attachment_Slot_Muzzle, "Attachment.Slot.Muzzle");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Attachment_Slot_Barrel, "Attachment.Slot.Barrel");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Attachment_Slot_Optic, "Attachment.Slot.Optic");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Attachment_Slot_Stock, "Attachment.Slot.Stock");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Attachment_Slot_Underbarrel, "Attachment.Slot.Underbarrel");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Attachment_Slot_Magazine, "Attachment.Slot.Magazine");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Attachment_Slot_RearGrip, "Attachment.Slot.RearGrip");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Attachment_Slot_Laser, "Attachment.Slot.Laser");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Attachment_Slot_Forestock, "Attachment.Slot.Forestock");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Attachment_Slot_Slide, "Attachment.Slot.Slide");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Attachment_Slot_Ironsights, "Attachment.Slot.Ironsights");

AYcWeaponActor::AYcWeaponActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(true);

	// Root
	WeaponRoot = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponRoot"));
	SetRootComponent(WeaponRoot);

	// Main Weapon Skeletal Mesh
	Weapon = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Weapon"));
	Weapon->SetupAttachment(WeaponRoot);
	Weapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// ════════════════════════════════════════════════════════════════════════
	// 配件Mesh组件（默认附加到武器骨骼，运行时由配件系统动态调整）
	// ════════════════════════════════════════════════════════════════════════

	MeshMuzzle = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshMuzzle"));
	MeshMuzzle->SetupAttachment(Weapon);
	MeshMuzzle->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MeshBarrel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshBarrel"));
	MeshBarrel->SetupAttachment(Weapon);
	MeshBarrel->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MeshOptic = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshOptic"));
	MeshOptic->SetupAttachment(Weapon);
	MeshOptic->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MeshUnderbarrel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshUnderbarrel"));
	MeshUnderbarrel->SetupAttachment(Weapon);
	MeshUnderbarrel->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MeshMagazine = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshMagazine"));
	MeshMagazine->SetupAttachment(Weapon);
	MeshMagazine->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MeshMagazineReserve = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshMagazineReserve"));
	MeshMagazineReserve->SetupAttachment(Weapon);
	MeshMagazineReserve->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MeshLaser = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshLaser"));
	MeshLaser->SetupAttachment(Weapon);
	MeshLaser->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MeshForestock = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshForestock"));
	MeshForestock->SetupAttachment(Weapon);
	MeshForestock->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MeshIronsights = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshIronsights"));
	MeshIronsights->SetupAttachment(Weapon);
	MeshIronsights->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MeshSlide = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshSlide"));
	MeshSlide->SetupAttachment(Weapon);
	MeshSlide->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// ════════════════════════════════════════════════════════════════════════
	// 功能组件
	// ════════════════════════════════════════════════════════════════════════
	
	EquipmentActorComponent = CreateDefaultSubobject<UYcEquipmentActorComponent>(TEXT("EquipmentActorComponent"));
	EquipmentActorComponent->OnEquipmentRep.AddDynamic(this, &AYcWeaponActor::OnEquipmentInstRep);

	AttachmentComponent = CreateDefaultSubobject<UYcWeaponAttachmentComponent>(TEXT("AttachmentComponent"));

	// 初始化槽位到Mesh组件的映射
	InitializeSlotMeshMapping();
}

void AYcWeaponActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	
	// 隐藏备用弹匣
	SetReserveMagazineVisibility(false);
}

void AYcWeaponActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	/**
	 * 绑定配件变化事件
	 * 因为UYcWeaponAttachmentComponent的AttachmentArray的客户端回调比OnEquipmentInstRep更早触发, 所以需要在这里就提前绑定好, 以便响应配件变化事件
	 */
	BindAttachmentEvents();
}

void AYcWeaponActor::SetReserveMagazineVisibility(bool bVisibility)
{
	if (MeshMagazineReserve)
	{
		MeshMagazineReserve->SetHiddenInGame(!bVisibility);
	}
}

void AYcWeaponActor::SetMagazineVisibility(bool bVisibility)
{
	if (MeshMagazine)
	{
		MeshMagazine->SetHiddenInGame(!bVisibility);
	}
}

void AYcWeaponActor::OnEquipmentInstRep(UYcEquipmentInstance* EquipmentInst)
{
	if (EquipmentInst)
	{
		// 初始化配件系统
		InitializeAttachmentSystem(EquipmentInst);
		
		/**
		 * 对于武器默认配件它会在客户端的OnEquipmentInstRep调用前就已经同步到客户端
		 * 所以我们在这里针对客户端做一遍刷新以便这会儿同步过来的武器实例能够计算出正确的数值
		 */
		if (!HasAuthority())
		{
			GetWeaponInstance()->RecalculateStats();
		}

		// 调用蓝图扩展点
		K2_OnWeaponInitialized(EquipmentInst);
	}
}

UYcHitScanWeaponInstance* AYcWeaponActor::GetWeaponInstance() const
{
	if (EquipmentActorComponent)
	{
		return Cast<UYcHitScanWeaponInstance>(EquipmentActorComponent->GetOwningEquipment());
	}
	return nullptr;
}

void AYcWeaponActor::InitializeAttachmentSystem(UYcEquipmentInstance* EquipmentInst)
{
	if (!AttachmentComponent || !EquipmentInst)
	{
		return;
	}

	// 尝试转换为HitScan武器实例
	UYcHitScanWeaponInstance* WeaponInstance = Cast<UYcHitScanWeaponInstance>(EquipmentInst);
	if (!WeaponInstance)
	{
		return;
	}

	// 获取配件配置Fragment
	const FYcFragment_WeaponAttachments* AttachmentsFragment = WeaponInstance->GetAttachmentsFragment();
	if (!AttachmentsFragment)
	{
		// 武器没有配置配件系统，跳过初始化
		return;
	}
	
	// 初始化配件组件并关联到武器实例
	WeaponInstance->SetAttachmentComponent(AttachmentComponent);
	
	// 仅服务端刷新所有配件视觉（显示默认配件）
	// 客户端的视觉更新通过 Fast Array 回调 (PostReplicatedAdd/Change) 触发
	if (HasAuthority())
	{
		RefreshAllAttachmentVisuals();
	}

	UE_LOG(LogYcShooterCore, Log, TEXT("AYcWeaponActor: 配件系统初始化完成，槽位数量: %d [%s]"), 
		AttachmentsFragment->AttachmentSlots.Num(),
		HasAuthority() ? TEXT("Server") : TEXT("Client"));
}

void AYcWeaponActor::InitializeSlotMeshMapping()
{
	// 建立槽位Tag到Mesh组件的映射
	SlotToMeshMap.Add(TAG_Attachment_Slot_Muzzle, MeshMuzzle);
	SlotToMeshMap.Add(TAG_Attachment_Slot_Barrel, MeshBarrel);
	SlotToMeshMap.Add(TAG_Attachment_Slot_Optic, MeshOptic);
	SlotToMeshMap.Add(TAG_Attachment_Slot_Underbarrel, MeshUnderbarrel);
	SlotToMeshMap.Add(TAG_Attachment_Slot_Magazine, MeshMagazine);
	SlotToMeshMap.Add(TAG_Attachment_Slot_Laser, MeshLaser);
	SlotToMeshMap.Add(TAG_Attachment_Slot_Forestock, MeshForestock);
	SlotToMeshMap.Add(TAG_Attachment_Slot_Slide, MeshSlide);
	SlotToMeshMap.Add(TAG_Attachment_Slot_Ironsights, MeshIronsights);
}

void AYcWeaponActor::BindAttachmentEvents()
{
	if (!AttachmentComponent)
	{
		return;
	}

	//@TODO 后续实际上还可以采用GameplayMessage来处理降低耦合
	
	// 绑定配件安装/卸载事件
	AttachmentComponent->OnAttachmentInstalled.AddDynamic(this, &AYcWeaponActor::OnAttachmentInstalled);
	AttachmentComponent->OnAttachmentUninstalled.AddDynamic(this, &AYcWeaponActor::OnAttachmentUninstalled);
	
	// 绑定槽位可用性变化事件（动态槽位）
	AttachmentComponent->OnSlotAvailabilityChanged.AddDynamic(this, &AYcWeaponActor::OnSlotAvailabilityChanged);
}

UStaticMeshComponent* AYcWeaponActor::GetMeshComponentForSlot(FGameplayTag SlotType) const
{
	if (const TObjectPtr<UStaticMeshComponent>* Found = SlotToMeshMap.Find(SlotType))
	{
		return *Found;
	}
	return nullptr;
}

void AYcWeaponActor::UpdateAttachmentVisual(FGameplayTag SlotType, const FYcAttachmentDefinition* AttachmentDef)
{
	UStaticMeshComponent* MeshComp = GetMeshComponentForSlot(SlotType);
	if (!MeshComp)
	{
		UE_LOG(LogYcShooterCore, Warning, TEXT("AYcWeaponActor::UpdateAttachmentVisual - 未找到槽位 %s 对应的Mesh组件"), 
			*SlotType.ToString());
		return;
	}

	if (!AttachmentDef)
	{
		// 清空配件
		MeshComp->SetStaticMesh(nullptr);
		MeshComp->SetVisibility(false);
		return;
	}

	// 加载配件网格体
	UStaticMesh* AttachmentMesh = AttachmentDef->AttachmentMesh.LoadSynchronous();
	
	// 更新网格体
	MeshComp->SetStaticMesh(AttachmentMesh);
	
	if (AttachmentMesh)
	{
		// 获取附加父组件
		USceneComponent* ParentComp = nullptr;
		FName SocketName;
		
		if (GetAttachmentParent(AttachmentDef, ParentComp, SocketName))
		{
			// 重新附加到正确的父组件
			MeshComp->AttachToComponent(ParentComp, 
				FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
		}
		
		// 应用偏移变换
		MeshComp->SetRelativeTransform(AttachmentDef->AttachOffset);
		MeshComp->SetVisibility(true);
		
		// 应用隐藏槽位设置
		ApplyHiddenSlots(AttachmentDef, true);
	}
	else
	{
		MeshComp->SetVisibility(false);
	}
}

void AYcWeaponActor::ClearAttachmentVisual(FGameplayTag SlotType)
{
	// 先获取当前配件，用于恢复隐藏的槽位
	if (AttachmentComponent)
	{
		const FYcAttachmentDefinition* CurrentAttachment = AttachmentComponent->GetAttachmentDefInSlot(SlotType);
		if (CurrentAttachment)
		{
			// 恢复被隐藏的槽位
			ApplyHiddenSlots(CurrentAttachment, false);
		}
	}
	
	UpdateAttachmentVisual(SlotType, nullptr);
}

void AYcWeaponActor::RefreshAllAttachmentVisuals()
{
	if (!AttachmentComponent)
	{
		return;
	}

	// 获取所有已安装的配件
	const TArray<FYcAttachmentInstance>& Attachments = AttachmentComponent->GetAllAttachments();
	
	for (const FYcAttachmentInstance& Instance : Attachments)
	{
		if (!Instance.IsValid())
		{
			// 槽位为空，清空视觉
			UStaticMeshComponent* MeshComp = GetMeshComponentForSlot(Instance.SlotType);
			if (MeshComp)
			{
				MeshComp->SetStaticMesh(nullptr);
				MeshComp->SetVisibility(false);
			}
			continue;
		}

		// 获取配件定义（使用新的 GetDefinition 方法从 DataRegistry 获取）
		const FYcAttachmentDefinition* AttachmentDef = Instance.GetDefinition();
		if (!AttachmentDef)
		{
			continue;
		}

		// 更新视觉
		UpdateAttachmentVisual(Instance.SlotType, AttachmentDef);
	}
}

void AYcWeaponActor::SetSlotVisibility(FGameplayTag SlotType, bool bVisible)
{
	UStaticMeshComponent* MeshComp = GetMeshComponentForSlot(SlotType);
	if (MeshComp)
	{
		MeshComp->SetVisibility(bVisible);
	}
}

void AYcWeaponActor::OnAttachmentInstalled(FGameplayTag SlotType, const FDataRegistryId& AttachmentId)
{
	// 从 AttachmentComponent 获取配件定义
	const FYcAttachmentDefinition* AttachmentDef = nullptr;
	if (AttachmentComponent)
	{
		AttachmentDef = AttachmentComponent->GetAttachmentDefInSlot(SlotType);
	}

	if (!AttachmentDef)
	{
		// 如果没有配件定义，可能是槽位被清空或配件无效
		// 清空该槽位的视觉
		UStaticMeshComponent* MeshComp = GetMeshComponentForSlot(SlotType);
		if (MeshComp)
		{
			MeshComp->SetStaticMesh(nullptr);
			MeshComp->SetVisibility(false);
		}
		
		UE_LOG(LogYcShooterCore, Verbose, TEXT("AYcWeaponActor::OnAttachmentInstalled - 槽位 %s 无配件定义，已清空视觉"), 
			*SlotType.ToString());
		return;
	}

	// 更新视觉
	UpdateAttachmentVisual(SlotType, AttachmentDef);

	UE_LOG(LogYcShooterCore, Log, TEXT("AYcWeaponActor: 配件 %s 已安装到槽位 %s"), 
		*AttachmentDef->DisplayName.ToString(), *SlotType.ToString());
}


void AYcWeaponActor::OnAttachmentUninstalled(FGameplayTag SlotType, const FDataRegistryId& AttachmentId)
{
	// 注意：卸载时配件已从组件中移除，无法通过 GetAttachmentDefInSlot 获取
	// AttachmentId 可能是无效的（当通过 PostReplicatedChange 检测到槽位被清空时） 

	// 清空该槽位的视觉
	UStaticMeshComponent* MeshComp = GetMeshComponentForSlot(SlotType);
	if (MeshComp)
	{
		MeshComp->SetStaticMesh(nullptr);
		MeshComp->SetVisibility(false);
	}
}

bool AYcWeaponActor::GetAttachmentParent(const FYcAttachmentDefinition* AttachmentDef, 
	USceneComponent*& OutParent, FName& OutSocketName) const
{
	if (!AttachmentDef)
	{
		return false;
	}

	OutSocketName = AttachmentDef->AttachSocketName;

	switch (AttachmentDef->AttachTarget)
	{
	case EYcAttachmentTarget::WeaponSocket:
		// 附加到武器骨骼网格体
		OutParent = Weapon;
		return true;

	case EYcAttachmentTarget::AttachmentSlot:
		// 附加到另一个配件的Mesh组件
		if (AttachmentDef->TargetSlotType.IsValid())
		{
			UStaticMeshComponent* TargetMesh = GetMeshComponentForSlot(AttachmentDef->TargetSlotType);
			if (TargetMesh)
			{
				OutParent = TargetMesh;
				return true;
			}
			else
			{
				UE_LOG(LogYcShooterCore, Warning, 
					TEXT("AYcWeaponActor::GetAttachmentParent - 目标槽位 %s 没有对应的Mesh组件"), 
					*AttachmentDef->TargetSlotType.ToString());
			}
		}
		// 回退到武器骨骼
		OutParent = Weapon;
		return true;

	default:
		OutParent = Weapon;
		return true;
	}
}

void AYcWeaponActor::ApplyHiddenSlots(const FYcAttachmentDefinition* AttachmentDef, bool bHide)
{
	if (!AttachmentDef)
	{
		return;
	}

	for (const FGameplayTag& SlotToHide : AttachmentDef->HiddenSlots)
	{
		SetSlotVisibility(SlotToHide, !bHide);
	}
}

void AYcWeaponActor::OnSlotAvailabilityChanged(FGameplayTag SlotType, bool bIsAvailable)
{
	// @TODO 可以增加动态Mesh组件功能, 目前静态就不能做移除
	// if (bIsAvailable)
	// {
	// 	// 动态槽位被添加，创建对应的Mesh组件
	// 	CreateDynamicMeshComponent(SlotType);
	// 	
	// 	UE_LOG(LogYcShooterCore, Log, TEXT("AYcWeaponActor: 动态槽位 %s 已添加"), *SlotType.ToString());
	// }
	// else
	// {
	// 	// 动态槽位被移除，销毁对应的Mesh组件
	// 	DestroyDynamicMeshComponent(SlotType);
	// 	
	// 	UE_LOG(LogYcShooterCore, Log, TEXT("AYcWeaponActor: 动态槽位 %s 已移除"), *SlotType.ToString());
	// }
}

UStaticMeshComponent* AYcWeaponActor::CreateDynamicMeshComponent(FGameplayTag SlotType)
{
	// 检查是否已存在
	if (SlotToMeshMap.Contains(SlotType))
	{
		return SlotToMeshMap[SlotType];
	}
	
	// 创建新的StaticMeshComponent
	FName ComponentName = FName(*FString::Printf(TEXT("DynamicMesh_%s"), *SlotType.ToString()));
	UStaticMeshComponent* NewMeshComp = NewObject<UStaticMeshComponent>(this, ComponentName);
	
	if (NewMeshComp)
	{
		// 默认附加到武器骨骼
		NewMeshComp->SetupAttachment(Weapon);
		NewMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		NewMeshComp->SetVisibility(false);
		NewMeshComp->RegisterComponent();
		
		// 添加到映射
		SlotToMeshMap.Add(SlotType, NewMeshComp);
		
		UE_LOG(LogYcShooterCore, Log, TEXT("AYcWeaponActor: 为动态槽位 %s 创建了Mesh组件"), *SlotType.ToString());
	}
	
	return NewMeshComp;
}

void AYcWeaponActor::DestroyDynamicMeshComponent(FGameplayTag SlotType)
{
	TObjectPtr<UStaticMeshComponent>* Found = SlotToMeshMap.Find(SlotType);
	if (!Found)
	{
		return;
	}
	
	UStaticMeshComponent* MeshComp = *Found;
	if (MeshComp)
	{
		// 检查是否是预定义的组件（不应该销毁）
		// 预定义组件在构造函数中创建，有固定的名称
		FString CompName = MeshComp->GetName();
		if (CompName.StartsWith(TEXT("DynamicMesh_")))
		{
			MeshComp->DestroyComponent();
			UE_LOG(LogYcShooterCore, Log, TEXT("AYcWeaponActor: 销毁动态槽位 %s 的Mesh组件"), *SlotType.ToString());
		}
	}
	
	SlotToMeshMap.Remove(SlotType);
}
