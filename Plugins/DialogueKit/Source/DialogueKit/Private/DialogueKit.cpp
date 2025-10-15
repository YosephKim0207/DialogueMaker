#include "DialogueKit.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogDialogueKit, Log, All);

void FDialogueKitModule::StartupModule()
{
	UE_LOG(LogDialogueKit, Display, TEXT("FDialogueKitModule::StartupModule : Enter"));

}

void FDialogueKitModule::ShutdownModule()
{
	UE_LOG(LogDialogueKit, Display, TEXT("FDialogueKitModule::StartupModule : End"));
}

IMPLEMENT_MODULE(FDialogueKitModule, DialogueKit);