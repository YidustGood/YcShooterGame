// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

// 获取是服务器还是服务端
YICHENGAMECORE_API FString GetClientServerContextString(UObject* ContextObject = nullptr);

// Msg直接传入TEXT("Message")，或者*FString对象
#define YC_LOG(CategoryName, Verbosity, Msg) \
UE_LOG(CategoryName, Verbosity, TEXT("%hs: %s"), __FUNCTION__, Msg)