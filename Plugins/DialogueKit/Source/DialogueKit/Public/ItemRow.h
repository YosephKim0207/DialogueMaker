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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Item")
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Item")
	EItemType ItemType;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Item")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Item")
	FText Desc;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Item")
	int32 Value;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Item")
	TSoftObjectPtr<UTexture2D> Icon;
	
};
