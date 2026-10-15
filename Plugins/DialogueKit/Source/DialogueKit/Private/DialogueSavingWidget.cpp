#include "DialogueSavingWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/Image.h"
#include "Components/CircularThrobber.h"
#include "Components/TextBlock.h"

// 늦게 연결해도 큐의 현재 상태를 즉시 반영한다.
void UDialogueSavingWidget::BindQueue(UDialogueSaveQueue* NewQueue)
{
    if (Queue) Queue->OnSaveStateChanged.RemoveDynamic(this, &UDialogueSavingWidget::RefreshState);
    Queue = NewQueue;
    if (Queue) Queue->OnSaveStateChanged.AddUniqueDynamic(this, &UDialogueSavingWidget::RefreshState);
    RefreshState(false, false);
}

// head 완료와 다음 요청 시작 사이에도 전체 대기 개수로 표시를 유지한다.
bool UDialogueSavingWidget::IsSavePending() const
{
    return Queue && Queue->GetPendingCount() > 0 && !Queue->HasFailedWrite();
}

// 실패는 성공처럼 사라지지 않고 게임의 재시도 판단까지 남는다.
bool UDialogueSavingWidget::IsSaveFailed() const
{
    return Queue && Queue->HasFailedWrite();
}

// 입력을 가로채지 않는 우측 하단 저장 표시를 구성한다.
TSharedRef<SWidget> UDialogueSavingWidget::RebuildWidget()
{
    if (!WidgetTree) WidgetTree = NewObject<UWidgetTree>(this);
    auto* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget = Canvas;
    auto* Content = WidgetTree->ConstructWidget<UVerticalBox>();
    auto* Position = Canvas->AddChildToCanvas(Content);
    Position->SetAnchors(FAnchors(1.0f, 1.0f));
    Position->SetAlignment(FVector2D(1.0f, 1.0f));
    Position->SetPosition(FVector2D(-32, -32));
    Position->SetSize(FVector2D(140, 80));
    if (SavingImage)
    {
        auto* Image = WidgetTree->ConstructWidget<UImage>();
        Image->SetBrushFromTexture(SavingImage);
        Image->SetDesiredSizeOverride(FVector2D(32, 32));
        Indicator = Image;
    }
    else
    {
        Indicator = WidgetTree->ConstructWidget<UCircularThrobber>();
    }
    Content->AddChild(Indicator);
    StatusText = WidgetTree->ConstructWidget<UTextBlock>();
    Content->AddChild(StatusText);
    RefreshState(false, false);
    return Super::RebuildWidget();
}

// 이벤트의 일시적인 writing 값 대신 큐 전체를 조회한다. 진행/저장 함수는 호출하지 않는다.
void UDialogueSavingWidget::RefreshState(bool bWriting, bool bFailed)
{
    const bool bPending = IsSavePending();
    const bool bFailure = IsSaveFailed();
    SetVisibility(bPending || bFailure ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    if (Indicator) Indicator->SetVisibility(bPending ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    if (StatusText) StatusText->SetText(FText::FromString(bFailure ? TEXT("저장 실패") : TEXT("저장 중…")));
}

// 화면 제거는 저장을 취소하지 않는다. 재사용 시 다시 BindQueue한다.
void UDialogueSavingWidget::NativeDestruct()
{
    BindQueue(nullptr);
    Super::NativeDestruct();
}
