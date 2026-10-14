#pragma once

#include "CoreMinimal.h"
#include "DialogueNodeType.generated.h"

UENUM(BlueprintType)
enum class EDialogueType : uint8
{
	Unknown,
	StartNode,
	DialogueNode,
	EndNode,
	BranchNode,
	LineNode,
	ChoiceNode,
};
