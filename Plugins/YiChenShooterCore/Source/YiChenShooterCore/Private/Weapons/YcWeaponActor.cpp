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
#include "YiChenShooterCore.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcWeaponActor)

AYcWeaponActor::AYcWeaponActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(true);

	// 根组件
	WeaponRoot = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponRoot"));
	SetRootComponent(WeaponRoot);

	// 武器主骨骼网格体
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(WeaponRoot);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 装备Actor组件
	EquipmentActorComponent = CreateDefaultSubobject<UYcEquipmentActorComponent>(TEXT("EquipmentActorComponent"));
	EquipmentActorComponent->OnEquipmentRep.AddDynamic(this, &AYcWeaponActor::OnEquipmentInstRep);

	// 配件管理器组件
	AttachmentComponent = CreateDefaultSubobject<UYcWeaponAttachmentComponent>(TEXT("AttachmentComponent"));
}

void AYcWeaponActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	// 提前绑定配件变化事件
	// 因为 AttachmentArray 的客户端回调可能比 OnEquipmentInstRep 更早触发
	BindAttachmentEvents();
}

void AYcWeaponActor::BeginDestroy()
{
	// 清理所有动态创建的网格体组件
	DestroyAllAttachmentMeshes();
	
	Super::BeginDestroy();
}

UYcHitScanWeaponInstance* AYcWeaponActor::GetWeaponInstance() const
{
	if (EquipmentActorComponent)
	{
		return Cast<UYcHitScanWeaponInstance>(EquipmentActorComponent->GetOwningEquipment());
	}
	return nullptr;
}

UStaticMeshComponent* AYcWeaponActor::GetAttachmentMeshForSlot(FGameplayTag SlotType) const
{
	if (const TObjectPtr<UStaticMeshComponent>* Found = SlotToMeshMap.Find(SlotType))
	{
		return *Found;
	}
	return nullptr;
}

bool AYcWeaponActor::HasAttachmentMeshForSlot(FGameplayTag SlotType) const
{
	return SlotToMeshMap.Contains(SlotType);
}

void AYcWeaponActor::SetSlotVisibility(FGameplayTag SlotType, bool bVisible)
{
	if (UStaticMeshComponent* MeshComp = GetAttachmentMeshForSlot(SlotType))
	{
		MeshComp->SetVisibility(bVisible);
	}
}

void AYcWeaponActor::RefreshAllAttachmentVisuals()
{
	if (!AttachmentComponent)
	{
		return;
	}

	// 先销毁所有现有的配件网格体
	DestroyAllAttachmentMeshes();

	// 获取所有已安装的配件并重建视觉
	const TArray<FYcAttachmentInstance>& Attachments = AttachmentComponent->GetAllAttachments();
	
	for (const FYcAttachmentInstance& Instance : Attachments)
	{
		if (!Instance.IsValid())
		{
			continue;
		}

		const FYcAttachmentDefinition* AttachmentDef = Instance.GetDefinition();
		if (!AttachmentDef)
		{
			continue;
		}

		CreateAttachmentMesh(Instance.SlotType, AttachmentDef);
	}
}

void AYcWeaponActor::OnEquipmentInstRep(UYcEquipmentInstance* EquipmentInst)
{
	if (!EquipmentInst)
	{
		return;
	}

	// 初始化配件系统
	InitializeAttachmentSystem(EquipmentInst);
	
	// 客户端：武器默认配件可能在此回调前就已同步
	// 需要重新计算属性以确保数值正确
	if (!HasAuthority())
	{
		if (UYcHitScanWeaponInstance* WeaponInst = GetWeaponInstance())
		{
			WeaponInst->RecalculateStats();
		}
	}

	// 调用蓝图扩展点
	K2_OnWeaponInitialized(EquipmentInst);
}

