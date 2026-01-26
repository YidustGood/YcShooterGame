#if EDITOR

/**
 * Shows how to add new options to the context menu for actors in a level,
 * using UScriptActorMenuExtension.
 * 为关卡中的Actor添加右键菜单选项
 */
class UExampleActorMenuExtension : UScriptActorMenuExtension
{
	// Specify one or more classes for which the menu options show
	default SupportedClasses.Add(AActor);

	// Every function with the CallInEditor specifier will become a context menu option
	UFUNCTION(CallInEditor)
	void ExampleActorMenuExtension()
	{
	}

	// The function's Category will be used to create sub-menus,该函数的类别将用于创建子菜单
	UFUNCTION(CallInEditor, Category = "Example Category")
	void ExampleOptionInSubCategory()
	{
	}

	// Custom icons can be specified with the `EditorIcon` meta tag:，指定图标
	UFUNCTION(CallInEditor, Category = "Example Category", Meta = (EditorIcon = "Icons.Link"))
	void ExampleOptionWithIcon()
	{
	}

	// If the function takes an actor parameter, it will be called once for every selected actor
	// 为每一个选中的Actor调用一次该方法，参数就是被选中Actor
	UFUNCTION(CallInEditor, Category = "Example Category")
	void CalledForEverySelectedActor(AActor SelectedActor)
	{
		Print(f"Actor {SelectedActor} is selected");
	}

	// If the function has any other parameters, a dialog popup will be shown prompting for values
	// 如果函数有参数，会弹出对话框
	UFUNCTION(CallInEditor, Category = "Example Category")
	void OpensPromptForParameters(AActor SelectedActor, bool bCheckboxParameter = true, FVector VectorParameter = FVector::ZeroVector)
	{
	}
};

/**
 * Using UScriptAssetMenuExtension allows adding options to the context menu
 * of assets in the Content Browser.
 * 为内容浏览器资产创建上下文菜单功能
 */
class UExampleAssetMenuExtension : UScriptAssetMenuExtension
{
	// These options should only show up for textures
	default SupportedClasses.Add(UTexture2D);

	// When clicked, will be called once for every selected texture asset
	UFUNCTION(CallInEditor, Category = "Example Texture Actions")
	void ExampleModifyTextureLODBias(FAssetData SelectedAsset, int LODBias = 0)
	{
		UTexture2D SelectedTexture = Cast<UTexture2D>(SelectedAsset.GetSoftObjectPath().TryLoad());
		SelectedTexture.Modify();
		SelectedTexture.LODBias = LODBias;
	}
};

/**
 * Other editor menus can be extended with UScriptEditorMenuExtension.
 * 其他类型编辑器菜单
 */
class UExampleEditorMenuExtension : UScriptEditorMenuExtension
{
	// This is the same extension point used by UToolMenus::ExtendMenu
	// In this example, we extend the top menu of the main window:
	// 这与UToolMenus::ExtendMenu使用的扩展点相同
	// 在这个例子中，我们扩展了主窗口的顶部菜单:
	default ExtensionPoint = n"MainFrame.MainMenu";

	UFUNCTION(CallInEditor, Category = "My Example Menu")
	void ExampleMainMenuOption()
	{
	}

	UFUNCTION(CallInEditor, Category = "My Example Menu | Example Sub Menu")
	void ExampleSubMenuOption()
	{
	}
};

/**
 * Toolbars can also be extended. Each function will become its own toolbar button.
 * 工具栏也可以扩展。每个函数都将成为自己的工具栏按钮。
 */
class UExampleToolbarExtension : UScriptEditorMenuExtension
{
	// Add to the extension point at the end of the level editor toolbar:
	default ExtensionPoint = n"LevelEditor.LevelEditorToolBar.User";

	// Will become a toolbar button displayed using the editor's "Paste" icon
	UFUNCTION(CallInEditor, Meta = (EditorIcon = "GenericCommands.Paste"))
	void ExampleToolbarButton()
	{
		TMap<FString, FString> LanguageMap;
		LanguageMap.Add("en", "英语");
		LanguageMap.Add("zh-Hans", "中文");

		FString CurrentLanguageCode = Internationalization::GetCurrentLanguage();
		Log(CurrentLanguageCode);
		if (CurrentLanguageCode == "en")
		{
			Internationalization::SetCurrentLanguage("zh-Hans", true);
		}
		else if (CurrentLanguageCode == "zh-Hans")
		{
			Internationalization::SetCurrentLanguage("en", true);
		}
	}

	// Customize the style to make the display name appear in the toolbar instead of just the icon
	UFUNCTION(CallInEditor, DisplayName = "Extension", Meta = (EditorIcon = "Icons.Plus", EditorButtonStyle = "CalloutToolbar"))
	void ExampleToolbarButtonWithDisplayName()
	{
	}
};

#endif
