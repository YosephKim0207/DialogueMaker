#pragma once
#include "CoreMinimal.h"
#include "DialogueBlockRunner.h"
#include "DialogueTextPresentation.generated.h"

UENUM(BlueprintType)
enum class EDialoguePresentationInput : uint8
{
	None,
	Reveal,
	Advance,
	Select
};

// 화면의 출력 상태만 관리한다. 진행 커서·열람·효과·저장에는 접근하지 않는다.
UCLASS(BlueprintType)
class DIALOGUEKIT_API UDialogueTextPresentation : public UObject
{
	GENERATED_BODY()
public:
	// 재개/새 발화마다 처음부터 출력한다. Revision으로 이전 화면의 지연 입력을 거부한다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Presentation")
	void SetPresentation(const FDialoguePresentation& Presentation);
	// 화면의 실제 경과 시간이다. 텍스트 출력 중에만 호출하면 된다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Presentation")
	void AdvanceTime(float DeltaSeconds);
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Presentation")
	void SetVoicePlaying(bool bPlaying, int64 ExpectedRevision);
	// 입력 토큰은 한 물리 입력/프레임에서 동일한 값을 사용한다. 증가한 값만 허용한다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Presentation")
	EDialoguePresentationInput RequestAdvance(int64 ExpectedRevision, int64 InputToken);
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Presentation")
	EDialoguePresentationInput RequestChoice(FGuid ChoiceId, int64 ExpectedRevision, int64 InputToken);
	UFUNCTION(BlueprintPure, Category = "DialogueKit|Presentation")
	FText GetVisibleText() const;
	UFUNCTION(BlueprintPure, Category = "DialogueKit|Presentation")
	TArray<FDialogueChoiceView> GetVisibleChoices() const;
	UFUNCTION(BlueprintPure, Category = "DialogueKit|Presentation")
	bool IsTextComplete() const { return VisibleCharacters >= CharacterEnds.Num(); }
	UFUNCTION(BlueprintPure, Category = "DialogueKit|Presentation")
	bool IsVoicePlaying() const { return bVoicePlaying; }
	UFUNCTION(BlueprintPure, Category = "DialogueKit|Presentation")
	int64 GetRevision() const { return Revision; }
private:
	bool AcceptInput(int64 ExpectedRevision, int64 InputToken);
	EDialoguePresentationInput RevealIfNeeded();
	UPROPERTY() FDialoguePresentation Current;
	FString FullText;
	TArray<int32> CharacterEnds;
	double Duration = 0.0;
	double Elapsed = 0.0;
	int32 VisibleCharacters = 0;
	int64 Revision = 0;
	int64 LastInputToken = -1;
	bool bVoicePlaying = false;
};
