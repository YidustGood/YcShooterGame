/**
 * 带世界展示模型的拾取Actor。
 * 基于拾取库存中的主物品，异步加载并应用 Asset.Visual.Pickup 资源。
 */
class AYcDisplayMeshPickupActor : AYcPickupActor
{
	UPROPERTY(DefaultComponent, RootComponent)
	USceneComponent SceneRoot;

	UPROPERTY(DefaultComponent, Attach = SceneRoot)
	UStaticMeshComponent DisplayMeshComp;

	/** 是否在展示模型应用完成后开启物理模拟。 */
	UPROPERTY(EditAnywhere, Category = "Pickup|Visual")
	bool bEnableDisplayMeshPhysics = false;

	private FGameplayMessageListenerHandle PickupVisualAssetLoadedHandle;

	UFUNCTION(BlueprintOverride)
	void BeginPlay()
	{
		Super::BeginPlay();

		PickupVisualAssetLoadedHandle = UGameplayMessageSubsystem::Get().RegisterListener(
			FGameplayTag::RequestGameplayTag(n"Asset.Visual.Pickup"),
			this,
			n"OnPickupVisualDataLoaded",
			FYcDataAssetLifecycleMessage(),
			EGameplayMessageMatch::ExactMatch);

		YcPickupableComp.OnPickupInventoryChanged.AddUFunction(this, n"RefreshDisplayMeshFromPickupInventory");
		YcPickupableComp.OnPickupInventoryChanged.AddUFunction(this, n"RefreshInteractionPromptFromPickupInventory");
		RefreshDisplayMeshFromPickupInventory();
		RefreshInteractionPromptFromPickupInventory();
	}

	UFUNCTION(BlueprintOverride)
	void EndPlay(EEndPlayReason Reason)
	{
		PickupVisualAssetLoadedHandle.Unregister();
	}

	UFUNCTION()
	void RefreshDisplayMeshFromPickupInventory()
	{
		FDataRegistryId ItemRegistryId;
		if (!YcInventory::GetPrimaryPickupItemRegistryId(YcPickupableComp.StaticInventory, ItemRegistryId))
		{
			return;
		}

		YcInventory::LoadItemDataAssetByTagAsync(ItemRegistryId, FGameplayTag::RequestGameplayTag(n"Asset.Visual.Pickup"));
	}

	UFUNCTION()
	void RefreshInteractionPromptFromPickupInventory()
	{
		FYcInteractionOption& NewOption = YcInteractableComp.GetInteractionOption();
		NewOption.Text = FText::FromString("拾取");

		FDataRegistryId ItemRegistryId;
		if (YcInventory::GetPrimaryPickupItemRegistryId(YcPickupableComp.StaticInventory, ItemRegistryId))
		{
			FYcInventoryItemDefinition ItemDef;
			if (YcInventory::GetItemDefinition(ItemRegistryId, ItemDef) && !ItemDef.DisplayName.IsEmpty())
			{
				NewOption.Text = FText::FromString("拾取 " + ItemDef.DisplayName.ToString());
			}
		}

		YcInteractableComp.SetInteractionOption(NewOption);
	}

	UFUNCTION()
	void OnPickupVisualDataLoaded(FGameplayTag ActualTag, FYcDataAssetLifecycleMessage Data)
	{
		if (Data.RelatedObject != this || !Data.bIsLoaded)
		{
			return;
		}

		auto PickupVisualData = Cast<UYcPickupVisualData>(Data.LoadedDataAsset);
		if (PickupVisualData == nullptr)
		{
			return;
		}

		auto DisplayMesh = PickupVisualData.GetResolvedDisplayMesh();
		if (DisplayMesh == nullptr)
		{
			return;
		}

		DisplayMeshComp.SetStaticMesh(DisplayMesh);
		DisplayMeshComp.SetRelativeTransform(PickupVisualData.MeshRelativeTransform);
		DisplayMeshComp.SetSimulatePhysics(false);
		DisplayMeshComp.SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);

		auto OverrideMaterials = PickupVisualData.GetResolvedOverrideMaterials();
		for (int32 MaterialIndex = 0; MaterialIndex < OverrideMaterials.Num(); ++MaterialIndex)
		{
			if (OverrideMaterials[MaterialIndex] != nullptr)
			{
				DisplayMeshComp.SetMaterial(MaterialIndex, OverrideMaterials[MaterialIndex]);
			}
		}

		if (bEnableDisplayMeshPhysics)
		{
			DisplayMeshComp.SetSimulatePhysics(true);
		}
	}
}
