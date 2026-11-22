#include "BBGameMode.h"
#include "BBBus.h"
#include "BBTown.h"
#include "BBHUD.h"
#include "BBLayout.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

ABBGameMode::ABBGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = ABBBus::StaticClass();   // the player IS the bus
	HUDClass = ABBHUD::StaticClass();
}

bool ABBGameMode::Pressed(const FKey& K) const { return PC && PC->WasInputKeyJustPressed(K); }
bool ABBGameMode::Down(const FKey& K) const { return PC && PC->IsInputKeyDown(K); }

void ABBGameMode::BeginPlay()
{
	Super::BeginPlay();
	FBBScripts::Load();                        // read the story text files
	LoadBeats();
	Town = GetWorld()->SpawnActor<ABBTown>();
	Town->Build();
	PC = UGameplayStatics::GetPlayerController(this, 0);
	Bus = Cast<ABBBus>(UGameplayStatics::GetPlayerPawn(this, 0));
	Dlg.S = &S;
	// when the story gives money, shows toasts etc
	S.OnToast = [this](const FString& T, int32 Kind) { ShowMessage(T); };
	S.OnScene = [this](const FString& Id) { SceneQueue.Add(Id); };
	S.OnEvent = [this](const FString& E) { if (E.StartsWith(TEXT("ending:"))) PendingEnding = E.Mid(7); };
	if (Bus) PlaceBusAtDepot();
}

void ABBGameMode::ShowMessage(const FString& Text) { Message = Text; MessageTime = 4.f; }

void ABBGameMode::LoadBeats()
{
	// each row of beats.txt: id | days | from | to | place | npc | scene | cond | miss | objective
	for (const TArray<FString>& Row : FBBScripts::BeatRows)
	{
		FBBBeat B;
		B.Id = Row[0];  B.Scene = Row[6];  B.Cond = Row[7];  B.Miss = Row[8];
		B.Obj = Row.Num() > 9 ? Row[9] : FString();
		FString Days = Row[1];
		int32 Dash;
		if (Days.FindChar(TEXT('-'), Dash)) { B.D0 = FCString::Atoi(*Days.Left(Dash)); B.D1 = FCString::Atoi(*Days.Mid(Dash + 1)); }
		else { B.D0 = FCString::Atoi(*Days); B.D1 = B.D0; }
		B.H0 = FCString::Atof(*Row[2]);
		B.H1 = FCString::Atof(*Row[3]);
		FString Place = Row[4];
		if (Place.StartsWith(TEXT("!"))) Place = Place.Mid(1);
		if (Place == TEXT("phone")) B.bPhone = true;
		else if (Place == TEXT("sleep")) B.bSleep = true;
		else B.Pos = BBL::PlaceWorld(Place);
		Beats.Add(B);
	}
}

void ABBGameMode::PlaceBusAtDepot()
{
	FVector P = BBL::PlaceWorld(TEXT("depot"));
	Bus->SetActorLocation(FVector(P.X * 0.9f, P.Y * 0.9f, 100.f));     // a bit nearer to the middle, on the road side
	Bus->SetActorRotation(FRotator(0, 0, 0));
	Bus->Speed = 0.f;
}

void ABBGameMode::NewGame()
{
	S = FBBState();
	S.OnToast = [this](const FString& T, int32 Kind) { ShowMessage(T); };
	S.OnScene = [this](const FString& Id) { SceneQueue.Add(Id); };
	S.OnEvent = [this](const FString& E) { if (E.StartsWith(TEXT("ending:"))) PendingEnding = E.Mid(7); };
	Dlg.S = &S;
	SceneQueue.Empty(); PendingEnding.Empty();
	TodayRiders = 0; TodayMoney = 0;
	PlaceBusAtDepot();
	Screen = EScreen::Playing;
}

void ABBGameMode::StartScene(const FString& Id)
{
	Choice = 0;
	Dlg.Play(Id, nullptr);
}

