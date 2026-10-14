#include "DialogueSampleCheckpointProvider.h"

// 호출자가 이후 원본을 수정해도 접수한 값은 변경되지 않는다.
FDialogueCheckpointAcceptance UDialogueSampleCheckpointProvider::RequestDialogueCheckpoint_Implementation(const FDialogueCheckpointRequest& Request)
{
	FDialogueCheckpointAcceptance Result;
	if (Request.Validate(Result.ErrorCode))
	{
		CapturedRequests.Add(Request);
		Result.bAccepted = true;
	}
	return Result;
}
