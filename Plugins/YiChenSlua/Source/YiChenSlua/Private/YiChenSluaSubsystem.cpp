// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "YiChenSluaSubsystem.h"

#include "YiChenSlua.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(YiChenSluaSubsystem)

// read file content
static uint8* ReadFile(IPlatformFile& PlatformFile, FString path, uint32& len) {
	IFileHandle* FileHandle = PlatformFile.OpenRead(*path);
	if (FileHandle) {
		len = (uint32)FileHandle->Size();
		uint8* buf = new uint8[len];

		FileHandle->Read(buf, len);

		// Close the file again
		delete FileHandle;

		return buf;
	}

	return nullptr;
}

void UYiChenSluaSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	UE_LOG(LogYcSlua, Log, TEXT("YiChenSluaSubsystem Initialized"));
	
	if (!HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		CreateLuaState();
	}
	
}

void UYiChenSluaSubsystem::Deinitialize()
{
	CloseLuaState();
	Super::Deinitialize();
}

bool UYiChenSluaSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return true;
}

void UYiChenSluaSubsystem::RunLuaFile(FString FileName)
{
	if (state == nullptr) return;
	if (FileName.IsEmpty()) return;
	
	slua::LuaVar v = state->doFile(TCHAR_TO_UTF8(*FileName));
}

static int32 PrintLog(NS_SLUA::lua_State *L)
{
	FString str;
	size_t len;
	const char* s = luaL_tolstring(L, 1, &len);
	if (s) str += UTF8_TO_TCHAR(s);
	NS_SLUA::Log::Log("PrintLog %s", TCHAR_TO_UTF8(*str));
	return 0;
}

void UYiChenSluaSubsystem::LuaStateInitCallback(slua::lua_State* L)
{
	lua_pushcfunction(L, PrintLog);
	lua_setglobal(L, "PrintLog");
}

void UYiChenSluaSubsystem::CreateLuaState()
{
	NS_SLUA::LuaState::onInitEvent.AddUObject(this, &UYiChenSluaSubsystem::LuaStateInitCallback);

	CloseLuaState();
	state = new NS_SLUA::LuaState("SLuaMainState", GetGameInstance());
	state->setLoadFileDelegate([](const char* fn, FString& filepath)->TArray<uint8> {

		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		FString path = FPaths::ProjectContentDir();
		FString filename = UTF8_TO_TCHAR(fn);
		path /= "Lua";
		path /= filename.Replace(TEXT("."), TEXT("/"));

		TArray<uint8> Content;
		TArray<FString> luaExts = { UTF8_TO_TCHAR(".lua"), UTF8_TO_TCHAR(".luac") };
		for (auto& it : luaExts) {
			auto fullPath = path + *it;

			FFileHelper::LoadFileToArray(Content, *fullPath);
			if (Content.Num() > 0) {
				filepath = fullPath;
				return MoveTemp(Content);
			}
		}

		return MoveTemp(Content);
	});
	state->init();
}

void UYiChenSluaSubsystem::CloseLuaState()
{
	if (state)
	{
		state->close();
		delete state;
		state = nullptr;
	}
}
