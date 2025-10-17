#pragma once
#include "Engine/DataTable.h"
#include "ItemRow.generated.h"

UENUM(BlueprintType)
enum class EItemType : uint8
{
	Consumable,
	QuestOnly,
	Weapon,
	Armor,
};

USTRUCT(BlueprintType)
struct FItemRow : public FTableRowBase
{
	GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|FItemRow|Identity")
    FName Id;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|FItemRow|Type")
    EItemType ItemType;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|FItemRow|Display")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|FItemRow|Display")
    FText Desc;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|FItemRow|Attributes")
    int32 Value;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|FItemRow|Display")
    TSoftObjectPtr<UTexture2D> Icon;
	
};
