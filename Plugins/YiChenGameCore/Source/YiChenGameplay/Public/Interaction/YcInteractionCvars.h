#pragma once

namespace YcConsoleVariables
{
	// 控制是否绘制交互对象查询范围。 使用方法: 1.引入本头文件 2.YcConsoleVariables::CVarDrawDebugShape.GetValueOnGameThread()获取值
	static TAutoConsoleVariable<bool> CVarDrawDebugShape(
		TEXT("Yc.Interact.DrawDebugShape"),
		false,
		TEXT("是否绘制交互系统的调试形状."),
		ECVF_Default);
}