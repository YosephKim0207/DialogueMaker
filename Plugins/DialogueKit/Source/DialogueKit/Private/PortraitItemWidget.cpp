// Fill out your copyright notice in the Description page of Project Settings.


#include "PortraitItemWidget.h"
#include "Components/Image.h"

void UPortraitItemWidget::SetBaseTranslation(const FVector2D& TargetPosition)
{
	FWidgetTransform NewTransform = GetRenderTransform();
	NewTransform.Translation = TargetPosition;
	SetRenderTransform(NewTransform);
}

void UPortraitItemWidget::SetEmoteImageTranslation(const FVector2D& TargetPosition)
{
	if (EmoteImage == nullptr)
	{
		return;
	}
	
	FWidgetTransform NewTransform = EmoteImage->GetRenderTransform();
	NewTransform.Translation = TargetPosition;
	EmoteImage->SetRenderTransform(NewTransform);
}

void UPortraitItemWidget::ResetTranslation()
{
	FWidgetTransform NewBaseTransform = GetRenderTransform();
	NewBaseTransform.Translation = FVector2D::ZeroVector;
	SetRenderTransform(NewBaseTransform);

	if (PortraitImage)
	{
		FWidgetTransform NewPortraitImageTransform = PortraitImage->GetRenderTransform();
		NewPortraitImageTransform.Translation = BasePortraitImageTranslation;
		PortraitImage->SetRenderTransform(NewPortraitImageTransform);
	}
	
	if (EmoteImage)
	{
		FWidgetTransform NewEmoteImageTransform = EmoteImage->GetRenderTransform();
		NewEmoteImageTransform.Translation = BaseEmoteImageTranslation;
		EmoteImage->SetRenderTransform(NewEmoteImageTransform);
	}
}

void UPortraitItemWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (PortraitImage)
	{
		BasePortraitImageTranslation = PortraitImage->GetRenderTransform().Translation;
	}

	if (EmoteImage)
	{
		BaseEmoteImageTranslation = EmoteImage->GetRenderTransform().Translation;
	}
}
