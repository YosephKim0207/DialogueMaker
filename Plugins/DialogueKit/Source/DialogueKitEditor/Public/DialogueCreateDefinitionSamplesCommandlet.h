#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "DialogueCreateDefinitionSamplesCommandlet.generated.h"

// 배포할 개발자 참고 에셋을 명시적 실행으로만 생성한다. 기존 에셋은 덮어쓰지 않는다.
UCLASS()
class UDialogueCreateDefinitionSamplesCommandlet : public UCommandlet
{
	GENERATED_BODY()
public:
	UDialogueCreateDefinitionSamplesCommandlet();
	virtual int32 Main(const FString& Params) override;
};
