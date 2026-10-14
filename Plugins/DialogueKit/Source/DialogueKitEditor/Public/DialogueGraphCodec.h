#pragma once

#include "CoreMinimal.h"

class UDialogueGraph;
class UEdGraph;

// 편집기와 자동화가 동일한 영속 변환 경로를 사용한다.
class DIALOGUEKITEDITOR_API FDialogueGraphCodec
{
public:
	// 실패 시 이전 Asset Graph를 유지한다. 편집 중 미연결 출력은 허용한다.
	static bool ExportGraph(UEdGraph* EditorGraph, UDialogueGraph* Asset, FString& OutError);
	// 실패 시 기존 EditorGraph를 유지한다. 조회에서 ID를 재발급하지 않는다.
	static bool ImportGraph(const UDialogueGraph* Asset, UEdGraph* EditorGraph, FString& OutError);
};
