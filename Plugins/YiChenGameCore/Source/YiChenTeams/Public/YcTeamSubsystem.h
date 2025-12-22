// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "YcTeamSubsystem.generated.h"

struct FGameplayTag;
class AYcTeamInfoBase;
class IYcTeamAgentInterface;
class AYcTeamPublicInfo;
class UYcTeamAsset;

/** 团队资源变更委托，当团队的资源发生变化时触发 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnYcTeamAssetChangedDelegate, const UYcTeamAsset*, TeamAsset);

/** 团队跟踪信息结构体，用于存储和管理团队的相关信息 */
USTRUCT()
struct YICHENTEAMS_API FYcTeamTrackingInfo
{
	GENERATED_BODY()
public:
	/** 团队的公开信息对象，存储团队的公开数据 */
	UPROPERTY()
	TObjectPtr<AYcTeamPublicInfo> PublicInfo = nullptr;

	/** 团队资源资产，存储团队的配置数据（如颜色、图标等） */
	UPROPERTY()
	TObjectPtr<UYcTeamAsset> TeamAsset = nullptr;

	/** 团队资源变更委托，当团队资源发生变化时触发 */
	UPROPERTY()
	FOnYcTeamAssetChangedDelegate OnTeamAssetChanged;

public:
	/**
	 * 设置团队信息
	 * @param Info 要设置的团队信息对象
	 */
	void SetTeamInfo(AYcTeamInfoBase* Info);
	
	/**
	 * 移除团队信息
	 * @param Info 要移除的团队信息对象
	 */
	void RemoveTeamInfo(AYcTeamInfoBase* Info);
};

/** 比较两个Actor的团队归属关系的结果 */
UENUM(BlueprintType)
enum class EYcTeamComparison : uint8
{
	/** 两个Actor属于同一团队 */
	OnSameTeam,

	/** 两个Actor属于敌对团队 */
	DifferentTeams, 

	/** 一个或两个Actor无效或不属于任何团队 */
	InvalidArgument
};

/**
 * 团队子系统，用于方便地访问基于团队的Actor（如Pawn或PlayerState）的团队信息
 * 推荐做法是在PlayerState中实现IYcTeamAgentInterface接口
 * 或者给PlayerState挂载UYcTeamComponent组件, 该组件提供了基本的团队接口实现
 * 在通过团队子系统查找Actor团队信息的时候会迭代寻找, 最终是会在PlayerState或者其组件上寻找团队接口实现来进行调用
 */
