#if EDITOR
// 这里我们自定义了一个更加便捷的编辑器工具栏按钮来快速打开GameplayTag管理器
// 这个功能依赖YcEditorUtilities插件

// 这个是显示在主界面的
class UEditorExtens_GameplayTag : UScriptEditorMenuExtension
{
	// 设置扩展UI的插槽位置
	default ExtensionPoint = n"LevelEditor.LevelEditorToolBar.PlayToolBar";

	// 打开GameplayTag管理窗口
	UFUNCTION(CallInEditor, DisplayName = "GameplayTags", Meta = (EditorIcon = "Icons.Plus", EditorButtonStyle = "CalloutToolbar"))
	void OpenGameplayTagsManager()
	{
		YcEditorUtilities::OpenGameplayTagsManager();
	}

	// 想扩展更多的Editor功能可以在编辑器偏好中开启 Display UI Extension Points以显示ExtensionPoint
	// 同时可以利用工具->调试->控件反射器来查找指定编辑器UI功能所对应的Slate代码片段,可以参考其实现进行自定义扩展
}

// @BUG 会导致编辑器打开部分特殊资产界面时崩溃掉,例如音频的参数补丁资产
// // 这个是显示在资产编辑界面的
// class UEditorExtens_GameplayTagAssetEditor : UScriptEditorMenuExtension
// {
//     default ExtensionPoint = n"AssetEditor.DefaultToolBar";

// 	UFUNCTION(CallInEditor, DisplayName = "GameplayTags", Meta = (EditorIcon = "Icons.Plus", EditorButtonStyle = "CalloutToolbar"))
// 	void OpenGameplayTagsManager()
// 	{
//         YcEditorUtilities::OpenGameplayTagsManager();
// 	}
// }
#endif