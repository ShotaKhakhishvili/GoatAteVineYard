#include "SimpleLoadScreen.h"
#include "SLoadingScreen.h"
#include "MoviePlayer.h"
#include "Modules/ModuleManager.h"
#include "ShaderPipelineCache.h"
#include "Misc/CoreDelegates.h"
#include "LevelLoadingSettings.h"
#include "Misc/PackageName.h"
#include "RenderingThread.h"

// Added includes to get UTexture2D definition and TObjectIterator
#include "Engine/Texture2D.h"
#include "UObject/UObjectIterator.h"

IMPLEMENT_MODULE(FSimpleLoadScreenModule, SimpleLoadScreen);

// Minimum fully-rendered frames before the loading screen is dismissed.
// Each frame here is confirmed by the render thread, not the game thread,
// so every frame guarantees all draw calls and shaders for it are done.
static constexpr int32 MinRenderedFramesRequired = 4;

void FSimpleLoadScreenModule::StartupModule()
{
	UE_LOG(LogTemp, Warning, TEXT("[LoadScreen] StartupModule — phase: PreLoadingScreen"));

	if (const ULevelLoadingSettings* Settings = GetDefault<ULevelLoadingSettings>())
	{
		UE_LOG(LogTemp, Warning, TEXT("[LoadScreen] Settings found. Level entries: %d"), Settings->LoadingScreensPerLevel.Num());

		for (const auto& LoadingScreenDataForLevel : Settings->LoadingScreensPerLevel)
		{
			FLoadedLevelLoadingScreens LoadedData(LoadingScreenDataForLevel.LoadingScreenLevel);

			UE_LOG(LogTemp, Warning, TEXT("[LoadScreen] Processing level: '%s' with %d texture path(s)"),
				*LoadingScreenDataForLevel.LoadingScreenLevel.ToString(),
				LoadingScreenDataForLevel.LoadingScreens.Num());

			for (const FSoftObjectPath& LoadingScreen : LoadingScreenDataForLevel.LoadingScreens)
			{
				UE_LOG(LogTemp, Warning, TEXT("[LoadScreen]   Attempting TryLoad: '%s'"), *LoadingScreen.ToString());

				if (UTexture2D* Texture = Cast<UTexture2D>(LoadingScreen.TryLoad()))
				{
					Texture->AddToRoot();
					LoadedData.LoadingScreens.Add(Texture);
					UE_LOG(LogTemp, Warning, TEXT("[LoadScreen]   ✅ Loaded texture: '%s' (%dx%d)"),
						*LoadingScreen.ToString(), Texture->GetSizeX(), Texture->GetSizeY());
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("[LoadScreen]   ❌ TryLoad FAILED for: '%s'"), *LoadingScreen.ToString());
				}
			}

			LoadingScreensPerLevel.Add(LoadedData);
		}

		// Load global loading icon part textures and keep them rooted.
		UE_LOG(LogTemp, Warning, TEXT("[LoadScreen] Loading icon parts: %d"), Settings->LoadingIconParts.Num());

		for (const FSoftObjectPath& LoadingIconPartPath : Settings->LoadingIconParts)
		{
			UE_LOG(LogTemp, Warning, TEXT("[LoadScreen]   Attempting icon part TryLoad: '%s'"), *LoadingIconPartPath.ToString());

			if (UTexture2D* Texture = Cast<UTexture2D>(LoadingIconPartPath.TryLoad()))
			{
				Texture->AddToRoot();
				Texture->NeverStream = true;
				Texture->UpdateResource();
				LoadingIconTextures.Add(Texture);

				UE_LOG(LogTemp, Warning, TEXT("[LoadScreen]   ✅ Loaded icon part: '%s' (%dx%d)"),
					*LoadingIconPartPath.ToString(), Texture->GetSizeX(), Texture->GetSizeY());
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[LoadScreen]   ❌ Icon part TryLoad FAILED for: '%s'"), *LoadingIconPartPath.ToString());
			}
		}

		if (LoadingIconTextures.Num() == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[LoadScreen] No loading icon parts were loaded. Center icon will be hidden."));
		}

		if (Settings->LoadingIconThrobberTexture.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("[LoadScreen]   Attempting throbber TryLoad: '%s'"),
				*Settings->LoadingIconThrobberTexture.ToString());

			if (UTexture2D* Texture = Cast<UTexture2D>(Settings->LoadingIconThrobberTexture.TryLoad()))
			{
				Texture->AddToRoot();
				Texture->NeverStream = true;
				Texture->UpdateResource();
				LoadingIconThrobberTexture = Texture;

				UE_LOG(LogTemp, Warning, TEXT("[LoadScreen]   ✅ Loaded throbber: '%s' (%dx%d)"),
					*Settings->LoadingIconThrobberTexture.ToString(), Texture->GetSizeX(), Texture->GetSizeY());
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[LoadScreen]   ❌ Throbber TryLoad FAILED for: '%s'"),
					*Settings->LoadingIconThrobberTexture.ToString());
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[LoadScreen] ❌ GetDefault<ULevelLoadingSettings>() returned null — settings not available yet"));
	}

	FCoreUObjectDelegates::PreLoadMap.AddRaw(this, &FSimpleLoadScreenModule::OnPreLoadMap);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddRaw(this, &FSimpleLoadScreenModule::OnPostLoadMap);
}

