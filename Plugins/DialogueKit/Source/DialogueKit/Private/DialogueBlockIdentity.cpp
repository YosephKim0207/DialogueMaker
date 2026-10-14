#include "DialogueBlockIdentity.h"

// 실제 에셋 목록의 정확한 ID/버전 중복과 잘못된 참조를 분리한다.
bool FDialogueBlockIdentity::ValidateCatalog(const TArray<FDialogueBlockReference>& References,
    TArray<int32>& OutInvalidIndices, TArray<int32>& OutDuplicateIndices)
{
    OutInvalidIndices.Reset();
    OutDuplicateIndices.Reset();
    TSet<FDialogueBlockReference> Seen;
    for (int32 Index = 0; Index < References.Num(); ++Index)
    {
        const FDialogueBlockReference& Reference = References[Index];
        if (!Reference.IsValid())
        {
            OutInvalidIndices.Add(Index);
        }
        else if (Seen.Contains(Reference))
        {
            OutDuplicateIndices.Add(Index);
        }
        else
        {
            Seen.Add(Reference);
        }
    }
    return OutInvalidIndices.IsEmpty() && OutDuplicateIndices.IsEmpty();
}
