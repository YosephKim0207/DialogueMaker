#pragma once

#include "CoreMinimal.h"
#include "DialogueConditionTypes.h"
#include "IPropertyTypeCustomization.h"
#include "PropertyHandle.h"

class IPropertyUtilities;

struct FDialogueIdOption
{
	FName Id;
	FText Label;
};

// 에디터 정의 목록과 필터다. 게임의 현재 상태를 조회하거나 저장하지 않는다.
class DIALOGUEKITEDITOR_API FDialogueDefinitionOptions
{
public:
	// 설정된 에셋을 호출하는 동안만 보호하고 검증된 값 목록을 복사한다.
	bool LoadConfigured();
	// 인터페이스 오류 또는 잘못된 전체 정의를 원자적으로 거부한다.
	bool ReadSource(UObject* Source);
	// 복합 키 중복과 필수 필드를 검사한다.
	bool SetDefinitions(const TArray<FDialogueStateDefinition>& InDefinitions);
	// 종류별 대상 또는 종류·대상별 항목 목록을 안정 ID 순서로 반환한다.
	TArray<FDialogueIdOption> GetOptions(EDialogueStateKind Kind, FName TargetId, bool bTarget) const;
	// 원본 전체에서 활성 키가 등록되어 있는지 확인한다.
	bool Contains(const FDialogueStateKey& Key) const;
	// 이름 또는 ID의 부분 문자열을 대소문자 구분 없이 검색한다.
	static bool Matches(const FDialogueIdOption& Option, const FString& Search);

	TArray<FDialogueStateDefinition> Definitions;
	FText Error;
};

class DIALOGUEKITEDITOR_API FDialogueStateKeyCustomization : public IPropertyTypeCustomization
{
public:
	// 공통 상태 키의 Details 인스턴스를 만든다.
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> Handle, FDetailWidgetRow& Row, IPropertyTypeCustomizationUtils& Utils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> Handle, IDetailChildrenBuilder& Builder, IPropertyTypeCustomizationUtils& Utils) override;
	// 실제 선택 경로와 테스트가 같은 편집 잠금/필터/프로퍼티 알림 경로를 사용한다.
	static FPropertyAccess::Result ApplySelection(const TSharedRef<IPropertyHandle>& KeyHandle, const FDialogueDefinitionOptions& Options, bool bTarget, FName Id, bool bEditingEnabled);

private:
	// 혼합 선택은 임의 첫 객체 값으로 해석하지 않는다.
	bool GetFilter(EDialogueStateKind& Kind, FName& Target, bool bTarget) const;
	// 위젯 및 콜백에서 Details/PIE 잠금을 함께 확인한다.
	bool CanEdit() const;
	// 메뉴를 여는 시점에만 원본을 다시 읽는다.
	TSharedRef<SWidget> MakeMenu(bool bTarget);
	// 현재 저장된 ID를 등록 여부와 관계없이 표시한다.
	FText GetLabel(bool bTarget) const;
	// 캐시와 현재 키로만 경고를 만든다. Tick에서 에셋을 로드하지 않는다.
	FText GetStatus() const;
	// 대상 또는 항목의 검색 목록 선택 행을 추가한다.
	void AddIdRows(IDetailChildrenBuilder& Builder, bool bTarget);

	TSharedPtr<IPropertyHandle> Key;
	TWeakPtr<IPropertyUtilities> Utilities;
	FDialogueDefinitionOptions Options;
};
