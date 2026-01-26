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
#include "Weapons/YcWeaponVisualData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcWeaponActor)

AYcWeaponActor::AYcWeaponActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	// 根组件
	WeaponRoot = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponRoot"));
	SetRootComponent(WeaponRoot);

	// 武器主骨骼网格体
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(WeaponRoot);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetFirstPersonPrimitiveType(FirstPersonType);

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

	// 获取所有已安装的配件
	const TArray<FYcAttachmentInstance>& Attachments = AttachmentComponent->GetAllAttachments();
	
	// 按依赖关系排序：先创建被依赖的配件，再创建依赖它们的配件
	TArray<FYcAttachmentInstance> SortedAttachments = Attachments;
	SortedAttachments.Sort([](const FYcAttachmentInstance& A, const FYcAttachmentInstance& B)
	{
		const FYcAttachmentDefinition* DefA = A.GetDefinition();
		const FYcAttachmentDefinition* DefB = B.GetDefinition();
		
		if (!DefA || !DefB)
		{
			return false;
		}
		
		// 如果 B 依赖 A（B 要附加到 A 的槽位上），则 A 应该排在前面
		if (DefB->AttachTarget == EYcAttachmentTarget::AttachmentSlot && 
			DefB->TargetSlotType == A.SlotType)
		{
			return true;
		}
		
		// 如果 A 依赖 B，则 B 应该排在前面
		if (DefA->AttachTarget == EYcAttachmentTarget::AttachmentSlot && 
			DefA->TargetSlotType == B.SlotType)
		{
			return false;
		}
		
		// 默认保持原顺序
		return false;
	});
	
	// 按排序后的顺序创建配件网格体
	for (const FYcAttachmentInstance& Instance : SortedAttachments)
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
	const UYcWeaponInstance* WeaponInstance = Cast<UYcWeaponInstance>(EquipmentInst);
	if (!WeaponInstance)
	{
		UE_LOG(LogYcShooterCore, Error, TEXT("AYcWeaponActor::OnEquipmentInstRep: 武器Actor关联的EquipmentInstance非UYcWeaponInstance或其子类!"));
		return;
	}
	
	// 设置武器模型和武器动画蓝图类
	if (const UYcWeaponVisualData* WeaponVisualData = WeaponInstance->GetWeaponVisualData())
	{
		WeaponMesh->SetSkeletalMesh(WeaponVisualData->MainWeaponMesh.Get());
		WeaponMesh->SetAnimInstanceClass(WeaponVisualData->WeaponAnimInstanceClass.Get());
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
	
	// 标记武器已初始化并广播委托
	// 注意：委托触发时，所有绑定的Lambda会同时执行
	// 为了确保配件按依赖关系正确创建，我们需要在委托中批量处理
	bWeaponInitialized = true;
	OnWeaponInitializedDelegate.Broadcast();
	
	UE_LOG(LogYcShooterCore, Log, 
	TEXT("[%s] OnEquipmentInstRep: 武器初始化完成 [%s]"),
	*GetName(), HasAuthority() ? TEXT("Server") : TEXT("Client"));

	// 调用蓝图扩展点
	K2_OnWeaponInitialized(EquipmentInst);
}

bool AYcWeaponActor::IsWeaponInitialized() const
{
	return bWeaponInitialized && GetWeaponInstance() != nullptr && WeaponMesh->GetSkeletalMeshAsset() != nullptr;
}

UStaticMeshComponent* AYcWeaponActor::CreateAttachmentMesh(FGameplayTag SlotType, const FYcAttachmentDefinition* AttachmentDef)
{
	if (!SlotType.IsValid() || !AttachmentDef)
	{
		UE_LOG(LogYcAttachment, Warning, 
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
		UE_LOG(LogYcAttachment, Verbose, 
			TEXT("[%s] CreateAttachmentMesh: 槽位 %s 的配件没有网格体资源"), 
			*GetName(), *SlotType.ToString());
		return nullptr;
	}

	// 创建网格体组件
	const FName ComponentName = FName(*FString::Printf(TEXT("AttachmentMesh_%s"), *SlotType.GetTagName().ToString()));
	UStaticMeshComponent* NewMeshComp = NewObject<UStaticMeshComponent>(this, ComponentName);
	
	if (!NewMeshComp)
	{
		UE_LOG(LogYcAttachment, Error, 
			TEXT("[%s] CreateAttachmentMesh: 创建网格体组件失败，槽位: %s"), 
			*GetName(), *SlotType.ToString());
		return nullptr;
	}

	// 设置网格体
	NewMeshComp->SetStaticMesh(MeshAsset);
	NewMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NewMeshComp->SetFirstPersonPrimitiveType(FirstPersonType);
	
	// 配置附加关系和变换
	ConfigureAttachmentMesh(NewMeshComp, AttachmentDef);
	
	// 注册组件
	NewMeshComp->RegisterComponent();
	
	// 根据武器装备状态设置初始可见性
	const bool bShouldBeVisible = ShouldAttachmentsBeVisible();
	NewMeshComp->SetVisibility(bShouldBeVisible, true);
	
	// 添加到映射
	SlotToMeshMap.Add(SlotType, NewMeshComp);

	// 应用隐藏槽位设置
	ApplyHiddenSlots(AttachmentDef, true);

	UE_LOG(LogYcAttachment, Log, 
		TEXT("[%s] CreateAttachmentMesh: 为槽位 %s 创建了配件网格体 (%s)，初始可见性: %s"), 
		*GetName(), *SlotType.ToString(), *AttachmentDef->DisplayName.ToString(),
		bShouldBeVisible ? TEXT("可见") : TEXT("隐藏"));

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

	UE_LOG(LogYcAttachment, Log, 
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
			
			// 注意：此警告不应该出现，因为动态槽位的默认配件安装已延迟到下一帧
			// 如果仍然出现，说明配件依赖关系配置有问题
			UE_LOG(LogYcAttachment, Error, 
				TEXT("[%s] ResolveAttachmentParent: 目标槽位 %s 没有网格体组件！这可能是配件依赖配置错误，回退到武器网格体"), 
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
		UE_LOG(LogYcAttachment, Warning, 
			TEXT("[%s] OnAttachmentInstalled: 槽位 %s 无法获取配件定义"), 
			*GetName(), *SlotType.ToString());
		return;
	}

	// 创建配件网格体
	CreateAttachmentMesh(SlotType, AttachmentDef);

	UE_LOG(LogYcAttachment, Log, 
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

	UE_LOG(LogYcAttachment, Log, 
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

	UE_LOG(LogYcAttachment, Log, 
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

bool AYcWeaponActor::ShouldAttachmentsBeVisible() const
{
	// 获取武器实例
	const UYcWeaponInstance* WeaponInst = GetWeaponInstance();
	if (!WeaponInst)
	{
		// 武器实例未设置，默认隐藏
		return false;
	}
	
	// 根据装备状态判断
	const EYcEquipmentState EquipmentState = WeaponInst->GetEquipmentState();
	return EquipmentState == EYcEquipmentState::Equipped;
}
