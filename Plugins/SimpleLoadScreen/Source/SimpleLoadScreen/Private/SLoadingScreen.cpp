// Fill out your copyright notice in the Description page of Project Settings.

#include "SLoadingScreen.h"
#include "SlateOptMacros.h"
#include "SlateExtras.h"
#include "Engine/Texture2D.h"
#include "Framework/Application/SlateApplication.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SLoadingScreen::Construct(const FArguments& InArgs)
{
	BackgroundTexture = InArgs._BackgroundTexture;
	LoadingIconTextures = InArgs._LoadingIconTextures;
	LoadingIconThrobberTexture = InArgs._LoadingIconThrobberTexture;
	LoadingIconScale = FMath::Clamp(InArgs._LoadingIconScale, 0.1f, 5.0f);
	LoadingIconThrobberRotationSpeed = FMath::Max(0.0f, InArgs._LoadingIconThrobberRotationSpeed);
	LoadingIconThrobberStartTimeSeconds = FSlateApplication::IsInitialized()
		? FSlateApplication::Get().GetCurrentTime()
		: FPlatformTime::Seconds();

	BackrgoundBrush = MakeShared<FSlateBrush>();

	if (BackgroundTexture)
	{
		BackrgoundBrush->SetResourceObject(BackgroundTexture);
		BackrgoundBrush->ImageSize = FVector2D(BackgroundTexture->GetSizeX(), BackgroundTexture->GetSizeY());
		BackrgoundBrush->DrawAs = ESlateBrushDrawType::Image;
		BackrgoundBrush->Tiling = ESlateBrushTileType::NoTile;
	}

	float ApplicationScale = 1.0f;
	if (FSlateApplication::IsInitialized())
	{
		ApplicationScale = FSlateApplication::Get().GetApplicationScale();
	}
	const float SafeAppScale = FMath::Max(KINDA_SMALL_NUMBER, ApplicationScale);

	TSharedRef<SOverlay> IconPartsOverlay = SNew(SOverlay);
	bool bHasAnyIconPart = false;

	for (UTexture2D* LoadingIconTexture : LoadingIconTextures)
	{
		if (!LoadingIconTexture)
		{
			continue;
		}

		TSharedPtr<FSlateBrush> LoadingIconBrush = MakeShared<FSlateBrush>();
		LoadingIconBrush->SetResourceObject(LoadingIconTexture);
		LoadingIconBrush->ImageSize = FVector2D(LoadingIconTexture->GetSizeX(), LoadingIconTexture->GetSizeY());
		LoadingIconBrush->DrawAs = ESlateBrushDrawType::Image;
		LoadingIconBrush->Tiling = ESlateBrushTileType::NoTile;

		const FVector2D DesiredPhysicalSize = (LoadingIconBrush->ImageSize * LoadingIconScale) * ApplicationScale;
		const FVector2D SnappedPhysicalSize(
			FMath::Max(1.0f, FMath::RoundToFloat(DesiredPhysicalSize.X)),
			FMath::Max(1.0f, FMath::RoundToFloat(DesiredPhysicalSize.Y))
		);
		const FVector2D DrawSize = SnappedPhysicalSize / SafeAppScale;

		LoadingIconBrushes.Add(LoadingIconBrush);
		bHasAnyIconPart = true;

		IconPartsOverlay->AddSlot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(DrawSize.X)
			.HeightOverride(DrawSize.Y)
			[
				SNew(SImage)
				.Image(LoadingIconBrush.Get())
				.ColorAndOpacity(FLinearColor::White)
			]
		];
	}

	FVector2D ThrobberDrawSize(0.f, 0.f);
	if (LoadingIconThrobberTexture)
	{
		LoadingIconThrobberBrush = MakeShared<FSlateBrush>();
		LoadingIconThrobberBrush->SetResourceObject(LoadingIconThrobberTexture);
		LoadingIconThrobberBrush->ImageSize = FVector2D(
			LoadingIconThrobberTexture->GetSizeX(),
			LoadingIconThrobberTexture->GetSizeY()
		);
		LoadingIconThrobberBrush->DrawAs = ESlateBrushDrawType::Image;
		LoadingIconThrobberBrush->Tiling = ESlateBrushTileType::NoTile;

		const FVector2D DesiredPhysicalSize = (LoadingIconThrobberBrush->ImageSize * LoadingIconScale) * ApplicationScale;
		const FVector2D SnappedPhysicalSize(
			FMath::Max(1.0f, FMath::RoundToFloat(DesiredPhysicalSize.X)),
			FMath::Max(1.0f, FMath::RoundToFloat(DesiredPhysicalSize.Y))
		);
		ThrobberDrawSize = SnappedPhysicalSize / SafeAppScale;
	}

	ChildSlot
	[
		SNew(SOverlay)

		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SImage)
			.Image(BackgroundTexture ? BackrgoundBrush.Get() : nullptr)
			.ColorAndOpacity(FLinearColor::White)
		]

		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.Visibility(bHasAnyIconPart ? EVisibility::HitTestInvisible : EVisibility::Collapsed)
			[
				IconPartsOverlay
			]
		]

		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.Visibility(LoadingIconThrobberBrush.IsValid() ? EVisibility::HitTestInvisible : EVisibility::Collapsed)
			.WidthOverride(ThrobberDrawSize.X)
			.HeightOverride(ThrobberDrawSize.Y)
			[
				SAssignNew(LoadingIconThrobberImage, SImage)
				.Image(LoadingIconThrobberBrush.IsValid() ? LoadingIconThrobberBrush.Get() : nullptr)
				.ColorAndOpacity(FLinearColor::White)
				.RenderTransform(this, &SLoadingScreen::GetThrobberRenderTransform)
				.RenderTransformPivot(FVector2D(0.5f, 0.5f))
			]
		]
	];

	if (LoadingIconThrobberBrush.IsValid() && LoadingIconThrobberRotationSpeed > 0.0f)
	{
		RegisterActiveTimer(0.0f, FWidgetActiveTimerDelegate::CreateSP(this, &SLoadingScreen::HandleThrobberTick));
	}
}

EActiveTimerReturnType SLoadingScreen::HandleThrobberTick(double InCurrentTime, float InDeltaTime)
{
	if (!LoadingIconThrobberImage.IsValid() || LoadingIconThrobberRotationSpeed <= 0.0f)
	{
		return EActiveTimerReturnType::Stop;
	}

	LoadingIconThrobberImage->Invalidate(EInvalidateWidgetReason::Paint);
	return EActiveTimerReturnType::Continue;
}

TOptional<FSlateRenderTransform> SLoadingScreen::GetThrobberRenderTransform() const
{
	if (LoadingIconThrobberRotationSpeed <= 0.0f)
	{
		return FSlateRenderTransform();
	}

	const double CurrentTimeSeconds = FSlateApplication::IsInitialized()
		? FSlateApplication::Get().GetCurrentTime()
		: FPlatformTime::Seconds();
	const double ElapsedSeconds = CurrentTimeSeconds - LoadingIconThrobberStartTimeSeconds;
	const double AngleDegrees = FMath::Fmod(ElapsedSeconds * LoadingIconThrobberRotationSpeed, 360.0);

	return FSlateRenderTransform(FQuat2D(FMath::DegreesToRadians(static_cast<float>(AngleDegrees))));
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION
