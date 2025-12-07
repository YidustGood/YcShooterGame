// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Engine/StreamableManager.h"

/** 资产管理器启动工作进度更新委托 */
DECLARE_DELEGATE_OneParam(FYcAssetManagerStartupJobSubstepProgress, float /*NewProgress*/);

/**
 * 资产管理器启动工作
 * 
 * 表示资产管理器启动过程中的一个工作任务，处理来自FStreamableHandle的进度报告, 支持异步加载和进度跟踪。
 * 每个工作任务都有一个权重，用于计算整体加载进度。
 */
struct FYcAssetManagerStartupJob
{
	/** 工作进度更新委托 */
	FYcAssetManagerStartupJobSubstepProgress SubstepProgressDelegate;
	
	/** 工作函数，执行实际的加载逻辑 */
	TFunction<void(const FYcAssetManagerStartupJob&, TSharedPtr<FStreamableHandle>&)> JobFunc;
	
	/** 工作名称 */
	FString JobName;
	
	/** 工作权重，用于计算整体进度 */
	float JobWeight;
	
	/** 上次进度更新的时间戳 */
	mutable double LastUpdate = 0;

	/**
	 * 构造函数
	 * 简单的工作，完全同步进行
	 * @param InJobName 工作名称
	 * @param InJobFunc 工作函数
	 * @param InJobWeight 工作权重
	 */
	FYcAssetManagerStartupJob(const FString& InJobName, const TFunction<void(const FYcAssetManagerStartupJob&, TSharedPtr<FStreamableHandle>&)>& InJobFunc, float InJobWeight)
		: JobFunc(InJobFunc)
		  , JobName(InJobName)
		  , JobWeight(InJobWeight)
	{
	}

	/**
	 * 执行工作任务
	 * @return 流式加载句柄，如果创建了异步加载则返回有效句柄
	 */
	TSharedPtr<FStreamableHandle> DoJob() const;

	/**
	 * 更新工作进度
	 * @param NewProgress 新的进度值（0.0-1.0）
	 */
	void UpdateSubstepProgress(float NewProgress) const
	{
		SubstepProgressDelegate.ExecuteIfBound(NewProgress);
	}

	/**
	 * 从流式加载句柄更新工作进度
	 * 为避免频繁查询句柄进度，限制更新频率为60Hz
	 * @param StreamableHandle 流式加载句柄
	 */
	void UpdateSubstepProgressFromStreamable(TSharedRef<FStreamableHandle> StreamableHandle) const
	{
		if (SubstepProgressDelegate.IsBound())
		{
			/** 
			 * 限制进度查询频率，避免性能问题 
			 * 因为“StreamableHandle::GetProgress”函数会遍历一个庞大的图结构，其执行效率相当低下。
			 */
			double Now = FPlatformTime::Seconds();
			if (LastUpdate - Now > 1.0 / 60)
			{
				SubstepProgressDelegate.Execute(StreamableHandle->GetProgress());
				LastUpdate = Now;
			}
		}
	}
};
