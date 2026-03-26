// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UTexture2D;
class SImage;

class SIMPLELOADSCREEN_API SLoadingScreen : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLoadingScreen)
		: _BackgroundTexture(nullptr)
		, _LoadingIconThrobberTexture(nullptr)
		, _LoadingIconScale(1.0f)
		, _LoadingIconThrobberRotationSpeed(120.0f)
	{
	}
		SLATE_ARGUMENT(UTexture2D*, BackgroundTexture)
		SLATE_ARGUMENT(TArray<UTexture2D*>, LoadingIconTextures)
		SLATE_ARGUMENT(UTexture2D*, LoadingIconThrobberTexture)
		// 1.0 = default icon size, 0.5 = half size, etc.
		SLATE_ARGUMENT(float, LoadingIconScale)
		// Degrees per second.
		SLATE_ARGUMENT(float, LoadingIconThrobberRotationSpeed)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	EActiveTimerReturnType HandleThrobberTick(double InCurrentTime, float InDeltaTime);
	TOptional<FSlateRenderTransform> GetThrobberRenderTransform() const;

	UTexture2D* BackgroundTexture;
	TSharedPtr<FSlateBrush> BackrgoundBrush;

	TArray<UTexture2D*> LoadingIconTextures;
	TArray<TSharedPtr<FSlateBrush>> LoadingIconBrushes;

	UTexture2D* LoadingIconThrobberTexture = nullptr;
	TSharedPtr<FSlateBrush> LoadingIconThrobberBrush;
	TSharedPtr<SImage> LoadingIconThrobberImage;

	float LoadingIconScale = 1.f;
	float LoadingIconThrobberRotationSpeed = 120.f;
	double LoadingIconThrobberStartTimeSeconds = 0.0;
};
