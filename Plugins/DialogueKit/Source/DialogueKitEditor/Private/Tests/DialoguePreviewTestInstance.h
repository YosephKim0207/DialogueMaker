#pragma once
#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "DialoguePreviewTestInstance.generated.h"

// 테스트에서 파일을 열지 않고 Subsystem의 PIE 월드 문맥만 구성한다.
UCLASS()
class UDialoguePreviewTestInstance : public UGameInstance
{
    GENERATED_BODY()
public:
    void SetTestContext(FWorldContext* Context) { WorldContext = Context; }
};
