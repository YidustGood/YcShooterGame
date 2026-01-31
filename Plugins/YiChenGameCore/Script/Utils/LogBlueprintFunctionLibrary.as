namespace LogUtils
{
	UENUM()
	enum EScriptLogType
	{
		Info,
		Warning,
		Error
	}

	UFUNCTION(DisplayName = "DEBUG Log Message", meta = (WorldContext = "WorldContextObject"))
	void DebugLogMessage(UObject WorldContextObject, EScriptLogType LogType, FString LogMessage, float32 Duration = 5.f, FLinearColor Color = FLinearColor::Blue)
	{
		switch (LogType)
		{
			case EScriptLogType::Info:
				Print(f"{WorldContextObject.GetName()}:{LogMessage}", Duration, Color);
				break;
			case EScriptLogType::Warning:
				PrintWarning(f"{WorldContextObject.GetName()}:{LogMessage}", Duration);
				break;
			case EScriptLogType::Error:
				PrintError(f"{WorldContextObject.GetName()}:{LogMessage}", Duration);
		}
	}

	UFUNCTION(DisplayName = "Log Message", meta = (WorldContext = "WorldContextObject"))
	void LogMsg(UObject WorldContextObject, FString LogMessage, EScriptLogType LogType = LogUtils::EScriptLogType::Info)
	{
		switch (LogType)
		{
			case EScriptLogType::Info:
				Log(f"{WorldContextObject.GetName()}:{LogMessage}");
				break;
			case EScriptLogType::Warning:
				Warning(f"{WorldContextObject.GetName()}:{LogMessage}");
				break;
			case EScriptLogType::Error:
				Error(f"{WorldContextObject.GetName()}:{LogMessage}");
		}
	}
}
