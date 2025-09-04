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

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EItemType ItemType;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Desc;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Value;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> Icon;
	
};
