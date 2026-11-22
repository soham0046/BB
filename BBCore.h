#pragma once
#include "CoreMinimal.h"

// ---------------------------------------------------------------------------------------------
//  Pure game logic (no UObjects): save state, story script parser, dialogue runner.
//  Ported 1:1 from the Unity version so the same Content/Story/*.txt scripts drive both.
// ---------------------------------------------------------------------------------------------

struct FBBLesson { FString Id, Title, Text, Perk; };

struct FBBChoice { FString Text, Effects, Goto, Cond; };
struct FBBNode { int32 Kind = 0; FString Speaker, Text, Effects, Cond, Label; TArray<FBBChoice> Choices; };   // 0 line 1 effects 2 choices 3 goto 4 end
struct FBBScene { FString Id; TArray<FBBNode> Nodes; TMap<FString, int32> Labels; };

struct FBBBeat
{
	FString Id, Place, Npc, Scene, Cond, Miss, Obj;
	int32 D0 = 0, D1 = 0; float H0 = 0.f, H1 = 0.f;
	bool bAuto = false, bPhone = false, bSleep = false, bSpawned = false;
	FVector Pos = FVector::ZeroVector;   // world position (cm)
};

class FBBState
{
public:
	int32 Day = 1; float Min = 5.f * 60.f + 50.f; int32 Cash = 140, Score = 0;
	TMap<FString, int32> V;
	TSet<FString> Flags, Lessons, Pages, Done, Missed, Owned, Upg;
	FString Ending; bool bIntroSeen = false;
	int32 Fares = 0, Tips = 0, Riders = 0, DayStartCash = 140;
	float Fuel = 1.f;

	// hooks installed by the game mode
	TFunction<void(const FString&, int32)> OnToast;
	TFunction<void(const FString&)> OnLesson, OnScene, OnEvent;

	FBBState() { Owned.Add(TEXT("bus")); }

	static const TArray<FString>& People();
	static const TArray<FBBLesson>& AllLessons();
	static const FBBLesson* FindLesson(const FString& Id);
	static FString NameOf(const FString& Id);

	int32 Weekday() const { return (Day - 1) % 7; }
	FString DayName() const;
	int32 Chapter() const { return Day <= 3 ? 0 : Day <= 7 ? 1 : Day <= 11 ? 2 : Day <= 15 ? 3 : 4; }
	int32 Season() const { const int32 C = Chapter(); return C == 4 ? 0 : C; }   // 0 autumn 1 winter 2 spring 3 summer
	float Hour() const { return Min / 60.f; }
	FString Clock() const;

	int32 Get(const FString& K) const { const int32* P = V.Find(K); return P ? *P : 0; }
	void Set(const FString& K, int32 N) { V.Add(K, N); }
	int32 Rel(const FString& Npc) const { return Get(TEXT("rel_") + Npc); }
	bool Flag(const FString& F) const { return Flags.Contains(F); }
	bool Has(const FString& LessonId) const { return Lessons.Contains(LessonId); }
	int32 Allies() const;

	void AddRel(const FString& Npc, int32 N);
	void Apply(const FString& Effects);
	bool Check(const FString& Cond) const;

	FString Serialize() const;
	static FBBState Parse(const FString& Txt);
	static FString SavePath();
	static bool HasSave();
	void Save() const;
	static FBBState Load();
};

class FBBScripts
{
public:
	static void Load();
	static const FBBScene* Get(const FString& Id);
	static TMap<FString, TArray<FString>> Chatter;
	static TArray<TArray<FString>> BeatRows;
	static TMap<FString, FString> Pages, Epilogues;
private:
	static TMap<FString, FBBScene> Scenes;
	static bool bLoaded;
	static void ParseScenes(const TArray<FString>& Lines);
	static void ParseKeyed(const TArray<FString>& Lines, TMap<FString, FString>& Into);
};

class FBBDialogue
{
public:
	FBBState* S = nullptr;
	bool bActive = false, bNarr = false;
	FString Speaker, SpeakerId, Text;
	TArray<FBBChoice> Choices;
	float Typed = 0.f;      // characters revealed (advanced by the game mode)

	bool Play(const FString& SceneId, TFunction<void()> OnDone);
	void Advance();          // next line (no-op while choices are showing)
	void Pick(int32 Index);
	void Abort();
	bool IsTyping() const { return Typed < (float)Text.Len(); }
	void SkipTyping() { Typed = (float)Text.Len(); }
private:
	const FBBScene* Sc = nullptr; int32 Pc = 0; TFunction<void()> Done;
	void Next();
	void Finish();
};
