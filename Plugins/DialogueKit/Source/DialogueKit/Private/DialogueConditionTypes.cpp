#include "DialogueConditionTypes.h"

// Tag와 수치 항목의 키 유효성을 분리한다.
bool FDialogueStateKey::IsValid() const
{
	if (Kind == EDialogueStateKind::Tag)
	{
		return Tag.IsValid();
	}
	return (Kind == EDialogueStateKind::Value || Kind == EDialogueStateKind::Count) && ItemId.IsNone() == false;
}

// 표시 이름은 항목 식별에 사용하지 않는다.
bool FDialogueStateKey::operator==(const FDialogueStateKey& Other) const
{
	return Kind == Other.Kind && TargetId == Other.TargetId &&
		(Kind == EDialogueStateKind::Tag ? Tag == Other.Tag : ItemId == Other.ItemId);
}

// 동등 비교와 같은 활성 필드만 해시에 포함한다.
uint32 GetTypeHash(const FDialogueStateKey& Key)
{
	return HashCombine(HashCombine(GetTypeHash(static_cast<uint8>(Key.Kind)), GetTypeHash(Key.TargetId)),
		Key.Kind == EDialogueStateKind::Tag ? GetTypeHash(Key.Tag) : GetTypeHash(Key.ItemId));
}
