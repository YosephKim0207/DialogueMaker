#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueDiagnostics.h"
#include "Misc/App.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueDiagnosticsTest,
	"DialogueKit.Contracts.Diagnostics.Dispatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 구독/해제와 게임 이름 기본값, 진단 위치/결과의 보존을 검사한다.
bool FDialogueDiagnosticsTest::RunTest(const FString& Parameters)
{
	UDialogueDiagnosticRouter* Router = NewObject<UDialogueDiagnosticRouter>();
	FDialogueDiagnostic Input;
	Input.Code = TEXT("SampleDiagnostic");
	Input.Message = TEXT("Test diagnostic");
	Input.Handling = TEXT("ContinueWithTrue");
	Input.Context.NodeGuid = FGuid::NewGuid();
	Input.Context.ExecutionId = FGuid::NewGuid();
	Input.Context.SaveRequestId = FGuid::NewGuid();
	int32 Received = 0;
	FDialogueDiagnostic Captured;
	const FDelegateHandle Handle = Router->OnDiagnosticNative.AddLambda([&](const FDialogueDiagnostic& Diagnostic)
	{
		++Received;
		Captured = Diagnostic;
	});
	const FDialogueDiagnostic Result = Router->ReportDiagnostic(Input);
	TestEqual(TEXT("One event"), Received, 1);
	TestEqual(TEXT("Project name default"), Result.GameName, FString(FApp::GetProjectName()));
	TestEqual(TEXT("Same node delivered"), Captured.Context.NodeGuid, Input.Context.NodeGuid);
	TestEqual(TEXT("Same save request delivered"), Captured.Context.SaveRequestId, Input.Context.SaveRequestId);
	TestEqual(TEXT("Same handling delivered"), Captured.Handling, Input.Handling);
	Router->OnDiagnosticNative.Remove(Handle);
	Input.GameName = TEXT("CustomGame");
	TestEqual(TEXT("Game override"), Router->ReportDiagnostic(Input).GameName, Input.GameName);
	TestEqual(TEXT("Unsubscribed"), Received, 1);
	TArray<uint8> Bytes;
	FMemoryWriter Writer(Bytes);
	FDialogueDiagnostic::StaticStruct()->SerializeItem(Writer, &Captured, nullptr);
	FDialogueDiagnostic Loaded;
	FMemoryReader Reader(Bytes);
	FDialogueDiagnostic::StaticStruct()->SerializeItem(Reader, &Loaded, nullptr);
	TestEqual(TEXT("Diagnostic archive context"), Loaded.Context.ExecutionId, Input.Context.ExecutionId);
	TestEqual(TEXT("No-router local path"), UDialogueDiagnosticRouter::Dispatch(Input, nullptr).Code, Input.Code);
	return true;
}
#endif
