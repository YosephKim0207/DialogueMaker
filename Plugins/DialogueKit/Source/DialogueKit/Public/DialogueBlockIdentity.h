#pragma once

#include "CoreMinimal.h"
#include "DialogueBlockIdentity.generated.h"

// 에셋 이름/경로와 독립적인 Block 실행 콘텐츠의 정확한 버전 참조다.
USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueBlockReference
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "DialogueKit|Identity")
    FGuid BlockId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "DialogueKit|Identity", meta = (ClampMin = "1"))
    int32 ContentVersion = 1;

    // 누락된 ID나 유효하지 않은 버전을 복원 가능한 참조로 취급하지 않는다.
    bool IsValid() const { return BlockId.IsValid() && ContentVersion > 0; }

    // 같은 Block의 다른 실행 버전은 서로 다른 참조다.
    bool operator==(const FDialogueBlockReference& Other) const
    {
        return BlockId == Other.BlockId && ContentVersion == Other.ContentVersion;
    }

    // 저장 카탈로그 및 조회 맵에서 ID와 버전을 함께 키로 사용한다.
    friend uint32 GetTypeHash(const FDialogueBlockReference& Reference)
    {
        return HashCombine(GetTypeHash(Reference.BlockId), GetTypeHash(Reference.ContentVersion));
    }
};

class DIALOGUEKIT_API FDialogueBlockIdentity
{
public:
    // 영속 에셋 카탈로그의 누락/중복 참조 인덱스를 반환한다. PIE/실행 복제본은 입력하지 않는다.
    // 다른 버전은 허용하며, 중복은 첫 항목 이후의 항목만 보고한다. 입력을 수정하지 않는다.
    static bool ValidateCatalog(const TArray<FDialogueBlockReference>& References,
        TArray<int32>& OutInvalidIndices, TArray<int32>& OutDuplicateIndices);
};
