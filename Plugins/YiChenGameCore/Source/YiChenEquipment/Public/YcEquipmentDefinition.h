// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "YcEquipmentDefinition.generated.h"

class UYcEquipmentInstance;
class UYcAbilitySet;
class UYcAbilitySystemComponent;

/** 用于生成并附加到玩家角色身上的Actor信息结构体，包含了要生成的Actor类和附加的Socket以及附加的Transform等信息 */
USTRUCT(Blueprintable)
struct YICHENEQUIPMENT_API FYcEquipmentActorToSpawn
{
	GENERATED_BODY()

	FYcEquipmentActorToSpawn() : bReplicateActor(true)
	{
	}

	// 指针和引用类型放在前面（8字节对齐）, 通过合理的声明顺序降低内存对齐填充带来的内存浪费
	/* 要附加到玩家角色身上的Actor类(需开启网络复制才能让所有人看到) */
	UPROPERTY(EditAnywhere, Category=Equipment, BlueprintReadWrite)
	TSoftClassPtr<AActor> ActorToSpawn;

	// 较大结构体放在中间
	/* 附加变换 */
	UPROPERTY(EditAnywhere, Category=Equipment, BlueprintReadWrite)
	FTransform AttachTransform;

	// 数组类型
	/* 这个Actor的Tags，方便查找 */
	UPROPERTY(EditAnywhere, Category=Equipment, BlueprintReadWrite)
	TArray<FName> ActorTags;

	// 较小类型分组排列
	/* 父组件Tag，会根据这个Tag去查找父组件对象 */
	UPROPERTY(EditAnywhere, Category=Equipment, BlueprintReadWrite)
	FName ParentComponentTag;

	/* 附加的Socket名称 */
	UPROPERTY(EditAnywhere, Category=Equipment, BlueprintReadWrite)
	FName AttachSocket;

	// 布尔标志放在最后（1字节）
	/* 是否为复制的Actor，如是则在服务器上生成，否则在本地生成 */
	UPROPERTY(EditAnywhere, Category=Equipment, BlueprintReadWrite)
	uint8 bReplicateActor : 1;
};

/**
 * 装备定义，例如可以用来定义附加到角色身上的武器、护甲、头盔等类似的装备
 */
USTRUCT(BlueprintType)
struct YICHENEQUIPMENT_API FYcEquipmentDefinition : public FTableRowBase
{
	GENERATED_BODY()
	
	// 装备的实例类
	UPROPERTY(EditDefaultsOnly, Category=Equipment, BlueprintReadOnly)
	TSubclassOf<UYcEquipmentInstance> InstanceType;
	
	// 当装备被装备时,需要生成并附加到玩家角色身上的Actor列表, 例如: 装备了一把武器, 会在生成武器并附加到角色手上
	UPROPERTY(EditDefaultsOnly, Category=Equipment, BlueprintReadOnly, meta=(TitleProperty="{ActorToSpawn} -> {ParentComponentTag}::{AttachSocket}"))
	TArray<FYcEquipmentActorToSpawn> ActorsToSpawn;
	
	// 当这个装备被装备时所需要授予相关玩家角色的技能集合,包含GA GE AS
	UPROPERTY(EditDefaultsOnly, Category=Equipment, BlueprintReadOnly)
	TArray<TObjectPtr<const UYcAbilitySet>> AbilitySetsToGrant;
};