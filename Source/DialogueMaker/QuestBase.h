#pragma once

#include "CoreMinimal.h"
#include "QuestBase.generated.h"

UCLASS()
class DIALOGUEMAKER_API UQuestBase : public UObject
{
	GENERATED_BODY()
public:
	void SyncQuestProgress();
	bool IsCleared() const;
	void SetClear(bool bIsNewClearValue);

private:
	void InitQuestProgress();
	
private:
	bool bIsCleared;
};
