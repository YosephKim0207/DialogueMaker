#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DialogueStoryDefinition.generated.h"

UENUM(BlueprintType)
enum class EDialogueStoryScope : uint8
{
	Unassigned,
	Unrestricted,
	Chapter,
	Scene
};

USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueChapterDefinition
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DialogueKit|Story")
	FGuid ChapterId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Story")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Story")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Story")
	int32 SortOrder = 0;
};

USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueSceneDefinition
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DialogueKit|Story")
	FGuid SceneId;

	// 비어 있으면 소속 없음이다. Chapter 삭제 후에도 Scene 자체는 보존한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Story")
	FGuid ChapterId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Story")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Story")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Story")
	int32 SortOrder = 0;
};

USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueStoryScope
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Story")
	EDialogueStoryScope Mode = EDialogueStoryScope::Unassigned;

	// Chapter 모드에서만 사용한다. Scene의 부모는 정의 원본에서 조회한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Story")
	FGuid ChapterId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Story")
	FGuid SceneId;
};

// 현재 진행은 게임 소유이며 정의 에셋을 수정하여 진행시키지 않는다.
USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueStoryProgress
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "DialogueKit|Story")
	FGuid ChapterId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "DialogueKit|Story")
	FGuid SceneId;
};

USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueStoryScopeResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Story")
	bool bValid = false;

	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Story")
	bool bMatches = false;

	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Story")
	FName ErrorCode = TEXT("InvalidStoryScope");
};

// 게임 콘텐츠에 보관하는 저작 정의다. Scene은 Chapter와 별도 배열로 보존한다.
UCLASS(BlueprintType)
class DIALOGUEKIT_API UDialogueStoryDefinition : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Story")
	TArray<FDialogueChapterDefinition> Chapters;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Story")
	TArray<FDialogueSceneDefinition> Scenes;

	// 저작 도구에서 명시적으로 호출한다. 이름 변경/로드/저장으로 ID를 다시 만들지 않는다.
	FGuid AddChapter(const FText& DisplayName);

	// 존재하는 Chapter 또는 빈 ID(소속 없음)에 새 Scene을 추가한다. 잘못된 부모는 실패한다.
	FGuid AddScene(const FGuid& ChapterId, const FText& DisplayName);

	// 순수 검사다. 누락/중복 ID나 사라진 부모를 자동 수정하지 않는다.
	bool ValidateDefinitions(TArray<FString>& OutIssues) const;

	// 중복 또는 누락이면 nullptr다. 반환 포인터는 배열 변경 전까지만 유효하다.
	const FDialogueChapterDefinition* FindChapter(const FGuid& ChapterId) const;
	const FDialogueSceneDefinition* FindScene(const FGuid& SceneId) const;
};

UCLASS()
class DIALOGUEKIT_API UDialogueStoryLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	// 신규 시작/Refresh용 소속 검사다. 활성 저장 복원에 적용하지 않으며 상태를 변경하지 않는다.
	// 다른 진입 조건/태그/우선순위는 이 함수의 책임이 아니다.
	UFUNCTION(BlueprintPure, Category = "DialogueKit|Story")
	static FDialogueStoryScopeResult EvaluateScope(const UDialogueStoryDefinition* Definition,
		const FDialogueStoryScope& Scope, const FDialogueStoryProgress& Progress);
};
