// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "LevelLoadingSettings.generated.h"

USTRUCT(BlueprintType)
struct FLevelLoadingScreens
{
	GENERATED_BODY()
public:

	UPROPERTY(Config, EditAnywhere, Category = "Loading Screen", meta = (AllowedClasses = "/Script/Engine.World"))
	FSoftObjectPath LoadingScreenLevel;

	UPROPERTY(Config, EditAnywhere, Category = "Loading Screen", meta = (AllowedClasses = "/Script/Engine.Texture"))
	TArray<FSoftObjectPath> LoadingScreens;

	UPROPERTY(Config, EditAnywhere, Category = "Loading Screen")
	bool RandomizeLoadingScreenLevel = false;
};

USTRUCT(BlueprintType)
struct FLoadedLevelLoadingScreens
{
	GENERATED_BODY()

public:

	FSoftObjectPath LoadingScreenLevel;

	TArray<UTexture2D*> LoadingScreens;

	bool RandomizeLoadingScreenLevel = false;

	FLoadedLevelLoadingScreens() = default;

	FLoadedLevelLoadingScreens(FSoftObjectPath LoadingScreenLevelIn)
		: LoadingScreenLevel(LoadingScreenLevelIn)
	{
	}
};

UCLASS(Config = "Game", DefaultConfig, meta = (DisplayName = "Loading Screen"))
class SIMPLELOADSCREEN_API ULevelLoadingSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	UPROPERTY(Config, EditAnywhere, Category = "Loading Screen")
	TArray<FLevelLoadingScreens> LoadingScreensPerLevel;

	UPROPERTY(Config, EditAnywhere, Category = "Loading Screen")
	float MinimumLoadingScreenDisplayTime = 3.f;

	// Icon parts drawn on top of each other at the same centered position.
	UPROPERTY(Config, EditAnywhere, Category = "Loading Screen", meta = (AllowedClasses = "/Script/Engine.Texture"))
	TArray<FSoftObjectPath> LoadingIconParts;

	// Separate rotating throbber texture (for example: arrow over compass).
	UPROPERTY(Config, EditAnywhere, Category = "Loading Screen", meta = (AllowedClasses = "/Script/Engine.Texture"))
	FSoftObjectPath LoadingIconThrobberTexture;

	// Rotation speed in degrees per second.
	UPROPERTY(Config, EditAnywhere, Category = "Loading Screen", meta = (ClampMin = "0.0", ClampMax = "1440.0"))
	float LoadingIconThrobberRotationSpeed = 120.f;

	// 1.0 = default icon size, 0.5 = half size, etc.
	UPROPERTY(Config, EditAnywhere, Category = "Loading Screen", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float LoadingIconScale = 1.f;
};
