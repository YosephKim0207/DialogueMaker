#pragma once

#include "Portrait.generated.h"

UENUM(BlueprintType)
enum class EPortraitActionType : uint8
{
	None UMETA(DisplayName = "None"),
	Move UMETA(DisplayName = "Move"),
	Emote UMETA(DisplayName = "Emote"),
};

UENUM(BlueprintType)
enum class EEmoteType : uint8
{
	None UMETA(DisplayName = "Default"),
	Smile UMETA(DisplayName = "Smile"),
	Sad UMETA(DisplayName = "Sad"),
	Angry UMETA(DisplayName = "Angry"),
	Surprise UMETA(DisplayName = "Surprise"),
};

UENUM(BlueprintType)
enum class EPortraitSide : uint8
{
	Left,
	Right,
	Center,
};
