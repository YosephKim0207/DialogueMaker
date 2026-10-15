#include "DialogueBlockLoader.h"
#include "DialogueSettings.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"

// 프로젝트 설정을 기본값으로 사용하며 게임은 로더별 soft 참조를 명시적으로 바꿀 수 있다.
UDialogueBlockLoader::UDialogueBlockLoader()
{
	CommonGreetingBlock = GetDefault<UDialogueSettings>()->CommonGreetingBlock;
}

// 이전 콜백을 먼저 무효화한 뒤 자신이 소유한 핸들만 취소한다.
void UDialogueBlockLoader::InvalidateCurrentRequest()
{
	Generation = FGuid::NewGuid();
	bPending = false;
	if (Selector)
	{
		Selector->CancelSelection();
	}
	if (Handle.IsValid())
	{
		Handle->CancelHandle();
		Handle.Reset();
	}
}

// 완료된 로드와 그 노출/완료 기록은 유지하고 아직 대기 중인 요청만 취소한다.
void UDialogueBlockLoader::CancelPending()
{
	if (bPending)
	{
		InvalidateCurrentRequest();
	}
}

// 소유자 파괴 후 콜백은 weak lambda와 세대로 차단한다. 공유 에셋을 전역 unload하지 않는다.
void UDialogueBlockLoader::BeginDestroy()
{
	InvalidateCurrentRequest();
	Super::BeginDestroy();
}

// 같은 대상의 Refresh는 Selector 세션을 유지한다.
FGuid UDialogueBlockLoader::BeginRequest(UDialogueDiagnosticRouter* Router)
{
	InvalidateCurrentRequest();
	DiagnosticRouter = Router;
	LastResult = FDialogueBlockLoadResult();
	LastResult.RequestId = Generation;
	Selected = FDialogueBlockCandidate();
	bGreeting = false;
	bPending = true;
	if (Selector == nullptr)
	{
		Selector = NewObject<UDialogueCandidateSelector>(this);
	}
	return Generation;
}

