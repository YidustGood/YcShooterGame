// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Character/YcPersistenceMessages.h"

namespace YcPersistenceTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Persistence_MarkDirty, "Yc.Persistence.MarkDirty", "Mark active profile dirty.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Persistence_RequestAutosave, "Yc.Persistence.RequestAutosave", "Request autosave for active profile.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Persistence_RequestFlushSave, "Yc.Persistence.RequestFlushSave", "Request immediate save flush for active profile.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Persistence_RequestCommitMatchResult, "Yc.Persistence.RequestCommitMatchResult", "Request in-match commit for active profile.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Persistence_ProfileHydrated, "Yc.Persistence.ProfileHydrated", "Active profile hydration finished.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Persistence_ProfileChanged, "Yc.Persistence.ProfileChanged", "Active profile identity changed.");
}