void ABBGameMode::Tick(float Dt)
{
	Super::Tick(Dt);
	// the player and the bus might not exist yet in BeginPlay, so grab them here if needed
	if (!PC) PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!Bus) { Bus = Cast<ABBBus>(UGameplayStatics::GetPlayerPawn(this, 0)); if (Bus) PlaceBusAtDepot(); }
	if (!PC || !Bus) return;
	if (MessageTime > 0.f) MessageTime -= Dt;

	// ---------------- title screen
	if (Screen == EScreen::Title)
	{
		if (Pressed(EKeys::Enter)) NewGame();
		if (Pressed(EKeys::C) && FBBState::HasSave()) { S = FBBState::Load(); Dlg.S = &S; S.OnToast = [this](const FString& T, int32 K) { ShowMessage(T); }; S.OnScene = [this](const FString& Id) { SceneQueue.Add(Id); }; S.OnEvent = [this](const FString& E) { if (E.StartsWith(TEXT("ending:"))) PendingEnding = E.Mid(7); }; PlaceBusAtDepot(); Screen = EScreen::Playing; }
		return;
	}

	// ---------------- end of the day summary
	if (Screen == EScreen::Summary)
	{
		if (Pressed(EKeys::Enter)) NextDay();
		return;
	}

	// ---------------- ending screen
	if (Screen == EScreen::Ending)
	{
		if (Pressed(EKeys::Enter)) { Screen = EScreen::Title; }
		return;
	}

	// ---------------- playing: conversation first
	if (Dlg.bActive)
	{
		Bus->Drive(Dt, 0.f, 0.f);                       // bus rolls to a stop while we talk
		bool bConfirm = Pressed(EKeys::Enter) || Pressed(EKeys::SpaceBar) || Pressed(EKeys::E);
		if (Dlg.Choices.Num() > 0)
		{
			int N = Dlg.Choices.Num();
			if (Pressed(EKeys::W) || Pressed(EKeys::Up)) Choice = (Choice + N - 1) % N;
			if (Pressed(EKeys::S) || Pressed(EKeys::Down)) Choice = (Choice + 1) % N;
			if (Pressed(EKeys::One) && N >= 1) { Dlg.Pick(0); Choice = 0; }
			else if (Pressed(EKeys::Two) && N >= 2) { Dlg.Pick(1); Choice = 0; }
			else if (Pressed(EKeys::Three) && N >= 3) { Dlg.Pick(2); Choice = 0; }
			else if (Pressed(EKeys::Four) && N >= 4) { Dlg.Pick(3); Choice = 0; }
			else if (bConfirm) { Dlg.Pick(Choice); Choice = 0; }
		}
		else
		{
			Dlg.Typed += Dt * 50.f;                     // letters appear one by one
			if (bConfirm)
			{
				if (Dlg.IsTyping()) Dlg.SkipTyping(); else Dlg.Advance();
			}
		}
		return;
	}

	// a scene is waiting in the queue
	if (SceneQueue.Num() > 0) { FString Id = SceneQueue[0]; SceneQueue.RemoveAt(0); StartScene(Id); return; }
	if (!PendingEnding.IsEmpty())
	{
		Screen = EScreen::Ending;
		const FString* Raw = FBBScripts::Epilogues.Find(PendingEnding);
		TArray<FString> Lines;
		if (Raw) Raw->ParseIntoArrayLines(Lines, false);
		EndTitle = Lines.Num() > 0 ? Lines[0] : FString(TEXT("The End"));
		if (Lines.Num() > 0) Lines.RemoveAt(0);
		EndBody = FString::Join(Lines, TEXT("\n"));
		IFileManager::Get().Delete(*FBBState::SavePath());
		PendingEnding.Empty();
		return;
	}

	// the day is over and all night scenes are done -> show the summary
	if (bDayEnding) { Screen = EScreen::Summary; return; }

	// ---------------- driving
	float Throttle = 0.f, Steer = 0.f;
	if (Down(EKeys::W) || Down(EKeys::Up)) Throttle += 1.f;
	if (Down(EKeys::S) || Down(EKeys::Down)) Throttle -= 1.f;
	if (Down(EKeys::D) || Down(EKeys::Right)) Steer += 1.f;
	if (Down(EKeys::A) || Down(EKeys::Left)) Steer -= 1.f;
	if (Down(EKeys::SpaceBar)) Bus->Speed *= 0.97f;      // handbrake
	Bus->Drive(Dt, Throttle, Steer);

	// clock: 3 game minutes every real second
	S.Min += Dt * 3.f;
	Town->SetHour(S.Hour());
	if (S.Min >= 22.f * 60.f) { EndDay(); return; }

	// stop cooldowns
	for (const BBL::FStop& St : BBL::Stops())
	{
		float* C = StopCooldown.Find(St.Id);
		if (C && *C > 0.f) *C -= Dt;
	}

	CheckBeats();
	CheckStops();
}

