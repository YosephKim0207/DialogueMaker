#pragma once

#include "CoreMinimal.h"
#include "DialogueNodeType.generated.h"

UENUM()
enum class EDialogueType
{
	Unknown,
	StartNode,
	DialogueNode,
	EndNode,
	BranchNode,
};