UCLASS()
class YICHENTEAMS_API UYcTeamSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	UYcTeamSubsystem();

	//~USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~End of USubsystem interface
	
	/**
	 * 获取团队子系统的实例
	 * @param WorldContextObject 用于获取世界上下文的对象
	 * @return 团队子系统的引用
	 */
	static UYcTeamSubsystem& Get(const UObject* WorldContextObject);
	
	/**
	 * 尝试注册一个新的团队信息
	 * @param TeamInfo 要注册的团队信息对象
	 * @return 注册成功返回true，否则返回false
	 */
	bool RegisterTeamInfo(AYcTeamInfoBase* TeamInfo);
	
	/**
	 * 尝试注销一个团队信息
	 * @param TeamInfo 要注销的团队信息对象
	 * @return 注销成功返回true，否则返回false
	 */
	bool UnregisterTeamInfo(AYcTeamInfoBase* TeamInfo);
	
	/**
	 * 检查指定的团队是否存在
	 * @param TeamId 要检查的团队ID
	 * @return 团队存在返回true，否则返回false
	 */
	UFUNCTION(BlueprintCallable, Category=Teams)
	bool DoesTeamExist(int32 TeamId) const;
	
	/**
	 * 获取所有团队的ID列表
	 * @return 排序后的团队ID数组
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category=Teams)
	TArray<int32> GetTeamIDs() const;

	/**
	 * 更改指定Actor关联的团队
	 * 注意：此函数只能在服务器端调用
	 * @param ActorToChange 要更改团队的Actor
	 * @param NewTeamId 新的团队ID
	 * @return 更改成功返回true，否则返回false
	 */
	bool ChangeTeamForActor(AActor* ActorToChange, int32 NewTeamId);
	
	/**
	 * 从Actor中查找团队代理接口
	 * @param PossibleTeamActor 可能包含团队接口的Actor
	 * @param OutTeamAgent 输出的团队代理接口
	 * @return 找到团队代理接口返回true，否则返回false
	 */
	UFUNCTION(BlueprintCallable, Category = "Team")
	static bool FindTeamAgentFromActor(AActor* PossibleTeamActor, TScriptInterface<IYcTeamAgentInterface>& OutTeamAgent);
	
	/**
	 * 从Actor的组件中查找团队代理接口
	 * @param PossibleTeamActor 可能包含团队接口的Actor
	 * @param OutTeamAgent 输出的团队代理接口
	 * @return 找到团队代理接口返回true，否则返回false
	 */
	UFUNCTION(BlueprintCallable, Category = "Team")
	static bool FindTeamAgentFromActorComponents(AActor* PossibleTeamActor, TScriptInterface<IYcTeamAgentInterface>& OutTeamAgent);
	
	/**
	 * 查找对象所属的团队ID
	 * @param TestObject 要测试的对象
	 * @return 对象所属的团队ID，如果不属于任何团队则返回INDEX_NONE
	 */
	static int32 FindTeamFromObject(UObject* TestObject);
	
	/**
	 * 查找Actor所属的团队ID
	 * @param TestActor 要测试的Actor
	 * @param bIsPartOfTeam 输出参数，表示是否属于某个团队
	 * @param TeamId 输出参数，团队ID
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category=Teams, meta=(Keywords="Get"))
	void FindTeamFromActor(AActor* TestActor, bool& bIsPartOfTeam, int32& TeamId) const;
	
	/**
	 * 比较两个对象的团队关系，并输出各自的团队ID
	 * @param A 第一个要比较的对象
	 * @param B 第二个要比较的对象
	 * @param TeamIdA 输出参数，对象A的团队ID
	 * @param TeamIdB 输出参数，对象B的团队ID
	 * @return 团队比较结果枚举值
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category=Teams, meta=(ExpandEnumAsExecs=ReturnValue))
	EYcTeamComparison CompareTeamsOut(UObject* A, UObject* B, int32& TeamIdA, int32& TeamIdB) const;

	/**
	 * 比较两个对象的团队关系
	 * @param A 第一个要比较的对象
	 * @param B 第二个要比较的对象
	 * @return 团队比较结果枚举值
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category=Teams, meta=(ExpandEnumAsExecs=ReturnValue))
	EYcTeamComparison CompareTeams(UObject* A, UObject* B) const;
	
	/**
	 * 判断伤害发起者是否可以对目标造成伤害，考虑友军伤害设置
	 * @param Instigator 伤害发起者
	 * @param Target 伤害目标
	 * @param bAllowDamageToSelf 是否允许对自己造成伤害，默认为true
	 * @return 可以造成伤害返回true，否则返回false
	 */
	bool CanCauseDamage(UObject* Instigator, UObject* Target, bool bAllowDamageToSelf = true) const;
	
	/**
	 * 为指定团队的标签添加指定数量的堆栈
	 * 注意：当 StackCount 小于 1 时不执行任何操作
	 * @param TeamId 要操作的团队ID
	 * @param Tag 要添加堆栈的标签
	 * @param StackCount 要添加的堆栈数量
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Teams)
	void AddTeamTagStack(const int32 TeamId, const FGameplayTag Tag, const int32 StackCount);

	/**
	 * 为指定团队的标签移除指定数量的堆栈
	 * 注意：当 StackCount 小于 1 时不执行任何操作
	 * @param TeamId 要操作的团队ID
	 * @param Tag 要移除堆栈的标签
	 * @param StackCount 要移除的堆栈数量
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Teams)
	void RemoveTeamTagStack(const int32 TeamId, const FGameplayTag Tag, const int32 StackCount);

	/**
	 * 获取指定团队在指定标签上的堆栈数量
	 * @param TeamId 要查询的团队ID
	 * @param Tag 要查询的标签
	 * @return 标签当前的堆栈数量，如果标签不存在则返回0
	 */
	UFUNCTION(BlueprintCallable, Category=Teams)
	int32 GetTeamTagStackCount(const int32 TeamId, const FGameplayTag Tag) const;

	/**
	 * 判断指定团队是否至少拥有一层指定标签的堆栈
	 * @param TeamId 要查询的团队ID
	 * @param Tag 要查询的标签
	 * @return 至少有一层堆栈返回true，否则返回false
	 */
	UFUNCTION(BlueprintCallable, Category=Teams)
	bool TeamHasTag(const int32 TeamId, const FGameplayTag Tag) const;
	
	/**
	 * 从指定观察者团队的视角获取目标团队的展示资产
	 * 在某些模式下（例如“本地玩家始终为蓝队”），不同观察者看到的同一团队可能使用不同的展示资产
	 * @param TeamId 目标团队ID
	 * @param ViewerTeamId 观察者所属的团队ID
	 * @return 用于展示的团队资产指针，如果未配置则返回nullptr
	 */
	UFUNCTION(BlueprintCallable, Category=Teams)
	UYcTeamAsset* GetTeamAsset(int32 TeamId, int32 ViewerTeamId);
 
	/**
	 * 从指定观察者对象的视角获取目标团队的最终展示资产
	 * 会根据观察者对象所属团队（通过团队系统查询）来决定返回哪一个展示资产
	 * @param TeamId 目标团队ID
	 * @param ViewerTeamAgent 观察者对象（需能通过团队系统推导其TeamId）
	 * @return 用于展示的团队资产指针，如果未配置则返回nullptr
	 */
	UFUNCTION(BlueprintCallable, Category = Teams)
	UYcTeamAsset* GetEffectiveTeamAsset(int32 TeamId, UObject* ViewerTeamAgent);
 
	/**
	 * 当某个团队展示资产被编辑时调用
	 * 会触发相关团队的资产变更委托，通知所有观察者更新（例如刷新队伍颜色/图标）
	 * @param ModifiedAsset 已被修改的团队展示资产
	 */
	void NotifyTeamAssetModified(UYcTeamAsset* ModifiedAsset);
 
	/**
	 * 为指定团队ID注册团队资产变更通知
	 * 可获取该团队对应的资产变更委托，用于绑定监听逻辑
	 * @param TeamId 要监听的团队ID
	 * @return 该团队对应的资产变更委托引用
	 */
	FOnYcTeamAssetChangedDelegate& GetTeamAssetChangedDelegate(int32 TeamId);
private:
	/** 团队ID到团队跟踪信息的映射表 */
	UPROPERTY()
	TMap<int32, FYcTeamTrackingInfo> TeamMap;

	/** 作弊管理器注册句柄 */
	FDelegateHandle CheatManagerRegistrationHandle;
};