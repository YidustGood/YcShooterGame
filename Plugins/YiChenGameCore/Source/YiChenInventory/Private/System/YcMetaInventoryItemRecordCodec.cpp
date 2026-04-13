// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "System/YcMetaInventoryItemRecordCodec.h"

#include "Algo/Sort.h"
#include "YcInventoryItemInstance.h"

namespace
{
	void SortAndNormalizeExtensionPayloads(TArray<FYcMetaItemExtensionPayload>& InOutPayloads)
	{
		for (int32 Index = InOutPayloads.Num() - 1; Index >= 0; --Index)
		{
			if (InOutPayloads[Index].ExtensionKey.IsNone())
			{
				InOutPayloads.RemoveAtSwap(Index);
			}
		}

		Algo::Sort(InOutPayloads, [](const FYcMetaItemExtensionPayload& A, const FYcMetaItemExtensionPayload& B)
		{
			if (A.ExtensionKey == B.ExtensionKey)
			{
				return A.Version < B.Version;
			}
			return A.ExtensionKey.ToString() < B.ExtensionKey.ToString();
		});
	}
}

void YcMetaInventoryItemRecordCodec::ExportFromItem(const UYcInventoryItemInstance& Item, const TArray<FYcMetaItemExtensionPayload>* UnknownPayloads, FYcMetaInventoryItemRecord& OutRecord)
{
	OutRecord.IntTagStacks.Reset();
	OutRecord.FloatTagStacks.Reset();
	OutRecord.ExtensionPayloads.Reset();

	TArray<TPair<FGameplayTag, int32>> IntStacks;
	TArray<TPair<FGameplayTag, float>> FloatStacks;
	FGameplayTagContainer OwnedTags;
	Item.ExportTagStates(IntStacks, FloatStacks, OwnedTags);

	Algo::Sort(IntStacks, [](const TPair<FGameplayTag, int32>& A, const TPair<FGameplayTag, int32>& B)
	{
		return A.Key.ToString() < B.Key.ToString();
	});
	OutRecord.IntTagStacks.Reserve(IntStacks.Num());
	for (const TPair<FGameplayTag, int32>& Pair : IntStacks)
	{
		if (!Pair.Key.IsValid() || Pair.Value <= 0)
		{
			continue;
		}
		FYcMetaTagIntValue Entry;
		Entry.Tag = Pair.Key;
		Entry.Value = Pair.Value;
		OutRecord.IntTagStacks.Add(Entry);
	}

	Algo::Sort(FloatStacks, [](const TPair<FGameplayTag, float>& A, const TPair<FGameplayTag, float>& B)
	{
		return A.Key.ToString() < B.Key.ToString();
	});
	OutRecord.FloatTagStacks.Reserve(FloatStacks.Num());
	for (const TPair<FGameplayTag, float>& Pair : FloatStacks)
	{
		if (!Pair.Key.IsValid() || Pair.Value <= 0.0f)
		{
			continue;
		}
		FYcMetaTagFloatValue Entry;
		Entry.Tag = Pair.Key;
		Entry.Value = Pair.Value;
		OutRecord.FloatTagStacks.Add(Entry);
	}

	OutRecord.OwnedTags = OwnedTags;

	if (UnknownPayloads)
	{
		OutRecord.ExtensionPayloads = *UnknownPayloads;
		SortAndNormalizeExtensionPayloads(OutRecord.ExtensionPayloads);
	}
}

void YcMetaInventoryItemRecordCodec::ImportToItem(UYcInventoryItemInstance& Item, const FYcMetaInventoryItemRecord& InRecord, TArray<FYcMetaItemExtensionPayload>& OutUnknownPayloads)
{
	TArray<TPair<FGameplayTag, int32>> IntStacks;
	IntStacks.Reserve(InRecord.IntTagStacks.Num());
	for (const FYcMetaTagIntValue& Entry : InRecord.IntTagStacks)
	{
		IntStacks.Emplace(Entry.Tag, Entry.Value);
	}

	TArray<TPair<FGameplayTag, float>> FloatStacks;
	FloatStacks.Reserve(InRecord.FloatTagStacks.Num());
	for (const FYcMetaTagFloatValue& Entry : InRecord.FloatTagStacks)
	{
		FloatStacks.Emplace(Entry.Tag, Entry.Value);
	}

	Item.ImportTagStates(IntStacks, FloatStacks, InRecord.OwnedTags);

	OutUnknownPayloads = InRecord.ExtensionPayloads;
	SortAndNormalizeExtensionPayloads(OutUnknownPayloads);
}