// ---------------------------------------------------------------- story beats
void ABBGameMode::CheckBeats()
{
	Prompt.Empty();
	Objective = TEXT("Drive the route. Pick up neighbors, earn fares.");
	for (FBBBeat& B : Beats)
	{
		if (B.bSleep) continue;
		if (S.Done.Contains(B.Id) || S.Missed.Contains(B.Id)) continue;
		bool InTime = S.Day >= B.D0 && S.Day <= B.D1 && S.Hour() >= B.H0 && (B.H1 <= 0.f || S.Hour() <= B.H1);
		bool Late = S.Day > B.D1 || (S.Day == B.D1 && B.H1 > 0.f && S.Hour() > B.H1);
		if (Late)
		{
			// we missed it
			if (S.Check(B.Cond)) { S.Missed.Add(B.Id); S.Apply(B.Miss); ShowMessage(TEXT("Missed: ") + B.Obj); }
			continue;
		}
		if (!InTime || !S.Check(B.Cond)) continue;
		Objective = B.Obj;
		// phone calls happen by themselves
		if (B.bPhone) { S.Done.Add(B.Id); StartScene(B.Scene); return; }
		// otherwise drive up to the place and slow down
		float Dist = FVector::Dist2D(Bus->GetActorLocation(), B.Pos);
		if (Dist < 1300.f && FMath::Abs(Bus->Speed) < 400.f)
		{
			S.Done.Add(B.Id);
			StartScene(B.Scene);
			S.Min += 10.f;
			return;
		}
	}
}

// ---------------------------------------------------------------- passengers (simple version)
void ABBGameMode::CheckStops()
{
	for (const BBL::FStop& St : BBL::Stops())
	{
		float Dist = FVector::Dist2D(Bus->GetActorLocation(), BBL::ToWorld(St.X, St.Z));
		if (Dist > 1000.f || FMath::Abs(Bus->Speed) > 150.f) continue;
		float* Cool = StopCooldown.Find(St.Id);
		if (Cool && *Cool > 0.f) continue;
		Prompt = FString::Printf(TEXT("Press E: pick up passengers at %s"), *St.Name);
		if (Pressed(EKeys::E))
		{
			int Fare = FMath::RandRange(3, 8);
			if (S.Has(TEXT("ground"))) Fare += 1;          // lesson "Hit the ground" pays a bit more
			S.Cash += Fare; S.Riders++; TodayRiders++; TodayMoney += Fare;
			StopCooldown.Add(St.Id, 45.f);
			ShowMessage(FString::Printf(TEXT("+$%d"), Fare));
		}
		return;
	}
}

// ---------------------------------------------------------------- end of day
void ABBGameMode::EndDay()
{
	// scenes that happen "during the night"
	for (int32 i = 0; i < Beats.Num(); i++)
	{
		FBBBeat& B = Beats[i];
		if (B.bSleep && B.D0 <= S.Day && S.Day <= B.D1 && !S.Done.Contains(B.Id) && S.Check(B.Cond))
		{
			S.Done.Add(B.Id);
			SceneQueue.Add(B.Scene);
		}
	}
	bDayEnding = true;
	// the queue plays first (see Tick), then Tick shows the summary
}

void ABBGameMode::NextDay()
{
	if (S.Day < 18) S.Day++;
	S.Min = 6.f * 60.f + 20.f;
	TodayRiders = 0; TodayMoney = 0;
	for (auto& Pair : StopCooldown) { Pair.Value = 0.f; }
	PlaceBusAtDepot();
	bDayEnding = false;
	S.Save();
	Screen = EScreen::Playing;
}
