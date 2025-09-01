#pragma once

#include "CoreMinimal.h"
#include "QuestBase.generated.h"

UCLASS()
class DIALOGUEMAKER_API UQuestBase : public UObject
{
	GENERATED_BODY()
public:
	void SyncQuestProgress();
	void StartQuest();
	bool IsCleared() const;
	void SetClear();
	FName GetQuestID() const;
	
private:
	void InitQuestProgress();
	
private:
	bool bIsCleared;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest", meta=(AllowPrivateAccess=true))
	FName QuestID;
};
