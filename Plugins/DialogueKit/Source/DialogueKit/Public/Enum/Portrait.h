#pragma once

#include "Portrait.generated.h"

UENUM(BlueprintType)
enum class EPortraitActionType : uint8
{
	None = 0 UMETA(DisplayName = "None"),
	Move = 10 UMETA(DisplayName = "Move"),
	Flip = 11 UMETA(DisplayName = "Flip"),
	Emote = 100 UMETA(DisplayName = "Emote"),
};

UENUM(BlueprintType)
enum class EPortraitFlipDirection : uint8
{
	Left = 0 UMETA(DisplayName = "Left"),
	Right = 1 UMETA(DisplayName = "Right"),
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
