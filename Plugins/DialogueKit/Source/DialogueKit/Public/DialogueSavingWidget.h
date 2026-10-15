#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DialogueSaveQueue.h"
#include "DialogueSavingWidget.generated.h"

// 대화 화면과 별도로 소유하여 대화 종료 후에도 저장 결과를 표시한다.
UCLASS(BlueprintType, NotBlueprintable)
class DIALOGUEKIT_API UDialogueSavingWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="DialogueKit|Save")
    void BindQueue(UDialogueSaveQueue* NewQueue);
    UFUNCTION(BlueprintPure, Category="DialogueKit|Save")
    bool IsSavePending() const;
    UFUNCTION(BlueprintPure, Category="DialogueKit|Save")
    bool IsSaveFailed() const;
    // 설정하지 않으면 기본 원형 저장 애니메이션을 사용한다. 생성 전에 지정한다.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DialogueKit|Save")
    TObjectPtr<UTexture2D> SavingImage;
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeDestruct() override;
    UFUNCTION() void RefreshState(bool bWriting, bool bFailed);
    UPROPERTY() TObjectPtr<UDialogueSaveQueue> Queue;
    UPROPERTY() TObjectPtr<class UWidget> Indicator;
    UPROPERTY() TObjectPtr<class UTextBlock> StatusText;
};
