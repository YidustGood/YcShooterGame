// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Components/GameFrameworkInitStateInterface.h"
#include "Components/PawnComponent.h"
#include "YcPawnExtensionComponent.generated.h"

class UYcAbilitySystemComponent;
class UYcPawnData;

/**
 * Pawn扩展功能维护组件，为所有Pawn类添加功能扩展支持（例如角色、载具等）。
 * 该组件协调其他负责具体功能扩展组件的初始化，通过分阶段初始化确保依赖关系正确，这个组件需要比其它功能扩展组件先创建
 * 所有功能扩展组件(也可称为功能模块, 都有一个NAME_ActorFeatureName就是功能模块名称)都需要实现IGameFrameworkInitStateInterface接口, 以正确的参与到初始化流程中来
 * 
 * 在Pawn的PossessedBy、UnPossessed、OnRep_Controller、OnRep_PlayerState、SetupPlayerInputComponent这5个函数分别需要调用该组件的以下函数：
 * HandleControllerChanged、HandleControllerChanged、HandleControllerChanged、HandlePlayerStateReplicated、SetupPlayerInputComponent
 * 在PlayerState的ClientInitialize函数调用CheckDefaultInitialization
 * 目的都是为了通知该组件所属Pawn的PlayerController、PlayerState、InputComponent状态发生了变化, 因为很多功能扩展都依赖与这写Gameplay框架核心对象
 * 一旦某个依赖对象准备好了我们就会调用到CheckDefaultInitialization, 该函数会调用Pawn上所有功能扩展组件的CheckDefaultInitialization，
 * 通过各个功能扩展组件之间反复调用CheckDefaultInitialization，最终所有组件都会到达GameReady状态完成全部的初始化
 * 简单来说：就是通过组件内部调用推动+外部调用推动，其中一个组件改变了状态会触发所有组件尝试推动，最终达成全部推动完成。
 */   
UCLASS(ClassGroup=(YiChenGameCore), meta=(BlueprintSpawnableComponent))
class YICHENGAMEPLAY_API UYcPawnExtensionComponent : public UPawnComponent, public IGameFrameworkInitStateInterface
{
    GENERATED_BODY()
public:
    UYcPawnExtensionComponent(const FObjectInitializer& ObjectInitializer);
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    
    /**
     * 在指定Actor上查找Pawn扩展组件。
     * @param Actor 要查找的Actor
     * @return 找到的Pawn扩展组件，如果不存在则返回nullptr
     */
    UFUNCTION(BlueprintPure, Category = "YcGameCore|Pawn")
    static UYcPawnExtensionComponent* FindPawnExtensionComponent(const AActor* Actor) { return (Actor ? Actor->FindComponentByClass<UYcPawnExtensionComponent>() : nullptr); }
    
    /**
     * 获取Pawn配置数据，用于指定Pawn的属性。
     * @tparam T Pawn数据的具体类型
     * @return 指定类型的Pawn数据指针，如果类型不匹配则返回nullptr
     */
    template <class T>
    const T* GetPawnData() const { return Cast<T>(PawnData); }
    
    /**
     * 设置Pawn配置数据（仅在权威端执行）。
     * 设置后会强制网络更新以确保客户端及时同步，并尝试推进初始化状态。
     * @param InPawnData 要设置的Pawn数据
     */
    void SetPawnData(const UYcPawnData* InPawnData);
    
   /////////////// 初始化状态链 ///////////////
    /** 此特性的名称，用于初始化状态系统识别 */
    static const FName NAME_ActorFeatureName;

    //~ Begin IGameFrameworkInitStateInterface interface
    /** 
     * 获取此组件的功能名称, 需要保持全局唯一
     * 在UGameFrameworkComponentManager中就是以这个名称来维护各个组件的状态
     */
    virtual FName GetFeatureName() const override { return NAME_ActorFeatureName; }
    
    /**
     * 检查是否允许初始化状态转换。
     * @param Manager GameFrameworkComponentManager实例
     * @param CurrentState 当前初始化状态
     * @param DesiredState 期望转换到的初始化状态
     * @return 如果允许状态转换则返回true
     */
    virtual bool CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const override;
    
