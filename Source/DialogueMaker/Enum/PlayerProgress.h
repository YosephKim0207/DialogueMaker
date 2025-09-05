#pragma once

#include "PlayerProgress.generated.h"

UENUM(BlueprintType)
enum class ENPCID : uint8
{
	// TODO Add Dialogue owned NPC name here
	TestNPC UMETA(DisplayName = "TestNPC"),
	TestNPC2 UMETA(DisplayName = "TestNPC2"),
};

UENUM(BlueprintType)
enum class EChapterID : uint8
{
	// TODO Add Story Chapter here
	Chapter01 UMETA(DisplayName = "Chapter01"),
	Chapter02 UMETA(DisplayName = "Chapter02"),
};
