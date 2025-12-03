// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YcReplicableObject.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcReplicableObject)


UYcReplicableObject::UYcReplicableObject(const FObjectInitializer& ObjectInitializer): Super(ObjectInitializer)
{
	// 必须在构造函数中初始化值，否则使用时在某些情况会崩溃,UObject->IsSupportedForNetworking()相关错误。
	bReplicatedFlag = 1;
}

bool UYcReplicableObject::IsSupportedForNetworking() const
{
	return bReplicatedFlag;
}

void UYcReplicableObject::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// Add any Blueprint properties
	// This is not required if you do not want the class to be "Blueprintable"
	if (const UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(GetClass()))
	{
		BPClass->GetLifetimeBlueprintReplicationList(OutLifetimeProps);
	}

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;
	DOREPLIFETIME_WITH_PARAMS_FAST(UYcReplicableObject, bReplicatedFlag, SharedParams);
}

int32 UYcReplicableObject::GetFunctionCallspace(UFunction* Function, FFrame* Stack)
{
	check(GetOuter() != nullptr);
	return GetOuter()->GetFunctionCallspace(Function, Stack);
}

bool UYcReplicableObject::CallRemoteFunction(UFunction* Function, void* Parms, FOutParmRec* OutParms, FFrame* Stack)
{
	check(!HasAnyFlags(RF_ClassDefaultObject)); // 排除CDO对象
	AActor* Owner = GetActorOuter();
	if (UNetDriver* NetDriver = Owner->GetNetDriver())
	{
		NetDriver->ProcessRemoteFunction(Owner, Function, Parms, OutParms, Stack, this);
		return true;
	}
	return false;
}

UWorld* UYcReplicableObject::GetWorld() const
{
	if (const UObject* MyOuter = GetOuter())
	{
		return MyOuter->GetWorld();
	}
	return nullptr;
}

AActor* UYcReplicableObject::GetActorOuter() const
{
	return GetTypedOuter<AActor>();
}

void UYcReplicableObject::Destroy()
{
	if (IsValid(this))
	{
		// 保证Destroy()是在服务器上调用的
		if (bReplicatedFlag) checkf(GetActorOuter()->HasAuthority() == true, TEXT("Destroy:: Object does not have authority to destroy itself!"));
		MultiOnDestroyed();
		MarkAsGarbage(); // 标记对象为垃圾状态，等待GC回收清理
	}
}

void UYcReplicableObject::MultiOnDestroyed_Implementation()
{
	OnDestroyed();
	K2_OnDestroyed();
}

void UYcReplicableObject::OnDestroyed()
{
	// Notify Owner etc.
}

void UYcReplicableObject::SetBReplicatedFlag(uint8 InReplicatedFlag)
{
	bReplicatedFlag = InReplicatedFlag;
	// PushModel 标记属性被改变，需要同步
	MARK_PROPERTY_DIRTY_FROM_NAME(UYcReplicableObject, bReplicatedFlag, this);
}