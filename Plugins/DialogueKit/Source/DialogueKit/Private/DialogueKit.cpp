#include "DialogueKit.h"

#include "DialogueSettings.h"
#include "Engine/AssetManager.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY_STATIC(DialogueKitModuleLog, Log, All);

void FDialogueKitModule::StartupModule()
{
	UE_LOG(DialogueKitModuleLog, Display, TEXT("FDialogueKitModule::StartupModule : Enter"));

	
	if (GEngine && UAssetManager::IsInitialized())
	{
		HandlePostEngineInit();
		return;	
	}

	PostEngineInitHandle = FCoreDelegates::OnPostEngineInit.AddRaw(this, &FDialogueKitModule::HandlePostEngineInit);
}

void FDialogueKitModule::ShutdownModule()
{
	UE_LOG(DialogueKitModuleLog, Display, TEXT("FDialogueKitModule::StartupModule : End"));

	if (PostEngineInitHandle.IsValid())
	{
		FCoreDelegates::OnPostEngineInit.Remove(PostEngineInitHandle);
		PostEngineInitHandle.Reset();
	}
}

// 엔진 초기화 이전 ApplyKitAssetToAssetManager 호출시 UAssetManager::Get에서 크래시 발생 방지
void FDialogueKitModule::HandlePostEngineInit()
{
	if (PostEngineInitHandle.IsValid())
	{
		FCoreDelegates::OnPostEngineInit.Remove(PostEngineInitHandle);
		PostEngineInitHandle.Reset();
	}
	
	if (const UDialogueSettings* DialogueSettings = GetDefault<UDialogueSettings>())
	{
		DialogueSettings->ApplyKitAssetToAssetManager();
	}
}

IMPLEMENT_MODULE(FDialogueKitModule, DialogueKit);
