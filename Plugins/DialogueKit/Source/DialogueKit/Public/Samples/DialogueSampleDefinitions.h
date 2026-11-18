#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "DialogueStateProvider.h"
#include "DialogueSampleStateProvider.h"
#include "DialogueSampleDefinitions.generated.h"

// 게임 소유 NPC 원본의 참고 형식이다. 실제 게임에 이 클래스를 강제하지 않는다.
UCLASS(BlueprintType)
class DIALOGUEKIT_API UDialogueSampleNpcDefinition : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Sample|NPC", meta = (ToolTip = "게임 NPC의 안정 ID 예시입니다. 에셋 이름이나 표시 이름을 바꾸어도 이 ID는 유지합니다."))
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Sample|NPC", meta = (ToolTip = "제작자에게 보여줄 NPC 이름입니다. 상태 조회 식별자는 Id입니다."))
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Sample|NPC", meta = (MultiLine = true, ToolTip = "게임 개발자를 위한 샘플 설명입니다. 현재 호감도나 진행 상태를 저장하는 필드가 아닙니다."))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Sample|NPC", meta = (ToolTip = "게임에서 사용할 초상화의 선택적 참조 예시입니다. 정의 목록 변환은 이미지를 로드하거나 Dialogue Line에 자동 적용하지 않습니다."))
	TSoftObjectPtr<UTexture2D> Portrait;
};

// 다수 아이템을 표로 관리하는 게임 원본 예시다. RowName은 편집 위치이고 Id가 상태 키다.
USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueSampleItemRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Sample|Item", meta = (ToolTip = "아이템의 안정 ID 예시입니다. DataTable 행 이름과 별도로 이 값을 조건의 Item ID로 사용합니다."))
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Sample|Item", meta = (ToolTip = "표시할 아이템 이름입니다. 이름 변경만으로 상태 키를 바꾸지 않습니다."))
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Sample|Item", meta = (MultiLine = true, ToolTip = "아이템 설명 예시입니다. 실제 아이템 효과와 게임 정책은 게임이 구현합니다."))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Sample|Item", meta = (ClampMin = "1", ToolTip = "게임 정의의 최대 중첩 수 예시입니다. 현재 보유량이 아니며 샘플 상태 제공자가 이 한도를 자동 적용하지 않습니다."))
	int32 MaxStack = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Sample|Item", meta = (ToolTip = "선택적 아이콘 참조 예시입니다. 목록 변환은 이미지를 로드하지 않습니다."))
	TSoftObjectPtr<UTexture2D> Icon;
};

// 스킬 기획/실행을 구현하지 않고 공통 수치와 현재 레벨의 분리만 보여 주는 표다.
USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueSampleSkillRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Sample|Skill", meta = (ToolTip = "스킬의 안정 ID 예시입니다. 샘플에서는 Value 종류의 Item ID로 사용하여 현재 레벨을 조회합니다."))
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Sample|Skill", meta = (ToolTip = "표시할 스킬 이름입니다. 스킬 실행이나 습득 여부를 결정하지 않습니다."))
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Sample|Skill", meta = (MultiLine = true, ToolTip = "게임 개발자를 위한 설명입니다. 스킬 동작·성장·습득 정책은 게임 쪽에서 결정합니다."))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Sample|Skill", meta = (ClampMin = "1", ToolTip = "최대 레벨의 예시 정의입니다. 현재 스킬 레벨이 아니며 샘플 상태 제공자가 범위를 자동 제한하지 않습니다."))
	int32 MaxLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Sample|Skill", meta = (ClampMin = "0", Units = "s", ToolTip = "기본 재사용 대기 시간의 예시 수치입니다. 현재 남은 시간이나 스킬 실행을 구현하지 않습니다."))
	float CooldownSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Sample|Skill", meta = (ToolTip = "선택적 스킬 아이콘입니다. 정의 변환 시 이미지를 로드하지 않습니다."))
	TSoftObjectPtr<UTexture2D> Icon;
};

// 게임의 서로 다른 원본을 대화 공통 목록으로 변환하는 교체 가능한 참고 구현이다.
UCLASS(BlueprintType)
class DIALOGUEKIT_API UDialogueSampleDefinitionCatalog : public UDataAsset, public IDialogueDefinitionProvider
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Sample|Sources", meta = (ToolTip = "포함할 NPC 정의 에셋입니다. 명시적으로 연결한 원본만 읽으며 프로젝트 전체 NPC를 자동 수집하지 않습니다."))
	TArray<TSoftObjectPtr<UDialogueSampleNpcDefinition>> Npcs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Sample|Sources", meta = (RequiredAssetDataTags = "RowStructure=/Script/DialogueKit.DialogueSampleItemRow", ToolTip = "FDialogueSampleItemRow 형식의 필수 샘플 테이블입니다. 행을 추가하면 다음 목록 조회에 반영되며 별도 ID 목록을 작성하지 않습니다."))
	TSoftObjectPtr<UDataTable> Items;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Sample|Sources", meta = (RequiredAssetDataTags = "RowStructure=/Script/DialogueKit.DialogueSampleSkillRow", ToolTip = "FDialogueSampleSkillRow 형식의 필수 샘플 테이블입니다. 실제 게임의 다른 행 형식은 게임 제공자에서 변환합니다."))
	TSoftObjectPtr<UDataTable> Skills;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Sample|Mapping", meta = (ToolTip = "아이템 보유량과 스킬 레벨을 조회할 대상의 예시 ID입니다. Player는 예약어가 아니며 자동 Actor 검색을 하지 않습니다."))
	FName PlayerTargetId = TEXT("Player");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Sample|Mapping", meta = (ToolTip = "각 NPC의 Value 항목으로 노출할 호감도 키 예시입니다. 실제 게임은 자신의 상태 규약에 맞게 연결합니다."))
	FName NpcAffectionItemId = TEXT("Affection");

	// 원본만 동기 로드하여 목록을 만든다. 실패는 빈 출력과 원인 문자열이며 상태/에셋을 변경하지 않는다.
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "DialogueKit|Sample")
	bool BuildDefinitions(TArray<FDialogueStateDefinition>& OutDefinitions, FString& OutError) const;

	// 같은 변환을 기존 공통 정의 인터페이스로 제공한다.
	virtual bool GetDialogueDefinitions_Implementation(TArray<FDialogueStateDefinition>& OutDefinitions) const override;

	// 정의만 등록한 새 메모리 제공자를 만든다. 호출자는 반환 객체를 UPROPERTY로 보관해야 한다.
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "DialogueKit|Sample")
	UDialogueSampleStateProvider* CreateMemoryProvider(UObject* Outer, FString& OutError) const;
};