// Registry 색인으로 대상 후보만 모으며 Graph와 초상화/음향은 아직 로드하지 않는다.
FGuid UDialogueBlockLoader::RequestFromRegistry(FGameplayTag Target, UDialogueStoryDefinition* Story, FDialogueStoryProgress Progress, UObject* Provider, UDialogueDiagnosticRouter* Router)
{
	const FGuid RequestId = BeginRequest(Router);
	if (Target.IsValid() == false)
	{
		FailRequest(RequestId, TEXT("InvalidDialogueTarget"));
		return RequestId;
	}
	FARFilter Filter;
	Filter.ClassPaths.Add(UDialogueGraph::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;
	Filter.TagsAndValues.Add(FDialogueBlockMetadata::TargetRegistryKey(), Target.ToString());
	TArray<FAssetData> Assets;
	FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get().GetAssets(Filter, Assets);
	TArray<FDialogueBlockCandidate> Candidates;
	for (const FAssetData& Asset : Assets)
	{
		FDialogueBlockCandidate Candidate;
		if (FDialogueBlockMetadata::ReadAsset(Asset, Candidate.Metadata))
		{
			Candidate.AssetPath = Asset.GetSoftObjectPath();
			Candidates.Add(Candidate);
		}
		else
		{
			FDialogueDiagnostic Diagnostic;
			Diagnostic.Code = TEXT("InvalidBlockMetadata");
			Diagnostic.Message = Asset.GetSoftObjectPath().ToString();
			Diagnostic.Handling = TEXT("ExcludeAndRequireAuthorCorrection");
			UDialogueDiagnosticRouter::Dispatch(Diagnostic, Router);
			if (RequestId != Generation || bPending == false)
			{
				return RequestId;
			}
		}
	}
	SelectAndLoad(RequestId, Candidates, Target, Story, Progress, Provider);
	return RequestId;
}

// 외부 카탈로그 역시 같은 선정/수명 규칙을 사용한다.
FGuid UDialogueBlockLoader::RequestCandidates(const TArray<FDialogueBlockCandidate>& Candidates, FGameplayTag Target,
	UDialogueStoryDefinition* Story, const FDialogueStoryProgress& Progress, UObject* Provider, UDialogueDiagnosticRouter* Router)
{
	const FGuid RequestId = BeginRequest(Router);
	SelectAndLoad(RequestId, Candidates, Target, Story, Progress, Provider);
	return RequestId;
}

// 요청 시점 조건으로 선택한 결과를 복사한다. 로드 이후 조건을 다시 평가하지 않는다.
void UDialogueBlockLoader::SelectAndLoad(FGuid RequestId, const TArray<FDialogueBlockCandidate>& Candidates, FGameplayTag Target,
	UDialogueStoryDefinition* Story, const FDialogueStoryProgress& Progress, UObject* Provider)
{
	if (RequestId != Generation || bPending == false)
	{
		return;
	}
	if (Selector->BeginSession(Target) == false)
	{
		FailRequest(RequestId, TEXT("InvalidDialogueTarget"));
		return;
	}
	const FDialogueSelectionResult Selection = Selector->Select(Candidates, Story, Progress, Provider, DiagnosticRouter);
	// 진단 구독자가 새 요청을 보냈을 수도 있으므로 선택 평가 후에도 세대를 확인한다.
	if (RequestId != Generation || bPending == false)
	{
		return;
	}
	if (Selection.bSelected)
	{
		Selected = Selection.Candidate;
		StartLoad(RequestId, Selected.AssetPath);
		return;
	}
	if (Selection.ErrorCode != TEXT("NoEligibleDialogueBlock"))
	{
		FailRequest(RequestId, Selection.ErrorCode);
		return;
	}
	FDialogueDiagnostic Diagnostic;
	Diagnostic.Code = TEXT("NoEligibleDialogueBlock");
	Diagnostic.Message = TEXT("실행 가능한 일반/기본 Block이 없습니다.");
	Diagnostic.Handling = TEXT("TryConfiguredCommonGreeting");
	// 게임 개발자는 이 지점의 진단을 제작사 오류 전송으로 연결한다.
	UDialogueDiagnosticRouter::Dispatch(Diagnostic, DiagnosticRouter);
	if (RequestId != Generation || bPending == false)
	{
		return;
	}
	if (CommonGreetingBlock.IsNull())
	{
		FailRequest(RequestId, TEXT("MissingCommonGreetingBlock"));
		return;
	}
	bGreeting = true;
	StartLoad(RequestId, CommonGreetingBlock.ToSoftObjectPath());
}

// 즉시 완료도 요청 상태와 핸들 소유권을 먼저 설정한 뒤 처리한다.
void UDialogueBlockLoader::StartLoad(FGuid RequestId, const FSoftObjectPath& Path)
{
	ExpectedPath = Path;
	if (LoadOverride.IsBound())
	{
		if (LoadOverride.Execute(RequestId, Path) == false)
		{
			FailRequest(RequestId, TEXT("BlockLoadRejected"));
		}
		return;
	}
	Handle = UAssetManager::GetStreamableManager().RequestAsyncLoad(Path,
		FStreamableDelegate::CreateWeakLambda(this, [this, RequestId, Path]()
		{
			CompleteRequest(RequestId, Cast<UDialogueGraph>(Path.ResolveObject()));
		}), FStreamableManager::DefaultAsyncLoadPriority, false, true, TEXT("DialogueBlock"));
	if (Handle.IsValid() == false)
	{
		FailRequest(RequestId, TEXT("BlockLoadFailed"));
		return;
	}
	Handle->StartStalledHandle();
}

// 이전 요청/취소된 요청은 UI·진단·현재 자료를 바꾸지 않는다.
bool UDialogueBlockLoader::CompleteRequest(FGuid RequestId, UDialogueGraph* Block)
{
	if (RequestId != Generation || bPending == false)
	{
		return false;
	}
	if (IsValid(Block) == false || FSoftObjectPath(Block) != ExpectedPath || Block->GetBlockReference().IsValid() == false ||
		(bGreeting ? Block->Availability != EDialogueBlockAvailability::Active : (Block->GetBlockReference() == Selected.Metadata.Block) == false))
	{
		FailRequest(RequestId, TEXT("InvalidLoadedBlock"));
		return false;
	}
	bPending = false;
	LastResult.RequestId = RequestId;
	LastResult.Block = Block;
	LastResult.bLoaded = true;
	Handle.Reset();
	Publish();
	return true;
}

// 실패도 최신 요청만 게시한다. 이전 성공 결과를 대신 보여주지 않는다.
void UDialogueBlockLoader::FailRequest(FGuid RequestId, FName ErrorCode)
{
	if (RequestId != Generation || bPending == false)
	{
		return;
	}
	bPending = false;
	Handle.Reset();
	LastResult.RequestId = RequestId;
	LastResult.bLoaded = false;
	LastResult.Block = nullptr;
	LastResult.ErrorCode = ErrorCode;
	FDialogueDiagnostic Diagnostic;
	Diagnostic.Code = ErrorCode;
	Diagnostic.Context.Block = Selected.Metadata.Block;
	Diagnostic.Message = TEXT("최신 Block 로드 요청이 실패했습니다.");
	Diagnostic.Handling = TEXT("ReturnFailureToGame");
	UDialogueDiagnosticRouter::Dispatch(Diagnostic, DiagnosticRouter);
	if (RequestId == Generation)
	{
		Publish();
	}
}

// 콜백 재진입으로 LastResult가 바뀌어도 현재 이벤트의 값을 보존한다.
void UDialogueBlockLoader::Publish()
{
	const FDialogueBlockLoadResult Result = LastResult;
	OnLoadCompletedNative.Broadcast(Result);
	if (Result.RequestId == Generation)
	{
		OnLoadCompleted.Broadcast(Result);
	}
}

// 게임이 명시적으로 대화 세션을 종료할 때 후보 임시 목록도 초기화한다.
void UDialogueBlockLoader::EndSession()
{
	InvalidateCurrentRequest();
	if (Selector)
	{
		Selector->EndSession();
	}
	LastResult = FDialogueBlockLoadResult();
}

// 공통 인사와 실패/취소는 일반 후보 소진으로 기록하지 않는다.
bool UDialogueBlockLoader::RecordBlockCompleted(FGuid RequestId, FDialogueBlockReference Block)
{
	return RequestId == Generation && RequestId == LastResult.RequestId && Selector && bGreeting == false && LastResult.bLoaded && LastResult.Block &&
		Block == LastResult.Block->GetBlockReference() && Selector->RecordCompleted(Block);
}

// 로드 취소나 실행 실패는 직전 노출로 처리하지 않는다.
bool UDialogueBlockLoader::RecordBlockExposed(FGuid RequestId, FDialogueBlockReference Block)
{
	return RequestId == Generation && RequestId == LastResult.RequestId && Selector && bGreeting == false && LastResult.bLoaded && LastResult.Block &&
		Block == LastResult.Block->GetBlockReference() && Selector->RecordExposed(Block);
}
