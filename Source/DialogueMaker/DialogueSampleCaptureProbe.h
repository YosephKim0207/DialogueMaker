#pragma once
#include "CoreMinimal.h"
#include "DialogueSampleCheckpointProvider.h"
#include "DialogueSampleCaptureProbe.generated.h"

// 샘플 호스트의 접수 재시도를 검사하는 비디스크 제공자다. 실제 게임에는 등록하지 않는다.
UCLASS()
class UDialogueSampleCaptureProbe : public UDialogueSampleCheckpointProvider
{
	GENERATED_BODY()
public:
#if WITH_DEV_AUTOMATION_TESTS
	bool bRejectNext = true;
	virtual bool CanCaptureDialogueCheckpoint_Implementation() const override { return true; }
	virtual FDialogueCheckpointAcceptance CaptureDialogueCheckpoint_Implementation(const FDialogueCheckpointSnapshot& Snapshot) override
	{
		if (bRejectNext)
		{
			bRejectNext = false;
			FDialogueCheckpointAcceptance Rejected;
			Rejected.ErrorCode = TEXT("SampleCaptureRejected");
			return Rejected;
		}
		FDialogueCheckpointRequest Request;
		Request.RequestId = FGuid::NewGuid();
		Request.QueueGenerationId = FGuid::NewGuid();
		Request.Sequence = 1;
		Request.Snapshot = Snapshot;
		return RequestDialogueCheckpoint_Implementation(Request);
	}
#endif
};