    /**
     * 处理初始化状态转换。
     * @param Manager GameFrameworkComponentManager实例
     * @param CurrentState 当前初始化状态
     * @param DesiredState 转换到的初始化状态
     */
    virtual void HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) override;
    
    /**
     * 当监听到其他组件状态改变时的回调。
     * @param Params 包含改变的组件特性名称和新状态的参数
     */
    virtual void OnActorInitStateChanged(const FActorInitStateChangedParams& Params) override;
    
    /**
     * 检查并尝试推进OwnerPawn所有功能扩展组件的初始化状态链。
     * 会调用Owner Pawn上所有实现IGameFrameworkInitStateInterface的组件的CheckDefaultInitialization()，
     * 然后沿着预定义的状态链尝试推进这些组件的初始化状态。
     * 当组件设置了一些数据后，就应调用此函数，尝试推进初始化进度，以便依赖该数据的其他组件能够继续推进他们的初始化进度。
     */
    virtual void CheckDefaultInitialization() override;
    //~ End IGameFrameworkInitStateInterface interface
   /////////////// ~初始化状态链 ///////////////
    
   /////////////// 技能系统相关 ///////////////
    /**
     * 获取当前的技能系统组件。
     * 技能系统组件可能由不同的Actor拥有（如PlayerState）。
     * @return 技能系统组件指针
     */
    UFUNCTION(BlueprintPure, Category = "YcGameCore|Pawn")
    UYcAbilitySystemComponent* GetYcAbilitySystemComponent() const { return AbilitySystemComponent; }

    /**
     * 初始化技能系统，将此Pawn设置为ASC的Avatar。
     * 如果已有其他Pawn作为Avatar，会先移除它。
     * @param InASC 要初始化的技能系统组件
     * @param InOwnerActor ASC的所有者Actor（通常是PlayerState）
     */
    void InitializeAbilitySystem(UYcAbilitySystemComponent* InASC, AActor* InOwnerActor);

    /**
     * 反初始化技能系统，移除此Pawn作为ASC的Avatar。
     * 会取消所有Ability、清除输入、移除GameplayCue等。
     */
    void UninitializeAbilitySystem();

    /**
     * 注册OnAbilitySystemInitialized委托，如果ASC已初始化则立即广播。
     * @param Delegate 要注册的委托
     */
    void OnAbilitySystemInitialized_RegisterAndCall(const FSimpleMulticastDelegate::FDelegate& Delegate);

    /**
     * 注册OnAbilitySystemUninitialized委托，当Pawn被移除为ASC的Avatar时触发。
     * @param Delegate 要注册的委托
     */
    void OnAbilitySystemUninitialized_Register(const FSimpleMulticastDelegate::FDelegate& Delegate);
   /////////////// ~技能系统相关 ///////////////
    
    /**
     * 处理Pawn的Controller变化。
     * 当Controller改变时应由Pawn调用此函数，以更新ASC的Owner信息。
     */
    void HandleControllerChanged();

    /**
     * 处理PlayerState复制完成。
     * 当PlayerState从服务器复制到客户端时应调用此函数，以推进初始化状态。
     */
    void HandlePlayerStateReplicated();

    /**
     * 处理InputComponent设置完成。
     * 当Pawn的InputComponent设置完成时应调用此函数，以推进初始化状态。
     */
    void SetupPlayerInputComponent();
    
protected:
    virtual void OnRegister() override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    
    /** PawnData网络复制回调 */
    UFUNCTION()
    void OnRep_PawnData();

    /** 当Pawn成为ASC的Avatar时广播的委托 */
    FSimpleMulticastDelegate OnAbilitySystemInitialized;

    /** 当Pawn被移除为ASC的Avatar时广播的委托 */
    FSimpleMulticastDelegate OnAbilitySystemUninitialized;
    
private:
    /** Pawn配置数据，用于指定Pawn的属性。通过网络复制同步 */
    UPROPERTY(EditInstanceOnly, ReplicatedUsing = OnRep_PawnData, Category = "YcGameCore|Pawn")
    TObjectPtr<const UYcPawnData> PawnData;
    
    /** 缓存的技能系统组件指针，便于快速访问 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "YcGameCore|Pawn", meta=(AllowPrivateAccess = "true"))
    TObjectPtr<UYcAbilitySystemComponent> AbilitySystemComponent;
};