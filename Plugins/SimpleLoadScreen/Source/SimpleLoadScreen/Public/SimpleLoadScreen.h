#pragma once

#include "LevelLoadingSettings.h"
#include "CoreMinimal.h"

class UTexture2D;

// Your plugin module class definition
class FSimpleLoadScreenModule : public IModuleInterface
{
public:
	/** Called right after the module DLL is loaded */
	virtual void StartupModule() override;

	/** Called before the module is unloaded */
	virtual void ShutdownModule() override;

	virtual void StartLoadingScreen(const FString& MapName);

private:
	void OnPreLoadMap(const FString& MapName);
	void OnPostLoadMap(UWorld* LoadedWorld);

	/** Called on the render thread after each fully completed frame */
	void OnEndFrameRenderThread();

	/** Ticker handle used to poll PSO + streaming + async load completion */
	FTSTicker::FDelegateHandle ShaderWarmupTickerHandle;

	/** The world that just finished loading — used to poll streaming state */
	TWeakObjectPtr<UWorld> LoadedWorldWeak;

	/** How many fully completed render-thread frames we have seen post-load */
	TAtomic<int32> RenderedFrameCount{ 0 };

	/** Guards against dismissing the screen more than once */
	TAtomic<bool> bLoadingScreenDismissed{ false };

	/** Guards against double-registering OnEndFrameRT across map loads */
	TAtomic<bool> bEndFrameRTRegistered{ false };

	TArray<FLoadedLevelLoadingScreens> LoadingScreensPerLevel;

	// Global loading icon part textures kept alive for the duration of the module
	TArray<UTexture2D*> LoadingIconTextures;

	// Separate rotating throbber texture
	UTexture2D* LoadingIconThrobberTexture = nullptr;
};