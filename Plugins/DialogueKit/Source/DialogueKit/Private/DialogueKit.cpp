#include "DialogueKit.h"

#include "DialogueSettings.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY_STATIC(DialogueKitModuleLog, Log, All);

void FDialogueKitModule::StartupModule()
{
	UE_LOG(DialogueKitModuleLog, Display, TEXT("FDialogueKitModule::StartupModule : Enter"));

	const UDialogueSettings* DialogueSettings = GetDefault<UDialogueSettings>();
	if (DialogueSettings)
	{
		DialogueSettings->ApplyKitAssetToAssetManager();
	}
}

void FDialogueKitModule::ShutdownModule()
{
	UE_LOG(DialogueKitModuleLog, Display, TEXT("FDialogueKitModule::StartupModule : End"));
}

IMPLEMENT_MODULE(FDialogueKitModule, DialogueKit);