// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "YcAbilitySet.h"
#include "YcEquipmentDefinition.h"
#include "Components/PawnComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "YcEquipmentManagerComponent.generated.h"

struct FYcEquipmentList;
class UYcEquipmentManagerComponent;
class UYcInventoryItemInstance;

////////////Applied Equipment FastArray Begin.//////////////

/** 
 * 表示被应用到玩家上的一个装备实例结构体，包含了装备的定义、装备的实例对象、通过该装备已授予的技能集句柄信息(用于移除技能)
 */
USTRUCT(BlueprintType)
struct FYcAppliedEquipmentEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FYcAppliedEquipmentEntry()
	{
	}

	FString GetDebugString() const;

private:
	friend FYcEquipmentList;
	friend UYcEquipmentManagerComponent;
	
	// 装备的实例对象
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UYcEquipmentInstance> Instance;
	
	// 装备所属的ItemInst
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UYcInventoryItemInstance> OwnerItemInstance;

	// 已授予的技能集句柄信息,非网络复制,只在服务器上存在,因为只有服务器能掌管技能的授予/收回等
	UPROPERTY(NotReplicated)
	FYcAbilitySet_GrantedHandles GrantedHandles;
};

/** 已应用到玩家的装备列表*/
USTRUCT(BlueprintType)
struct FYcEquipmentList : public FFastArraySerializer
{
	GENERATED_BODY()

	FYcEquipmentList()
		: OwnerComponent(nullptr)
	{
	}

	FYcEquipmentList(UActorComponent* InOwnerComponent)
		: OwnerComponent(InOwnerComponent)
	{
	}
	
	//~FFastArraySerializer contract
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	//~End of FFastArraySerializer contract

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FYcAppliedEquipmentEntry, FYcEquipmentList>(Equipments, DeltaParms, *this);
	}

	/**
	 * 添加指定装备，同时授予装备的能力、附加装备的Actors
	 * 根据指定类型的装备创建一个装备实例，并记录进Entries列表中
	 * 
	 * @param EquipmentDef 要添加的装备定义
	 * @param ItemInstance 这个装备所属的ItemInst
	 * @return 
	 */
	UYcEquipmentInstance* AddEntry(const FYcEquipmentDefinition& EquipmentDef, UYcInventoryItemInstance* ItemInstance);
	
	/**
	 * 移除指定装备实例(同时移除掉装备所赋予的能力和附加的Actors)
	 * @param Instance 要移除的实例对象
	 */
	void RemoveEntry(UYcEquipmentInstance* Instance);

private:
	// 获取OwnerActor的ASC组件对象
	UYcAbilitySystemComponent* GetAbilitySystemComponent() const;

	friend UYcEquipmentManagerComponent;
	
	// 网络复制的装备实例列表
	UPROPERTY(VisibleAnywhere)
	TArray<FYcAppliedEquipmentEntry> Equipments;

	UPROPERTY(NotReplicated, VisibleAnywhere)
	TObjectPtr<UActorComponent> OwnerComponent;
};

template <>
struct TStructOpsTypeTraits<FYcEquipmentList> : public TStructOpsTypeTraitsBase2<FYcEquipmentList>
{
	enum { WithNetDeltaSerializer = true };
};

////////////Applied Equipment FastArray End.//////////////

/**
 * Manages equipment applied to a pawn
 * 管理装备的组件,提供装备和卸载指定装备的能力
 * 通常是挂载到玩家控制的角色上, 角色死亡后组件负责清理已生成的装备Actor
 */
UCLASS(ClassGroup=(YiChenGameCore), meta=(BlueprintSpawnableComponent), Blueprintable, BlueprintType)
class YICHENEQUIPMENT_API UYcEquipmentManagerComponent : public UPawnComponent
{
	GENERATED_BODY()
public:
	UYcEquipmentManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	//~UActorComponent interface
	//virtual void EndPlay() override;
	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;
	virtual void ReadyForReplication() override;
	//~End of UActorComponent interface
	
	// [Server Only] 通过装备定义应用装备
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	UYcEquipmentInstance* EquipItemByEquipmentDef(const FYcEquipmentDefinition& EquipmentDef, UYcInventoryItemInstance* ItemInstance);
	
	// [Server Only] 只在服务器被调用
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void UnequipItem(UYcEquipmentInstance* EquipmentInst);
	
	/** 
	* 返回给定装备类型的第一个实例，如果没有找到则返回nullptr
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure)
	UYcEquipmentInstance* GetFirstInstanceOfType(TSubclassOf<UYcEquipmentInstance> InstanceType);

	/**
	 * 返回给定装备类型的所有装备实例，如果没有找到则返回空数组
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<UYcEquipmentInstance*> GetEquipmentInstancesOfType(TSubclassOf<UYcEquipmentInstance> InstanceType) const;

	template <typename T>
	T* GetFirstInstanceOfType()
	{
		return static_cast<T*>(GetFirstInstanceOfType(T::StaticClass()));
	}
	
private:
	UPROPERTY(Replicated, VisibleAnywhere)
	FYcEquipmentList EquipmentList;
};