void FSimpleLoadScreenModule::ShutdownModule()
{
	FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
	FCoreDelegates::OnEndFrameRT.RemoveAll(this);

	if (ShaderWarmupTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(ShaderWarmupTickerHandle);
	}

	// Do NOT RemoveFromRoot / null LoadingIconTextures here.
	// On shutdown, Slate and MoviePlayer may still reference them.
	// The process is exiting anyway, so leaking this reference is acceptable.

	UE_LOG(LogTemp, Log, TEXT("SimpleLoadScreen module shutdown"));
}

void FSimpleLoadScreenModule::OnPreLoadMap(const FString& MapName)
{
	UE_LOG(LogTemp, Warning, TEXT("[LoadScreen] >>> OnPreLoadMap fired — DESTINATION map: '%s'"), *MapName);
	RenderedFrameCount = 0;
	bLoadingScreenDismissed = false;
	bEndFrameRTRegistered = false;
	StartLoadingScreen(MapName);
}

void FSimpleLoadScreenModule::OnPostLoadMap(UWorld* LoadedWorld)
{
	if (ShaderWarmupTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(ShaderWarmupTickerHandle);
	}

	LoadedWorldWeak = LoadedWorld;

	ShaderWarmupTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda([this](float DeltaTime) -> bool
		{
			// Step 1 — PSO cache must finish first
			if (FShaderPipelineCache::NumPrecompilesRemaining() > 0)
			{
				return true;
			}

			// Step 2 — wait for all async asset package loads to drain
			if (IsAsyncLoading())
			{
				return true;
			}

			// Step 3 — wait for level streaming visibility to fully settle
			UWorld* World = LoadedWorldWeak.Get();
			if (World && World->IsVisibilityRequestPending())
			{
				return true;
			}

			// Step 4 — force texture mips to be resident for a short duration so
			// texture streaming has time to upload high mips before we stop the movie.
			{
				// Small duration to avoid holding memory forever; tune as needed.
				const float ForceDurationSeconds = 3.0f;

				// Force residency on cached loading-screen textures first (cheap).
				for (const auto& LoadedLevel : LoadingScreensPerLevel)
				{
					for (UTexture2D* Tex : LoadedLevel.LoadingScreens)
					{
						if (IsValid(Tex))
						{
							Tex->SetForceMipLevelsToBeResident(ForceDurationSeconds);
						}
					}
				}

				// Force residency on loading icon part textures.
				for (UTexture2D* Tex : LoadingIconTextures)
				{
					if (IsValid(Tex))
					{
						Tex->SetForceMipLevelsToBeResident(ForceDurationSeconds);
					}
				}

				// Also nudge all currently loaded streaming textures so world content has a chance.
				for (TObjectIterator<UTexture2D> It; It; ++It)
				{
					UTexture2D* Tex = *It;
					if (IsValid(Tex))
					{
						Tex->SetForceMipLevelsToBeResident(ForceDurationSeconds);
					}
				}

				// Force residency on throbber texture if valid
				if (IsValid(LoadingIconThrobberTexture))
				{
					LoadingIconThrobberTexture->SetForceMipLevelsToBeResident(ForceDurationSeconds);
				}

				// Small flush to give the RHI a chance to process (non-blocking).
				FlushRenderingCommands();
			}

			// Step 5 — everything settled; register the RT frame counter
			if (!bEndFrameRTRegistered)
			{
				bEndFrameRTRegistered = true;
				FCoreDelegates::OnEndFrameRT.AddRaw(this, &FSimpleLoadScreenModule::OnEndFrameRenderThread);
				UE_LOG(LogTemp, Log, TEXT("[LoadScreen] All systems settled — waiting for render thread frames."));
			}

			ShaderWarmupTickerHandle.Reset();
			return false;
		})
	);
}

