#include "DialogueKit.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogDialogueKit, Log, All);

void FDialogueKitModule::StartupModule()
{
	UE_LOG(LogDialogueKit, Log, TEXT("DialogueKit runtime module started"));

}

void FDialogueKitModule::ShutdownModule()
{
	UE_LOG(LogDialogueKit, Log, TEXT("DialogueKit runtime module shutdown"));
}

IMPLEMENT_MODULE(FDialogueKitModule, DialogueKit);