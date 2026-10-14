#pragma once

#include "CoreMinimal.h"
#include "DialogueBlockIdentity.h"
#include "DialogueDiagnostics.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/Interface.h"
#include "DialogueCheckpointContract.generated.h"

UENUM(BlueprintType)
enum class EDialogueResumeKind : uint8
{
	None,
	BlockStart,
	BeforeNode,
	BlockCompleted
};

UENUM(BlueprintType)
enum class EDialogueCheckpointReason : uint8
{
	GameSave,
	Choice,
	StateChange,
	BlockEnd,
	Replacement
};

// BeforeNode는 노드 진입 직전이다. 이미 적용한 Choice/StateChange의 후속 노드를 지정한다.
USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueResumePosition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "DialogueKit|Checkpoint")
	EDialogueResumeKind Kind = EDialogueResumeKind::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "DialogueKit|Checkpoint")
	FDialogueBlockReference Block;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "DialogueKit|Checkpoint")
	FGuid BlockRunId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "DialogueKit|Checkpoint")
	FGuid NodeGuid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "DialogueKit|Checkpoint")
	FGameplayTag TargetTag;

	// None은 활성 대화가 없는 상태이며 나머지는 정확한 Block 실행을 가리킨다.
	bool IsValid() const;
};

// 콘텐츠 버전과 독립적인 Line 열람 버전이다.
USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueLineReadRecord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "DialogueKit|Checkpoint")
	FGuid NodeGuid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "DialogueKit|Checkpoint")
	int32 ReadVersion = 1;

};

// 누적 완료와 열람을 같은 Block 기록에 둔다. 신규 후보의 영구 소진 목록이 아니다.
USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueBlockReadRecord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "DialogueKit|Checkpoint")
	FGuid BlockId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "DialogueKit|Checkpoint")
	bool bCompleted = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "DialogueKit|Checkpoint")
	TArray<FDialogueLineReadRecord> Lines;

};

// 재개 위치와 변경을 발생시킨 원인 노드는 별개다. 교체 정책은 S4에서 연결한다.
USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueCheckpointTrigger
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "DialogueKit|Checkpoint")
	EDialogueCheckpointReason Reason = EDialogueCheckpointReason::GameSave;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "DialogueKit|Checkpoint")
	FGuid OriginNodeGuid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "DialogueKit|Checkpoint")
	FGuid ChoiceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "DialogueKit|Checkpoint")
	FGuid ExecutionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "DialogueKit|Checkpoint")
	bool bHasEffects = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "DialogueKit|Checkpoint")
	FDialogueBlockReference ReplacedFromBlock;

};

// 대화 상태의 전체 값 복사다. 게임 상태/효과 ledger는 게임이 같은 스냅샷에 함께 보존한다.
USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueCheckpointSnapshot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "DialogueKit|Checkpoint")
	int32 SchemaVersion = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "DialogueKit|Checkpoint")
	FGuid CheckpointId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "DialogueKit|Checkpoint")
	FDialogueResumePosition Resume;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "DialogueKit|Checkpoint")
	FDialogueCheckpointTrigger Trigger;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "DialogueKit|Checkpoint")
	TArray<FDialogueBlockReadRecord> FullReadHistory;

	// 오류를 반환하며 입력을 보정하거나 ID를 자동 생성하지 않는다.
	bool Validate(FName& OutErrorCode) const;
};

// 같은 논리 스냅샷 재시도도 새 RequestId를 쓴다. Generation은 복원/초기화 경계를 구분한다.
USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueCheckpointRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Checkpoint")
	FGuid RequestId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Checkpoint")
	FGuid QueueGenerationId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Checkpoint")
	int64 Sequence = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Checkpoint")
	FDialogueCheckpointSnapshot Snapshot;

	// 순서와 ID를 검사한다. 실제 큐 순서/중복 여부는 게임 조율자가 관리한다.
	bool Validate(FName& OutErrorCode) const;
};

// 접수 성공은 디스크 저장 성공이 아니다. 게임은 반환 전에 게임 상태와 요청을 값 복사한다.
USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueCheckpointAcceptance
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Checkpoint")
	bool bAccepted = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Checkpoint")
	FName ErrorCode = TEXT("CheckpointProviderNotImplemented");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Checkpoint")
	FString Message;

};

// 게임의 비동기 저장 결과다. 실패는 기존 성공 저장을 바꾸지 않는다.
USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueCheckpointCompletion
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Checkpoint")
	FGuid RequestId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Checkpoint")
	FGuid QueueGenerationId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Checkpoint")
	int64 Sequence = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Checkpoint")
	bool bSaved = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Checkpoint")
	FName ErrorCode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Checkpoint")
	FString Message;

};

UINTERFACE(BlueprintType)
class DIALOGUEKIT_API UDialogueCheckpointProvider : public UInterface
{
	GENERATED_BODY()
};

class DIALOGUEKIT_API IDialogueCheckpointProvider
{
	GENERATED_BODY()
public:
	// 효과 성공 직후 후속 변경 전에 게임 상태/ledger/선택 기록/재개 위치를 함께 복사한다.
	// 큐 접수만 반환하며 실제 저장은 비동기로 수행한다. 실패 안내/재시도는 게임이 결정한다.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DialogueKit|Checkpoint")
	FDialogueCheckpointAcceptance RequestDialogueCheckpoint(const FDialogueCheckpointRequest& Request);

	// C++ 기본 구현도 실패를 반환한다.
	virtual FDialogueCheckpointAcceptance RequestDialogueCheckpoint_Implementation(const FDialogueCheckpointRequest& Request);
};

UCLASS()
class DIALOGUEKIT_API UDialogueCheckpointLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	// 접수 여부를 반환한다. 저장 완료를 기다리거나 현재 대화 커서를 변경하지 않는다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Checkpoint")
	static FDialogueCheckpointAcceptance SubmitCheckpoint(const FDialogueCheckpointRequest& Request,
		UObject* Provider, UDialogueDiagnosticRouter* Router);

	// 식별 상관관계만 검사한다. 중복/취소된 요청 제거 및 FIFO 처리는 게임의 outstanding 큐 책임이다.
	UFUNCTION(BlueprintPure, Category = "DialogueKit|Checkpoint")
	static bool MatchesCompletion(const FDialogueCheckpointRequest& PendingRequest,
		const FDialogueCheckpointCompletion& Completion);
};
