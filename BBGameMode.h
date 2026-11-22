#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BBCore.h"
#include "BBGameMode.generated.h"

class ABBBus;
class ABBTown;

enum class EScreen : uint8 { Title, Playing, Summary, Ending };

// Runs the whole game: clock, story, passengers, screens.
UCLASS()
class ABBGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	ABBGameMode();
	virtual void BeginPlay() override;
	virtual void Tick(float Dt) override;

	// things the HUD reads
	EScreen Screen = EScreen::Title;
	FBBState S;
	FBBDialogue Dlg;
	int32 Choice = 0;
	FString Message;       float MessageTime = 0.f;
	FString Prompt;
	FString Objective;
	FString EndTitle, EndBody;
	int32 TodayRiders = 0, TodayMoney = 0;
	UPROPERTY() ABBBus* Bus = nullptr;
	UPROPERTY() ABBTown* Town = nullptr;
	UPROPERTY() APlayerController* PC = nullptr;

private:
	TArray<FBBBeat> Beats;
	TMap<FString, float> StopCooldown;    // so you can't farm one stop forever
	TArray<FString> SceneQueue;
	TArray<int32> SleepScenes;
	FString PendingEnding;
	bool bDayEnding = false;

	void NewGame();
	void LoadBeats();
	void ShowMessage(const FString& Text);
	void PlaceBusAtDepot();
	void CheckBeats();
	void CheckStops();
	void StartScene(const FString& Id);
	void EndDay();
	void NextDay();
	bool Pressed(const FKey& K) const;
	bool Down(const FKey& K) const;
};
