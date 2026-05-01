// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "YcQuestTypes.h"
#include "YcQuestDefinition.generated.h"

class UYcQuestEffect;
class UYcQuestObjective;

/** 任务关联资源引用，用于声明某个 Bundle 需要加载的对象或类资源。 */
USTRUCT(BlueprintType)
struct YICHENQUEST_API FYcQuestAssetRef
{
    GENERATED_BODY()

    /** 资源所属的 Bundle 名称。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    FName BundleName = NAME_None;

    /** 需要按对象形式加载的软引用资源。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    TSoftObjectPtr<UObject> Asset;

    /** 需要按类形式加载的软引用资源。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    TSoftClassPtr<UObject> AssetClass;
};

/** 任务阶段到 Bundle 列表的映射表。 */
USTRUCT(BlueprintType)
struct YICHENQUEST_API FYcQuestPhaseBundleMapping
{
    GENERATED_BODY()

    /** 任务阶段，例如接取、完成、失败等。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    EYcQuestPhase Phase = EYcQuestPhase::OnAccepted;

    /** 该阶段需要处理的 Bundle 名称列表。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    TArray<FName> BundleNames;
};

/**
 * 任务定义资产。
 * 它描述一个任务的静态配置，包括任务作用域、持久化策略、归属模式、目标树、效果链以及业务扩展负载，
 * 是运行时创建任务实例的模板来源。
 */
UCLASS(BlueprintType)
class YICHENQUEST_API UYcQuestDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    /** 任务定义在 AssetManager 中使用的主资源类型。 */
    static const FPrimaryAssetType QuestDefinitionType;

    /** 返回任务定义的主资源 Id。 */
    virtual FPrimaryAssetId GetPrimaryAssetId() const override;

#if WITH_EDITOR
    /** 编辑器数据校验，确保任务定义满足最基本的可运行条件。 */
    virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

#if WITH_EDITORONLY_DATA
    /** 编辑器下根据资源声明刷新 Asset Bundle 元数据。 */
    virtual void UpdateAssetBundleData() override;
#endif

public:
    /** 任务唯一标识。 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest")
    FName QuestId = NAME_None;

    /** 任务作用域，决定它是全局持久、跨局持久还是仅对局内有效。 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest")
    EYcQuestScope QuestScope = EYcQuestScope::GlobalPersistent;

    /** 任务持久化策略，决定它是否必须进入存档。 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest")
    EYcQuestPersistencePolicy PersistencePolicy = EYcQuestPersistencePolicy::Required;

    /** 任务进度归属模式，决定是个人进度还是共享进度。 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest")
    EYcQuestProgressOwnershipMode ProgressOwnershipMode = EYcQuestProgressOwnershipMode::PerPlayer;

    /** 根目标节点，任务运行时会从这里展开整棵目标树。 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Instanced, Category = "Quest")
    TObjectPtr<UYcQuestObjective> RootObjective = nullptr;

    /** 任务级效果列表，通常在任务状态切换时触发。 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Instanced, Category = "Quest")
    TArray<TObjectPtr<UYcQuestEffect>> QuestEffects;

    /** 业务层扩展负载，用于挂载任务系统之外的自定义语义。 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest|Business")
    FInstancedStruct BusinessPayload;

    /** 任务声明的资源引用。 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest")
    TArray<FYcQuestAssetRef> AssetRefs;

    /** 各任务阶段对应的 Bundle 策略。 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest")
    TArray<FYcQuestPhaseBundleMapping> BundlePolicy;

    /** 任务完成后仍需保留的 Bundle 集合。 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest")
    TSet<FName> RetainedBundlesAfterComplete;
};