UStaticMeshComponent* AYcWeaponActor::CreateAttachmentMesh(FGameplayTag SlotType, const FYcAttachmentDefinition* AttachmentDef)
{
	if (!SlotType.IsValid() || !AttachmentDef)
	{
		UE_LOG(LogYcShooterCore, Warning, 
			TEXT("[%s] CreateAttachmentMesh: 无效的槽位或配件定义"), *GetName());
		return nullptr;
	}

	// 如果该槽位已有网格体，先销毁
	if (SlotToMeshMap.Contains(SlotType))
	{
		DestroyAttachmentMesh(SlotType);
	}

	// 加载配件网格体资源
	UStaticMesh* MeshAsset = AttachmentDef->AttachmentMesh.LoadSynchronous();
	if (!MeshAsset)
	{
		// 配件没有网格体是允许的（纯属性配件）
		UE_LOG(LogYcShooterCore, Verbose, 
			TEXT("[%s] CreateAttachmentMesh: 槽位 %s 的配件没有网格体资源"), 
			*GetName(), *SlotType.ToString());
		return nullptr;
	}

	// 创建网格体组件
	const FName ComponentName = FName(*FString::Printf(TEXT("AttachmentMesh_%s"), *SlotType.GetTagName().ToString()));
	UStaticMeshComponent* NewMeshComp = NewObject<UStaticMeshComponent>(this, ComponentName);
	
	if (!NewMeshComp)
	{
		UE_LOG(LogYcShooterCore, Error, 
			TEXT("[%s] CreateAttachmentMesh: 创建网格体组件失败，槽位: %s"), 
			*GetName(), *SlotType.ToString());
		return nullptr;
	}

	// 设置网格体
	NewMeshComp->SetStaticMesh(MeshAsset);
	NewMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	// 配置附加关系和变换
	ConfigureAttachmentMesh(NewMeshComp, AttachmentDef);
	
	// 注册组件
	NewMeshComp->RegisterComponent();
	
	// 添加到映射
	SlotToMeshMap.Add(SlotType, NewMeshComp);

	// 应用隐藏槽位设置
	ApplyHiddenSlots(AttachmentDef, true);

	UE_LOG(LogYcShooterCore, Log, 
		TEXT("[%s] CreateAttachmentMesh: 为槽位 %s 创建了配件网格体 (%s)"), 
		*GetName(), *SlotType.ToString(), *AttachmentDef->DisplayName.ToString());

	return NewMeshComp;
}

void AYcWeaponActor::DestroyAttachmentMesh(FGameplayTag SlotType)
{
	TObjectPtr<UStaticMeshComponent> MeshComp;
	if (!SlotToMeshMap.RemoveAndCopyValue(SlotType, MeshComp))
	{
		return;
	}

	if (!MeshComp)
	{
		return;
	}

	// 在销毁前，尝试恢复被此配件隐藏的槽位
	// 注意：此时配件可能已从 AttachmentComponent 中移除，无法获取定义
	// 隐藏槽位的恢复由 OnAttachmentUninstalled 在销毁前处理

	// 销毁组件
	MeshComp->DestroyComponent();

	UE_LOG(LogYcShooterCore, Log, 
		TEXT("[%s] DestroyAttachmentMesh: 销毁了槽位 %s 的配件网格体"), 
		*GetName(), *SlotType.ToString());
}

void AYcWeaponActor::DestroyAllAttachmentMeshes()
{
	for (auto& Pair : SlotToMeshMap)
	{
		if (Pair.Value)
		{
			Pair.Value->DestroyComponent();
		}
	}
	SlotToMeshMap.Empty();
}

void AYcWeaponActor::ConfigureAttachmentMesh(UStaticMeshComponent* MeshComp, const FYcAttachmentDefinition* AttachmentDef)
{
	if (!MeshComp || !AttachmentDef)
	{
		return;
	}

	// 解析附加目标
	USceneComponent* ParentComp = nullptr;
	FName SocketName;
	
	if (ResolveAttachmentParent(AttachmentDef, ParentComp, SocketName))
	{
		MeshComp->SetupAttachment(ParentComp, SocketName);
	}
	else
	{
		// 回退到武器主网格体
		MeshComp->SetupAttachment(WeaponMesh);
	}

	// 应用相对变换
	MeshComp->SetRelativeTransform(AttachmentDef->AttachOffset);
}

