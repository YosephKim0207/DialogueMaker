#pragma once

#include "CoreMinimal.h"
#include "DialogueBlockIdentity.h"
#include "DialogueDiagnostics.generated.h"

UENUM(BlueprintType)
enum class EDialogueDiagnosticSeverity : uint8
{
	Info,
	Warning,
	Error
};

USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueDiagnosticContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Diagnostics")
	FDialogueBlockReference Block;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Diagnostics")
	FGuid NodeGuid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Diagnostics")
	FGuid ExecutionId;

	// 동일 Block의 반복 실행을 구분한다. 구 저장은 빈 값으로 유지한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Diagnostics")
	FGuid BlockRunId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Diagnostics")
	FGuid SaveRequestId;
};

USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueDiagnostic
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Diagnostics")
	FName Code;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Diagnostics")
	EDialogueDiagnosticSeverity Severity = EDialogueDiagnosticSeverity::Warning;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Diagnostics")
	FString Message;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Diagnostics")
	FString Handling;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Diagnostics")
	FDialogueDiagnosticContext Context;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Diagnostics")
	FString GameName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DialogueKit|Diagnostics")
	FName Component = TEXT("DialogueMaker");
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogueDiagnostic, const FDialogueDiagnostic&, Diagnostic);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnDialogueDiagnosticNative, const FDialogueDiagnostic&);

// 게임이 소유/구독하는 진단 경계다. SDK/DSN/외부 전송 정책을 플러그인에 넣지 않는다.
UCLASS(BlueprintType)
class DIALOGUEKIT_API UDialogueDiagnosticRouter : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "DialogueKit|Diagnostics")
	FOnDialogueDiagnostic OnDiagnostic;

	FOnDialogueDiagnosticNative OnDiagnosticNative;

	// 동일 진단을 로컬 로그와 구독자에게 전달한다. 게임 스레드에서만 호출하고 Code는 비어 있지 않은 안정 코드를 지정한다.
	// 개발자는 이 이벤트에 게임 Sentry 등의 로그 전송 코드를 연결한다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Diagnostics")
	FDialogueDiagnostic ReportDiagnostic(FDialogueDiagnostic Diagnostic);

	// Router가 없어도 로컬 로그를 유지하며 임의 외부 전송을 하지 않는다.
	static FDialogueDiagnostic Dispatch(FDialogueDiagnostic Diagnostic, UDialogueDiagnosticRouter* Router);
};
