#include "DialogueDiagnostics.h"
#include "Misc/App.h"

DEFINE_LOG_CATEGORY_STATIC(DialogueDiagnosticLog, Log, All);

// 외부 전송의 실패/정책은 대화의 반환값이나 진행을 변경하지 않는다.
FDialogueDiagnostic UDialogueDiagnosticRouter::ReportDiagnostic(FDialogueDiagnostic Diagnostic)
{
	Diagnostic = Dispatch(MoveTemp(Diagnostic), nullptr);
	OnDiagnosticNative.Broadcast(Diagnostic);
	OnDiagnostic.Broadcast(Diagnostic);
	return Diagnostic;
}

// 오류 위치와 요청 식별자를 포함한 동일 정보를 전송 연동 지점에 제공한다.
FDialogueDiagnostic UDialogueDiagnosticRouter::Dispatch(FDialogueDiagnostic Diagnostic, UDialogueDiagnosticRouter* Router)
{
	if (IsValid(Router))
	{
		return Router->ReportDiagnostic(MoveTemp(Diagnostic));
	}
	if (Diagnostic.GameName.IsEmpty())
	{
		Diagnostic.GameName = FApp::GetProjectName();
	}
	Diagnostic.Component = TEXT("DialogueMaker");
	const FString Description = FString::Printf(TEXT("UDialogueDiagnosticRouter::Dispatch : [%s] %s | %s | Game=%s Block=%s Version=%d Node=%s Execution=%s Save=%s"),
		*Diagnostic.Code.ToString(), *Diagnostic.Message, *Diagnostic.Handling, *Diagnostic.GameName,
		*Diagnostic.Context.Block.BlockId.ToString(), Diagnostic.Context.Block.ContentVersion,
		*Diagnostic.Context.NodeGuid.ToString(), *Diagnostic.Context.ExecutionId.ToString(), *Diagnostic.Context.SaveRequestId.ToString());
	switch (Diagnostic.Severity)
	{
	case EDialogueDiagnosticSeverity::Error:
		UE_LOG(DialogueDiagnosticLog, Error, TEXT("%s"), *Description);
		break;
	case EDialogueDiagnosticSeverity::Warning:
		UE_LOG(DialogueDiagnosticLog, Warning, TEXT("%s"), *Description);
		break;
	default:
		UE_LOG(DialogueDiagnosticLog, Log, TEXT("%s"), *Description);
		break;
	}
	return Diagnostic;
}
