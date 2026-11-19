// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <functional>

#include "CoreMinimal.h"
#include "DialogueRuntimeGraph.h"
#include "DialogueEditorLayout.h"
#include "DialogueBlockIdentity.h"
#include "DialogueStoryDefinition.h"
#include "DialogueBlockMetadata.h"
#include "GameplayTagContainer.h"
#include "Enum/PlayerProgress.h"
#include "Enum/Portrait.h"
#include "Enum/DialogueGraphType.h"
#include "DialogueGraph.generated.h"

USTRUCT(BlueprintType)
struct FPortraitInitData
{
	GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|FPortraitInitData|Speaker", meta = (ToolTip = "대화 시작 시 표시할 초기 초상화의 화자입니다. 구형 대화 UI의 초기 배치에 사용합니다."))
    ESpeakerID Speaker = ESpeakerID::TestNPC;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|FPortraitInitData|Emote", meta = (ToolTip = "초기 초상화에 사용할 표정입니다. 화자와 표정에 대응하는 초상화 자료가 필요합니다."))
    EEmoteType EmoteType = EEmoteType::None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|FPortraitInitData|Portrait", meta = (ToolTip = "초기 초상화를 배치할 화면 위치입니다. Left/Center/Right 중 선택합니다."))
    EPortraitSide PortraitSide = EPortraitSide::Left;
};

UCLASS(BlueprintType)
class DIALOGUEKIT_API UDialogueGraph : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
#if WITH_EDITORONLY_DATA
    UPROPERTY()
    FDialogueEditorLayout EditorLayout;
#endif
    // 새 에셋 생성 시 누락된 ID만 초기화한다. 로드/저장에서는 자동 호출하지 않는다.
    // 이미 ID가 있거나 템플릿 객체이면 false이며 기존 ID는 변경하지 않는다.
    bool InitializeBlockIdentity();

    // 이름과 경로에 독립적인 Block 식별자를 조회한다.
    UFUNCTION(BlueprintPure, Category = "DialogueKit|Identity")
    FGuid GetBlockId() const { return BlockId; }

    // 저장 형식/열람 버전과 구분되는 실행 콘텐츠 버전을 조회한다.
    UFUNCTION(BlueprintPure, Category = "DialogueKit|Identity")
    int32 GetContentVersion() const { return ContentVersion; }

    // 정확한 콘텐츠를 지칭하는 ID/버전 쌍을 조회한다.
    UFUNCTION(BlueprintPure, Category = "DialogueKit|Identity")
    FDialogueBlockReference GetBlockReference() const;

    // 일반 저작 복제만 새 Block으로 취급하고 World/PIE 복제는 동일 콘텐츠로 유지한다.
    virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;

private:
#if WITH_EDITOR
    friend class UDialogueVersionLibrary;
    bool InitializeContentVersion(const UDialogueGraph* Source, int32 NewVersion);
#endif
    UPROPERTY(VisibleAnywhere, Category = "DialogueKit|Identity", meta = (ToolTip = "이 대화 Block의 고유 ID입니다. 에셋 이름·경로와 독립적이며 저장 복원에 사용합니다. 직접 편집하지 않습니다."))
    FGuid BlockId;

    UPROPERTY(VisibleAnywhere, Category = "DialogueKit|Identity", meta = (ToolTip = "동일 Block의 실행 콘텐츠 버전입니다. 저장 데이터 형식이나 Line 열람 버전과 다릅니다. 별도 버전 작성 기능으로 관리합니다."))
    int32 ContentVersion = 1;

public:
	// FString GetSpeakerName() const;
	void SetPreSaveListener(std::function<void()> NewOnPreSaveListener);

	virtual void PreSave(FObjectPreSaveContext SaveContext) override;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	virtual void GetAssetRegistryTags(FAssetRegistryTagsContext Context) const override;

	static FORCEINLINE FName GetSpeakerIDTag()
	{
		return GET_MEMBER_NAME_CHECKED(UDialogueGraph, SpeakerID);
	}

	static FORCEINLINE FName GetChapterIDTag()
	{
		return GET_MEMBER_NAME_CHECKED(UDialogueGraph, ChapterID);
	}
	
public:
	std::function<void()> OnPreSaveListener;

