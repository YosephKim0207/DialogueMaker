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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	EItemType ItemType;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FText Desc;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	int32 Value;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TSoftObjectPtr<UTexture2D> Icon;
	
};
