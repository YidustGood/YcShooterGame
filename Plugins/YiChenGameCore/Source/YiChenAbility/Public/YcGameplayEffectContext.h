// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "YcGameplayEffectContext.generated.h"

class AActor;
class FArchive;
class IYcAbilitySourceInterface;
class UObject;
class UPhysicalMaterial;

/**
 * 技能效果执行的上下文信息，在整个技能效果执行过程中传递，用于追踪执行的瞬态信息, 支持网络序列化
 */
USTRUCT()
struct YICHENABILITY_API FYcGameplayEffectContext : public FGameplayEffectContext
{
	GENERATED_BODY()

	FYcGameplayEffectContext()
		: FGameplayEffectContext()
	{
	}

	FYcGameplayEffectContext(AActor* InInstigator, AActor* InEffectCauser)
		: FGameplayEffectContext(InInstigator, InEffectCauser)
	{
	}

	/**
	 * 从句柄中提取FYcGameplayEffectContext
	 * 如果句柄不存在或类型不匹配则返回nullptr
	 */
	static FYcGameplayEffectContext* ExtractEffectContext(struct FGameplayEffectContextHandle Handle);

	/**
	 * 设置技能效果的源对象
	 * @param InObject 实现IYcAbilitySourceInterface的源对象
	 * @param InSourceLevel 源对象的等级
	 */
	void SetAbilitySource(const IYcAbilitySourceInterface* InObject, float InSourceLevel);

	/**
	 * 获取技能效果的源接口
	 * 仅在权威端有效
	 * @return 源对象实现的IYcAbilitySourceInterface接口
	 */
	const IYcAbilitySourceInterface* GetAbilitySource() const;

	/** 添加或替换运行时 Payload（按 Struct 类型唯一） */
	void AddOrReplaceRuntimePayload(const FInstancedStruct& InPayload);

	/** 按 Struct 类型查找运行时 Payload */
	const FInstancedStruct* FindRuntimePayload(const UScriptStruct* PayloadType) const;

	template<typename PayloadType>
	void AddOrReplaceRuntimePayload(const PayloadType& InPayload)
	{
		FInstancedStruct Payload;
		Payload.InitializeAs<PayloadType>(InPayload);
		AddOrReplaceRuntimePayload(Payload);
	}

	template<typename PayloadType>
	const PayloadType* FindRuntimePayload() const
	{
		if (const FInstancedStruct* Found = FindRuntimePayload(PayloadType::StaticStruct()))
		{
			return Found->GetPtr<PayloadType>();
		}
		return nullptr;
	}

	/** 复制FGameplayEffectContext，包括深拷贝HitResult */
	virtual FGameplayEffectContext* Duplicate() const override
	{
		FYcGameplayEffectContext* NewContext = new FYcGameplayEffectContext();
		*NewContext = *this;
		if (GetHitResult())
		{
			// 对碰撞结果进行深拷贝
			NewContext->AddHitResult(*GetHitResult(), true);
		}
		return NewContext;
	}

	/** 获取此结构体的脚本结构体信息 */
	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FYcGameplayEffectContext::StaticStruct();
	}

	/** 网络序列化，包括新增字段的序列化 */
	virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess) override;

	/**
	 * 从碰撞结果中获取物理材质
	 * @return 碰撞击中的物理材质，如果没有则返回nullptr
	 */
	const UPhysicalMaterial* GetPhysicalMaterial() const;

public:
	/**
	 * 弹匣ID
	 * 用于识别来自同一弹匣的多发子弹
	 */
	UPROPERTY()
	int32 CartridgeID = -1;

	/**
	 * 伤害类型标签
	 * 用于伤害计算时识别伤害类型（物理、火焰、冰冻等）
	 */
	UPROPERTY()
	FGameplayTag DamageTypeTag;

	/**
	 * 设置伤害类型标签
	 */
	void SetDamageTypeTag(const FGameplayTag& InTag) { DamageTypeTag = InTag; }

	/**
	 * 获取伤害类型标签
	 */
	const FGameplayTag& GetDamageTypeTag() const { return DamageTypeTag; }

	/**
	 * 运行时 Payload 容器
	 * 用于承载一次攻击的扩展上下文（如近战背刺、蓄力、连击段等）
	 * 当前默认不做网络序列化，供本地/权威端执行链路消费
	 */
	UPROPERTY()
	TArray<FInstancedStruct> RuntimePayloads;

protected:
	/**
	 * 技能效果源对象
	 * 应实现IYcAbilitySourceInterface接口，当前不支持网络复制
	 */
	UPROPERTY()
	TWeakObjectPtr<const UObject> AbilitySourceObject;
};

template<>
struct TStructOpsTypeTraits<FYcGameplayEffectContext> : public TStructOpsTypeTraitsBase2<FYcGameplayEffectContext>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true
	};
};