public:
	// 화자가 아니라 대화를 시작하는 대상이다. NPC/장소 등 게임이 등록한 Tag 하나를 사용한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Selection", meta = (ToolTip = "이 Block을 시작할 NPC·장소 등 대화 대상 태그입니다. Line의 화자 및 Conditions의 Target ID와는 별개입니다."))
	FGameplayTag DialogueTarget;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Selection", meta = (ToolTip = "Active는 신규 대화 후보로 사용할 수 있습니다. RestoreOnly는 신규 선정에서 제외하고 기존 저장의 정확한 버전 복원에만 사용합니다."))
	EDialogueBlockAvailability Availability = EDialogueBlockAvailability::Active;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Selection", meta = (ToolTip = "조건을 만족하는 최우선/일반 대화 후보가 모두 없을 때 사용하는 기본 대화입니다. 기본 대화끼리는 세션 순회 대상이며 Top Priority 설정은 적용하지 않습니다."))
	bool bDefaultBlock = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Selection", meta = (ToolTip = "조건을 만족하는 일반 대화 중 최우선 그룹으로 취급합니다. 기본 대화에는 적용하지 않습니다. 그룹 내 순위는 Dialogue Priority Weight를 사용합니다.", EditCondition = "!bDefaultBlock"))
	bool bTopPriorityBlock = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Selection", meta = (ToolTip = "이 Block을 신규 대화 후보로 선정할 때 평가할 조건입니다. 빈 목록은 조건 없이 통과합니다. 저장된 대화의 정확 버전 복원에는 이 진입 조건을 다시 적용하지 않습니다."))
	FDialogueConditionSet EntryConditions;
	// 원본 필드에서 Registry 자료를 생성한다. 별도 수동 카탈로그를 유지하지 않는다.
	FDialogueBlockMetadata GetSelectionMetadata() const;

public:	// Properties
	// 신규 소속 계약이다. 기존 ChapterID 소비처 전환은 후보 필터 단계에서 수행한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Story", meta = (ToolTip = "이 Block이 속한 Chapter 또는 Scene입니다. 소속 없음은 신규 후보에서 제외되며 제한 없음은 진행 범위를 제한하지 않습니다. 게임의 현재 진행 상태를 바꾸지 않습니다."))
	FDialogueStoryScope StoryScope;

	// UPROPERTY(EditAnywhere)
	// FString SpeakerName = FString("Enter Dialogue Name Here");
        UPROPERTY(VisibleAnywhere, Category = "DialogueKit|DialogueGraph|Runtime", meta = (ToolTip = "편집 그래프에서 생성되는 실행용 그래프 참조입니다. 읽기 전용이며 노드·연결은 왼쪽 그래프에서 편집합니다."))
        TObjectPtr<UDialogueRuntimeGraph> Graph;

        UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = "DialogueKit|DialogueGraph|Filter - Default", meta = (ToolTip = "구형 BeginDialogue 화자 검색용 ID입니다. 신규 Block의 대화 대상 태그나 각 Line의 화자 설정을 대신하지 않습니다."))
        ESpeakerID SpeakerID;

        UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = "DialogueKit|DialogueGraph|Filter - Default", meta = (ToolTip = "구형 대화 검색용 Chapter enum입니다. 신규 Block의 Chapter/Scene 소속은 소속 선택 메뉴에서 지정합니다."))
        EChapterID ChapterID;
	
	UPROPERTY(EditAnywhere, Category = "DialogueKit|DialogueGraph|Filter - Tags", meta = (ToolTip = "구형 검색에서 모두 보유해야 하는 태그입니다. 값이 남아 있으면 신규 Block 후보에서 제외됩니다. 신규 조건은 Entry Conditions에 옮긴 뒤 이 목록을 비웁니다."))
	FGameplayTagContainer RequiredAllTags;

	UPROPERTY(EditAnywhere, Category = "DialogueKit|DialogueGraph|Filter - Tags", meta = (ToolTip = "구형 검색에서 하나 이상 보유해야 하는 태그입니다. 값이 남아 있으면 신규 Block 후보에서 제외됩니다. 신규 조건은 Entry Conditions에 작성합니다."))
	FGameplayTagContainer RequiredAnyTags;

	UPROPERTY(EditAnywhere, Category = "DialogueKit|DialogueGraph|Filter - Tags", meta = (ToolTip = "구형 검색에서 하나라도 보유하면 제외하는 태그입니다. 값이 남아 있으면 신규 Block 후보에서 제외됩니다. 신규 조건은 Entry Conditions에 작성합니다."))
	FGameplayTagContainer BlockedAnyTags;
	
        UPROPERTY(EditAnywhere, AssetRegistrySearchable, BlueprintReadOnly, Category = "DialogueKit|Selection", meta = (DisplayName = "대화 유형", ToolTip = "퀘스트·스토리·일상 대화를 구분하는 분류입니다. 새 후보 선정의 우선순위·최우선·기본 대화 여부는 바꾸지 않습니다. 구형 BeginDialogue 경로에서는 기존 Quest→Story→DailyChat 정렬에 계속 사용됩니다."))
        EDialogueGraphType DialogueGraphType;

        UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = "DialogueKit|DialogueGraph|Filter - Order", meta = (ToolTip = "대화 선정 우선순위 값입니다. 신규 최우선/일반 그룹에서는 큰 값이 먼저이며 동률은 균등 선정합니다. 기본 대화 순회에는 이 값을 사용하지 않습니다."))
        int32 DialoguePriorityWeight;

        UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = "DialogueKit|DialogueGraph|Initial Portrait", meta = (ToolTip = "구형 대화 UI에서 시작 시 배치할 화자·표정·위치 목록입니다. 신규 Block 기본 화면은 각 Line의 초상화 설정을 사용합니다."))
        TArray<FPortraitInitData> InitPortraits;
};

