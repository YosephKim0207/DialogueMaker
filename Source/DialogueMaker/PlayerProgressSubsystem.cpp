#include "PlayerProgressSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(PlayerProgressSubsystem, Log, All);

UPlayerProgressSubsystem* UPlayerProgressSubsystem::Get(const UObject* WorldContextObject)
{
	if (WorldContextObject == nullptr)
	{
		return nullptr;
	}

	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (World == nullptr)
	{
		UE_LOG(PlayerProgressSubsystem, Warning, TEXT("UPlayerProgressSubsystem::Get : World is nullptr"));

		return nullptr;
	}

	if (UGameInstance* GameInstance = World->GetGameInstance())
	{
		return GameInstance->GetSubsystem<UPlayerProgressSubsystem>();
	}
				
	UE_LOG(PlayerProgressSubsystem, Warning, TEXT("UPlayerProgressSubsystem::Get : GameInstance is nullptr"));

	return nullptr;
}