bool AYcWeaponActor::ResolveAttachmentParent(const FYcAttachmentDefinition* AttachmentDef, 
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
		// 附加到武器骨骼网格体的Socket
		OutParent = WeaponMesh;
		return true;

	case EYcAttachmentTarget::AttachmentSlot:
		// 附加到另一个配件的网格体组件
		if (AttachmentDef->TargetSlotType.IsValid())
		{
			if (UStaticMeshComponent* TargetMesh = GetAttachmentMeshForSlot(AttachmentDef->TargetSlotType))
			{
				OutParent = TargetMesh;
				return true;
			}
			
			UE_LOG(LogYcShooterCore, Warning, 
				TEXT("[%s] ResolveAttachmentParent: 目标槽位 %s 没有网格体组件，回退到武器网格体"), 
				*GetName(), *AttachmentDef->TargetSlotType.ToString());
		}
		// 回退到武器骨骼
		OutParent = WeaponMesh;
		return true;

	default:
		OutParent = WeaponMesh;
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

void AYcWeaponActor::OnAttachmentInstalled(FGameplayTag SlotType, const FDataRegistryId& AttachmentId)
{
	if (!AttachmentComponent)
	{
		return;
	}

	// 获取配件定义
	const FYcAttachmentDefinition* AttachmentDef = AttachmentComponent->GetAttachmentDefInSlot(SlotType);
	if (!AttachmentDef)
	{
		UE_LOG(LogYcShooterCore, Warning, 
			TEXT("[%s] OnAttachmentInstalled: 槽位 %s 无法获取配件定义"), 
			*GetName(), *SlotType.ToString());
		return;
	}

	// 创建配件网格体
	CreateAttachmentMesh(SlotType, AttachmentDef);

	UE_LOG(LogYcShooterCore, Log, 
		TEXT("[%s] OnAttachmentInstalled: 配件 %s 已安装到槽位 %s"), 
		*GetName(), *AttachmentDef->DisplayName.ToString(), *SlotType.ToString());
}

void AYcWeaponActor::OnAttachmentUninstalled(FGameplayTag SlotType, const FDataRegistryId& AttachmentId)
{
	// 在销毁网格体前，尝试恢复被隐藏的槽位
	// 注意：此时配件已从 AttachmentComponent 中标记为卸载，但 CachedDef 可能仍然有效
	if (AttachmentComponent)
	{
		// 尝试从 AttachmentInstance 获取缓存的定义
		const FYcAttachmentInstance Instance = AttachmentComponent->GetAttachmentInSlot(SlotType);
		if (const FYcAttachmentDefinition* AttachmentDef = Instance.GetDefinition())
		{
			ApplyHiddenSlots(AttachmentDef, false);
		}
	}

	// 销毁配件网格体
	DestroyAttachmentMesh(SlotType);

	UE_LOG(LogYcShooterCore, Log, 
		TEXT("[%s] OnAttachmentUninstalled: 槽位 %s 的配件已卸载"), 
		*GetName(), *SlotType.ToString());
}

void AYcWeaponActor::InitializeAttachmentSystem(UYcEquipmentInstance* EquipmentInst)
{
	if (!AttachmentComponent || !EquipmentInst)
	{
		return;
	}

	// 转换为 HitScan 武器实例
	UYcHitScanWeaponInstance* WeaponInst = Cast<UYcHitScanWeaponInstance>(EquipmentInst);
	if (!WeaponInst)
	{
		return;
	}

	// 获取配件配置 Fragment
	const FYcFragment_WeaponAttachments* AttachmentsFragment = WeaponInst->GetAttachmentsFragment();
	if (!AttachmentsFragment)
	{
		// 武器没有配置配件系统
		return;
	}
	
	// 关联配件组件到武器实例
	WeaponInst->SetAttachmentComponent(AttachmentComponent);
	
	// 服务端：刷新所有配件视觉（显示默认配件）
	// 客户端：视觉更新通过 Fast Array 回调触发
	if (HasAuthority())
	{
		RefreshAllAttachmentVisuals();
	}

	UE_LOG(LogYcShooterCore, Log, 
		TEXT("[%s] InitializeAttachmentSystem: 配件系统初始化完成，槽位数量: %d [%s]"), 
		*GetName(), AttachmentsFragment->AttachmentSlots.Num(),
		HasAuthority() ? TEXT("Server") : TEXT("Client"));
}

void AYcWeaponActor::BindAttachmentEvents()
{
	if (!AttachmentComponent)
	{
		return;
	}

	// 绑定配件安装/卸载事件
	AttachmentComponent->OnAttachmentInstalled.AddDynamic(this, &AYcWeaponActor::OnAttachmentInstalled);
	AttachmentComponent->OnAttachmentUninstalled.AddDynamic(this, &AYcWeaponActor::OnAttachmentUninstalled);
}
