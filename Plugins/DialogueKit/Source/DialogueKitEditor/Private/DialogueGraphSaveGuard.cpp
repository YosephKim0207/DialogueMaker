#include "DialogueGraphValidation.h"
#include "DialoguePreviewPolicy.h"
#include "DialogueGraph.h"
#include "DialogueChoiceNodeInfo.h"
#include "DialogueEdGraphSchema.h"
#include "UObject/Package.h"
#include "UObject/UObjectIterator.h"
#include "Misc/CoreDelegates.h"
#include "Editor.h"

namespace DialogueSaveGuard
{
	TMap<TWeakObjectPtr<UDialogueGraph>, TWeakObjectPtr<UEdGraph>> WorkingGraphs;
	FCoreUObjectDelegates::FIsPackageOKToSaveDelegate Previous;
	FDelegateHandle InstalledHandle;
	FDelegateHandle EngineInitHandle;

	// 엔진의 기존 저장 권한 검사를 보존하고 Dialogue 검사를 추가한다.
	void Install()
	{
		if (InstalledHandle.IsValid()) return;
		Previous = FCoreUObjectDelegates::IsPackageOKToSaveDelegate;
		FCoreUObjectDelegates::IsPackageOKToSaveDelegate.BindLambda([](UPackage* Package, const FString& Filename, FOutputDevice* Error)
		{
			if (Previous.IsBound() && Previous.Execute(Package, Filename, Error) == false) return false;
			return FDialogueGraphValidation::CanSavePackage(Package);
		});
		InstalledHandle = FCoreUObjectDelegates::IsPackageOKToSaveDelegate.GetHandle();
	}
}

// 열린 작업 그래프를 패키지 저장 전에 조회할 수 있게 등록한다.
void FDialogueGraphValidation::RegisterWorkingGraph(UDialogueGraph* Asset, UEdGraph* Graph)
{
	DialogueSaveGuard::Install();
	DialogueSaveGuard::WorkingGraphs.Add(Asset, Graph);
}

// 에디터를 닫으면 약한 참조 등록을 제거한다.
void FDialogueGraphValidation::UnregisterWorkingGraph(UDialogueGraph* Asset)
{
	DialogueSaveGuard::WorkingGraphs.Remove(Asset);
}

// 엔진 기본 delegate 설치 후 체인을 연결한다.
void FDialogueGraphValidation::Startup()
{
	DialogueSaveGuard::EngineInitHandle = FCoreDelegates::OnPostEngineInit.AddStatic(&DialogueSaveGuard::Install);
}

// 자신이 마지막 바인딩일 때만 기존 delegate를 복원한다.
void FDialogueGraphValidation::Shutdown()
{
	FCoreDelegates::OnPostEngineInit.Remove(DialogueSaveGuard::EngineInitHandle);
	if (FCoreUObjectDelegates::IsPackageOKToSaveDelegate.GetHandle() == DialogueSaveGuard::InstalledHandle)
	{
		FCoreUObjectDelegates::IsPackageOKToSaveDelegate = DialogueSaveGuard::Previous;
	}
	DialogueSaveGuard::WorkingGraphs.Reset();
	DialogueSaveGuard::InstalledHandle.Reset();
}

// Save All/AutoSave도 빈 Choice 등 차단 오류를 기록한 채 실제 패키지 저장을 거부한다.
// 저장 호출 내부에서는 모달을 열지 않는다. 상세 목록은 Graph의 수동 검사에서 확인한다.
bool FDialogueGraphValidation::CanSavePackage(UPackage* Package)
{
	for (TObjectIterator<UDialogueGraph> It; It; ++It)
	{
		UDialogueGraph* Asset = *It;
		if (Asset->GetOutermost() != Package || Asset->HasAnyFlags(RF_ClassDefaultObject)) continue;
		if (UDialoguePreviewPolicy::IsBlockPreviewActive()) return false;
		UEdGraph* Working = DialogueSaveGuard::WorkingGraphs.FindRef(Asset).Get();
		if (Working == nullptr && Asset->Graph == nullptr) continue;
		if (Working == nullptr)
		{
			// 닫힌 에셋은 저장 권한 검사에서 UObject를 생성하지 않고 기존 런타임을 검사한다.
			TArray<FString> IdentifierIssues;
			if (Asset->Graph->ValidateIdentifiers(IdentifierIssues) == false) return false;
			int32 StartCount = 0;
			for (const UDialogueRuntimeNode* Node : Asset->Graph->Nodes)
			{
				if (Node->DialogueNodeType == EDialogueType::StartNode) ++StartCount;
				if (Node->DialogueNodeType == EDialogueType::ChoiceNode)
				{
					const auto* Choice = Cast<UDialogueChoiceNodeInfo>(Node->NodeInfo);
					if (Choice == nullptr || Choice->Options.IsEmpty()) return false;
				}
			}
			if (StartCount != 1) return false;
			continue;
		}
		const TArray<FDialogueGraphIssue> Issues = Inspect(Working);
		if (CanSave(Issues) == false)
		{
			UE_LOG(LogTemp, Warning, TEXT("DialogueMaker: 저장 차단 %s. 그래프 검사에서 빈 Choice/구조 오류를 확인하세요."), *Asset->GetPathName());
			return false;
		}
	}
	return true;
}