void FSimpleLoadScreenModule::OnEndFrameRenderThread()
{
	// RENDER THREAD — do NOT touch any delegate list here.
	// Calling RemoveAll() from inside an active delegate broadcast corrupts
	// the invocation list and triggers the IsInGameThread() assertion in
	// SkeletalMeshUpdater. Use only atomics and AsyncTask here.
	const int32 NewCount = ++RenderedFrameCount;

	if (NewCount < MinRenderedFramesRequired)
	{
		return;
	}

	// Hand everything off to the game thread.
	// Delegate removal, FlushRenderingCommands and StopMovie all require
	// the game thread — doing them here would crash.
	AsyncTask(ENamedThreads::GameThread, [this]()
	{
		if (bLoadingScreenDismissed)
		{
			return;
		}

		bLoadingScreenDismissed = true;

		// Safe to remove the render thread delegate from the game thread
		// because we are no longer inside the OnEndFrameRT broadcast
		FCoreDelegates::OnEndFrameRT.RemoveAll(this);
		bEndFrameRTRegistered = false;

		// Drain all pending GPU commands before dismissing
		FlushRenderingCommands();

		if (GetMoviePlayer()->IsMovieCurrentlyPlaying())
		{
			GetMoviePlayer()->StopMovie();
			UE_LOG(LogTemp, Log, TEXT("Loading screen dismissed after render thread confirmed %d fully rendered frames."), MinRenderedFramesRequired);
		}
	});
}

void FSimpleLoadScreenModule::StartLoadingScreen(const FString& MapName)
{
	UE_LOG(LogTemp, Warning, TEXT("[LoadScreen] StartLoadingScreen — raw: '%s'  normalized: '%s'"), *MapName, *FPackageName::ObjectPathToPackageName(MapName));

	const ULevelLoadingSettings* Settings = GetDefault<ULevelLoadingSettings>();
	if (!Settings) return;

	UTexture2D* BackgroundTexture = nullptr;
	const FString MapPackageName = FPackageName::ObjectPathToPackageName(MapName);

	for (const auto& LoadedLevel : LoadingScreensPerLevel)
	{
		const FString ConfigLevelPackage =
			FPackageName::ObjectPathToPackageName(LoadedLevel.LoadingScreenLevel.ToString());

		if (ConfigLevelPackage != MapPackageName)
			continue;

		if (LoadedLevel.LoadingScreens.Num() == 0)
			break;

		const int32 Index = FMath::RandRange(0, LoadedLevel.LoadingScreens.Num() - 1);
		BackgroundTexture = LoadedLevel.LoadingScreens.IsValidIndex(Index) ? LoadedLevel.LoadingScreens[Index] : nullptr;
		break;
	}

	if (!BackgroundTexture)
	{
		UE_LOG(LogTemp, Error, TEXT("[LoadScreen] ❌ No texture found for '%s'"), *MapPackageName);
		return;
	}

	const float IconScale = Settings->LoadingIconScale;
	const float ThrobberRotationSpeed = Settings->LoadingIconThrobberRotationSpeed;

	FLoadingScreenAttributes LoadingScreen;
	LoadingScreen.bAutoCompleteWhenLoadingCompletes = false;
	LoadingScreen.MinimumLoadingScreenDisplayTime = Settings->MinimumLoadingScreenDisplayTime;
	LoadingScreen.WidgetLoadingScreen =
		SNew(SLoadingScreen)
		.BackgroundTexture(BackgroundTexture)
		.LoadingIconTextures(LoadingIconTextures)
		.LoadingIconThrobberTexture(LoadingIconThrobberTexture)
		.LoadingIconScale(IconScale)
		.LoadingIconThrobberRotationSpeed(ThrobberRotationSpeed);

	GetMoviePlayer()->SetupLoadingScreen(LoadingScreen);

	// Ensure the movie starts even if MoviePlayer's own PreLoadMap already fired.
	GetMoviePlayer()->PlayMovie();

	UE_LOG(LogTemp, Warning, TEXT("[LoadScreen] ✅ SetupLoadingScreen + PlayMovie called"));
}