// Copyright Epic Games, Inc. All Rights Reserved.

#include "DialogueMaker.h"
#include "Modules/ModuleManager.h"
#include "DialoguePackageValidation.h"

class FDialogueMakerModule : public FDefaultGameModuleImpl
{
public:
	// 일반 게임 동작은 유지하고 명시적 배포 검증 인자만 처리한다.
	virtual void StartupModule() override
	{
		FDefaultGameModuleImpl::StartupModule();
		StartDialoguePackageValidation();
	}
	// 검증 ticker가 모듈보다 오래 남지 않게 한다.
	virtual void ShutdownModule() override
	{
		StopDialoguePackageValidation();
		FDefaultGameModuleImpl::ShutdownModule();
	}
};
IMPLEMENT_PRIMARY_GAME_MODULE(FDialogueMakerModule, DialogueMaker, "DialogueMaker");
