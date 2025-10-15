#include "DialogueKit.h"
#include "Modules/ModuleManager.h"

class FDialogueKitModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
    }

    virtual void ShutdownModule() override
    {
    }
};

IMPLEMENT_MODULE(FDialogueKitModule, DialogueKit);
