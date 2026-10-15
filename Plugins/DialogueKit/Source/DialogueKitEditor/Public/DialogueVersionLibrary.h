#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DialogueGraph.h"
#include "DialogueVersionLibrary.generated.h"

// 출시 버전은 별도 에셋으로 보존한다. 원본의 신규 노출 중단은 작가가 명시적으로 설정한다.
UCLASS()
class DIALOGUEKITEDITOR_API UDialogueVersionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Versions")
	static UDialogueGraph* CreateContentVersion(UDialogueGraph* Source, const FString& PackageName, FName AssetName, int32 NewVersion, FName& OutError);
};
