#include "BBCore.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

TMap<FString, TArray<FString>> FBBScripts::Chatter;
TArray<TArray<FString>> FBBScripts::BeatRows;
TMap<FString, FString> FBBScripts::Pages;
TMap<FString, FString> FBBScripts::Epilogues;
TMap<FString, FBBScene> FBBScripts::Scenes;
bool FBBScripts::bLoaded = false;

// ============================================================================ State
const TArray<FString>& FBBState::People()
{
	static const TArray<FString> P = { TEXT("ruth"), TEXT("teo"), TEXT("hal"), TEXT("greer"), TEXT("mo"), TEXT("lena"), TEXT("dara") };
	return P;
}

const TArray<FBBLesson>& FBBState::AllLessons()
{
	static const TArray<FBBLesson> L = {
		{ TEXT("slow"), TEXT("Slow is smooth"), TEXT("You can't outrun a morning you already missed. Smooth beats fast."), TEXT("Passengers tolerate rough roads far better.") },
		{ TEXT("ask"), TEXT("Ask early"), TEXT("Help is cheaper before the crisis."), TEXT("Restoration costs 20% less.") },
		{ TEXT("ledger"), TEXT("Keep the ledger"), TEXT("Dignity is also a kind of fare."), TEXT("Neighbors you carry warm to you faster.") },
		{ TEXT("thaw"), TEXT("Let it thaw"), TEXT("Some things only move when they're ready."), TEXT("Passengers wait 25% longer.") },
		{ TEXT("fixit"), TEXT("Show up"), TEXT("Be where you said you'd be. The rest can be repaired."), TEXT("Repairs are half price.") },
		{ TEXT("say"), TEXT("Say it plain"), TEXT("Say the true thing early and kindly."), TEXT("Relationships grow a little faster.") },
		{ TEXT("share"), TEXT("Share the road"), TEXT("Nobody arrives alone."), TEXT("Jobs pay 15% more.") },
		{ TEXT("ground"), TEXT("Hit the ground"), TEXT("The ball only bounces if it lets itself land."), TEXT("Every fare pays 10% more.") },
	};
	return L;
}

const FBBLesson* FBBState::FindLesson(const FString& Id)
{
	for (const FBBLesson& L : AllLessons()) if (L.Id == Id) return &L;
	return nullptr;
}

FString FBBState::NameOf(const FString& Id)
{
	static TMap<FString, FString> N;
	if (N.Num() == 0)
	{
		N.Add(TEXT("noor"), TEXT("Noor")); N.Add(TEXT("ruth"), TEXT("Ruth")); N.Add(TEXT("teo"), TEXT("Teo")); N.Add(TEXT("hal"), TEXT("Hal"));
		N.Add(TEXT("greer"), TEXT("Ines Greer")); N.Add(TEXT("mo"), TEXT("Mo")); N.Add(TEXT("lena"), TEXT("Lena")); N.Add(TEXT("dara"), TEXT("Dara"));
		N.Add(TEXT("pike"), TEXT("Mr. Pike")); N.Add(TEXT("anders"), TEXT("Anders")); N.Add(TEXT("priya"), TEXT("Priya")); N.Add(TEXT("eli"), TEXT("Eli"));
		N.Add(TEXT("passenger"), TEXT("Passenger"));
	}
	const FString* F = N.Find(Id.ToLower());
	return F ? *F : Id;
}

FString FBBState::DayName() const
{
	static const TCHAR* D[] = { TEXT("Monday"), TEXT("Tuesday"), TEXT("Wednesday"), TEXT("Thursday"), TEXT("Friday"), TEXT("Saturday"), TEXT("Sunday") };
	return D[Weekday()];
}

FString FBBState::Clock() const
{
	const int32 M = (int32)Min % 1440; const int32 H = M / 60; const int32 MM = M % 60;
	return FString::Printf(TEXT("%d:%02d %s"), (H % 12 == 0 ? 12 : H % 12), MM, H < 12 ? TEXT("AM") : TEXT("PM"));
}

int32 FBBState::Allies() const { int32 C = 0; for (const FString& P : People()) if (Rel(P) >= 12) C++; return C; }

void FBBState::AddRel(const FString& Npc, int32 N)
{
	if (N > 0 && Has(TEXT("say"))) N += 1;
	V.Add(TEXT("rel_") + Npc, FMath::Clamp(Rel(Npc) + N, -20, 60));
	if (N != 0 && OnToast) OnToast(NameOf(Npc) + (N > 0 ? TEXT(" warms to you") : TEXT(" pulls back")), N > 0 ? 1 : -1);
}

void FBBState::Apply(const FString& Effects)
{
	if (Effects.IsEmpty()) return;
	FString Norm = Effects; Norm.ReplaceInline(TEXT(","), TEXT(" "));
	TArray<FString> Toks; Norm.ParseIntoArray(Toks, TEXT(" "), true);
	for (const FString& T : Toks)
	{
		if (T.StartsWith(TEXT("flag:"))) Flags.Add(T.Mid(5));
		else if (T.StartsWith(TEXT("!flag:"))) Flags.Remove(T.Mid(6));
		else if (T.StartsWith(TEXT("lesson:"))) { const FString Id = T.Mid(7); if (!Lessons.Contains(Id)) { Lessons.Add(Id); if (OnLesson) OnLesson(Id); } }
		else if (T.StartsWith(TEXT("give:"))) { const FString Id = T.Mid(5); if (!Pages.Contains(Id)) { Pages.Add(Id); if (OnToast) OnToast(TEXT("Logbook page found"), 1); } }
		else if (T.StartsWith(TEXT("event:"))) { if (OnEvent) OnEvent(T.Mid(6)); }
		else if (T.StartsWith(TEXT("own:"))) Owned.Add(T.Mid(4));
		else if (T.StartsWith(TEXT("scene:"))) { if (OnScene) OnScene(T.Mid(6)); }
		else if (T.StartsWith(TEXT("time+"))) Min += (float)FCString::Atoi(*T.Mid(5));
		else
		{
			int32 I = INDEX_NONE;
			for (int32 k = 1; k < T.Len(); ++k) if (T[k] == TEXT('+') || T[k] == TEXT('-')) { I = k; break; }
			if (I <= 0) continue;
			const FString Name = T.Left(I); const int32 N = FCString::Atoi(*T.Mid(I));
			if (Name == TEXT("cash")) { Cash += N; if (OnToast) OnToast(FString::Printf(TEXT("%s$%d"), N >= 0 ? TEXT("+") : TEXT("-"), FMath::Abs(N)), N >= 0 ? 1 : -1); }
			else if (Name == TEXT("score")) Score += N;
			else AddRel(Name, N);
		}
	}
}

bool FBBState::Check(const FString& Cond) const
{
	if (Cond.IsEmpty()) return true;
	TArray<FString> Parts; Cond.ParseIntoArray(Parts, TEXT(","), true);
	for (FString T : Parts)
	{
		T.TrimStartAndEndInline(); if (T.IsEmpty()) continue;
		bool Ok = false;
		if (T.StartsWith(TEXT("!"))) Ok = !Check(T.Mid(1));
		else if (T.StartsWith(TEXT("flag:"))) Ok = Flags.Contains(T.Mid(5));
		else if (T.StartsWith(TEXT("lesson:"))) Ok = Lessons.Contains(T.Mid(7));
		else if (T.StartsWith(TEXT("done:"))) Ok = Done.Contains(T.Mid(5));
		else if (T.StartsWith(TEXT("missed:"))) Ok = Missed.Contains(T.Mid(7));
		else if (T.StartsWith(TEXT("upg:"))) Ok = Upg.Contains(T.Mid(4));
		else if (T.StartsWith(TEXT("own:"))) Ok = Owned.Contains(T.Mid(4));
		else
		{
			const TCHAR* Ops[] = { TEXT(">="), TEXT("<="), TEXT("=="), TEXT(">"), TEXT("<") };
			FString Op; int32 Idx = INDEX_NONE;
			for (const TCHAR* O : Ops) { Idx = T.Find(O); if (Idx != INDEX_NONE) { Op = O; break; } }
			if (Idx == INDEX_NONE) Ok = Flags.Contains(T);
			else
			{
				FString Name = T.Left(Idx); Name.TrimStartAndEndInline(); FString Rhs = T.Mid(Idx + Op.Len()); Rhs.TrimStartAndEndInline();
				const int32 N = FCString::Atoi(*Rhs);
				const int32 Val = Name == TEXT("cash") ? Cash : Name == TEXT("score") ? Score : Name == TEXT("pages") ? Pages.Num() : Name == TEXT("lessons") ? Lessons.Num()
					: Name == TEXT("allies") ? Allies() : Name == TEXT("day") ? Day : (People().Contains(Name) ? Rel(Name) : Get(Name));
				Ok = Op == TEXT(">=") ? Val >= N : Op == TEXT("<=") ? Val <= N : Op == TEXT(">") ? Val > N : Op == TEXT("<") ? Val < N : Val == N;
			}
		}
		if (!Ok) return false;
	}
	return true;
}

FString FBBState::Serialize() const
{
	FString S;
	S += FString::Printf(TEXT("day=%d\nmin=%.1f\ncash=%d\nscore=%d\nfuel=%.3f\nending=%s\nintro=%d\nfares=%d\ntips=%d\nriders=%d\ndsc=%d\n"),
		Day, Min, Cash, Score, Fuel, *Ending, bIntroSeen ? 1 : 0, Fares, Tips, Riders, DayStartCash);
	for (const TPair<FString, int32>& KV : V) S += FString::Printf(TEXT("v:%s=%d\n"), *KV.Key, KV.Value);
	auto Dump = [&S](const TCHAR* Tag, const TSet<FString>& Set) { for (const FString& X : Set) S += FString::Printf(TEXT("%s:%s\n"), Tag, *X); };
	Dump(TEXT("f"), Flags); Dump(TEXT("l"), Lessons); Dump(TEXT("p"), Pages); Dump(TEXT("d"), Done); Dump(TEXT("m"), Missed); Dump(TEXT("o"), Owned); Dump(TEXT("u"), Upg);
	return S;
}

FBBState FBBState::Parse(const FString& Txt)
{
	FBBState St; St.Owned.Empty();
	TArray<FString> Lines; Txt.ParseIntoArrayLines(Lines, true);
	for (const FString& L : Lines)
	{
		if (L.Len() < 3) continue;
		if (L[1] == TEXT(':') && L[0] != TEXT('v'))
		{
			const FString Val = L.Mid(2);
			switch (L[0]) { case 'f': St.Flags.Add(Val); break; case 'l': St.Lessons.Add(Val); break; case 'p': St.Pages.Add(Val); break; case 'd': St.Done.Add(Val); break;
				case 'm': St.Missed.Add(Val); break; case 'o': St.Owned.Add(Val); break; case 'u': St.Upg.Add(Val); break; default: break; }
			continue;
		}
		int32 Eq = INDEX_NONE; if (!L.FindChar(TEXT('='), Eq)) continue;
		const FString K = L.Left(Eq); const FString Vv = L.Mid(Eq + 1).TrimStartAndEnd();
		if (K.StartsWith(TEXT("v:"))) { St.V.Add(K.Mid(2), FCString::Atoi(*Vv)); continue; }
		if (K == TEXT("day")) St.Day = FCString::Atoi(*Vv); else if (K == TEXT("min")) St.Min = FCString::Atof(*Vv);
		else if (K == TEXT("cash")) St.Cash = FCString::Atoi(*Vv); else if (K == TEXT("score")) St.Score = FCString::Atoi(*Vv);
		else if (K == TEXT("fuel")) St.Fuel = FCString::Atof(*Vv); else if (K == TEXT("ending")) St.Ending = Vv;
		else if (K == TEXT("intro")) St.bIntroSeen = Vv == TEXT("1"); else if (K == TEXT("fares")) St.Fares = FCString::Atoi(*Vv);
		else if (K == TEXT("tips")) St.Tips = FCString::Atoi(*Vv); else if (K == TEXT("riders")) St.Riders = FCString::Atoi(*Vv);
		else if (K == TEXT("dsc")) St.DayStartCash = FCString::Atoi(*Vv);
	}
	St.Owned.Add(TEXT("bus"));
	return St;
}

FString FBBState::SavePath() { return FPaths::ProjectSavedDir() / TEXT("bounceback.sav"); }
bool FBBState::HasSave() { return IFileManager::Get().FileExists(*SavePath()); }
void FBBState::Save() const { FFileHelper::SaveStringToFile(Serialize(), *SavePath()); }
FBBState FBBState::Load() { FString T; FFileHelper::LoadFileToString(T, *SavePath()); return Parse(T); }

// ============================================================================ Scripts
void FBBScripts::Load()
{
	if (bLoaded) return;
	bLoaded = true;
	const FString Dir = FPaths::ProjectContentDir() / TEXT("Story");
	TArray<FString> Files; IFileManager::Get().FindFiles(Files, *(Dir / TEXT("*.txt")), true, false);
	Files.Sort();
	for (const FString& F : Files)
	{
		FString Txt; if (!FFileHelper::LoadFileToString(Txt, *(Dir / F))) continue;
		TArray<FString> Lines; Txt.ParseIntoArrayLines(Lines, false);
		const FString N = FPaths::GetBaseFilename(F).ToLower();
		if (N == TEXT("beats"))
		{
			for (const FString& Raw : Lines)
			{
				const FString L = Raw.TrimStartAndEnd(); if (L.IsEmpty() || L[0] == TEXT('#')) continue;
				TArray<FString> P; L.ParseIntoArray(P, TEXT("|"), false); for (FString& X : P) X.TrimStartAndEndInline();
				if (P.Num() >= 8) BeatRows.Add(P);
			}
		}
		else if (N == TEXT("chatter"))
		{
			FString Key;
			for (const FString& Raw : Lines)
			{
				const FString L = Raw.TrimStartAndEnd(); if (L.IsEmpty() || L[0] == TEXT('#')) continue;
				if (L[0] == TEXT('@')) { Key = L.Mid(1).TrimStartAndEnd(); Chatter.FindOrAdd(Key).Empty(); continue; }
				if (!Key.IsEmpty()) Chatter.FindOrAdd(Key).Add(L);
			}
		}
		else if (N == TEXT("pages")) ParseKeyed(Lines, Pages);
		else if (N == TEXT("epilogues")) ParseKeyed(Lines, Epilogues);
		else ParseScenes(Lines);
	}
	UE_LOG(LogTemp, Log, TEXT("BounceBack: %d scenes, %d beats, %d pages"), Scenes.Num(), BeatRows.Num(), Pages.Num());
}

void FBBScripts::ParseKeyed(const TArray<FString>& Lines, TMap<FString, FString>& Into)
{
	FString Key, Body;
	for (const FString& Raw : Lines)
	{
		FString L = Raw; L.TrimEndInline();
		if (L.StartsWith(TEXT("@"))) { if (!Key.IsEmpty()) Into.Add(Key, Body.TrimStartAndEnd()); Key = L.Mid(1).TrimStartAndEnd(); Body.Empty(); continue; }
		if (!Key.IsEmpty()) Body += L + TEXT("\n");
	}
	if (!Key.IsEmpty()) Into.Add(Key, Body.TrimStartAndEnd());
}

void FBBScripts::ParseScenes(const TArray<FString>& Lines)
{
	FBBScene* Cur = nullptr; int32 GrpNode = INDEX_NONE;
	for (const FString& Raw : Lines)
	{
		FString L = Raw.TrimStartAndEnd(); if (L.IsEmpty() || L[0] == TEXT('#')) continue;
		if (L[0] == TEXT('@')) { const FString Id = L.Mid(1).TrimStartAndEnd(); Cur = &Scenes.FindOrAdd(Id); Cur->Id = Id; Cur->Nodes.Empty(); Cur->Labels.Empty(); GrpNode = INDEX_NONE; continue; }
		if (!Cur) continue;
		if (L[0] == TEXT(':')) { Cur->Labels.Add(L.Mid(1).TrimStartAndEnd(), Cur->Nodes.Num()); GrpNode = INDEX_NONE; continue; }
		if (L[0] == TEXT('?'))
		{
			TArray<FString> P; L.Mid(1).ParseIntoArray(P, TEXT("|"), false); for (FString& X : P) X.TrimStartAndEndInline();
			FBBChoice C; C.Text = P.Num() > 0 ? P[0] : FString(); C.Effects = P.Num() > 1 ? P[1] : FString(); C.Goto = P.Num() > 2 ? P[2] : FString(); C.Cond = P.Num() > 3 ? P[3] : FString();
			if (GrpNode == INDEX_NONE) { FBBNode N; N.Kind = 2; GrpNode = Cur->Nodes.Add(N); }
			Cur->Nodes[GrpNode].Choices.Add(C); continue;
		}
		GrpNode = INDEX_NONE;
		FString Cond;
		if (L[0] == TEXT('[')) { int32 E = INDEX_NONE; if (L.FindChar(TEXT(']'), E) && E > 0) { Cond = L.Mid(1, E - 1); L = L.Mid(E + 1).TrimStartAndEnd(); } }
		FBBNode N; N.Cond = Cond;
		if (L.StartsWith(TEXT(">"))) { N.Kind = 1; N.Effects = L.Mid(1).TrimStartAndEnd(); }
		else if (L.StartsWith(TEXT("goto "))) { N.Kind = 3; N.Label = L.Mid(5).TrimStartAndEnd(); }
		else if (L == TEXT("end")) N.Kind = 4;
		else
		{
			N.Kind = 0; N.Text = L; int32 C = INDEX_NONE;
			if (L.FindChar(TEXT(':'), C) && C > 0 && C < 24 && !L.StartsWith(TEXT("*"))) { N.Speaker = L.Left(C).TrimStartAndEnd(); N.Text = L.Mid(C + 1).TrimStartAndEnd(); }
		}
		Cur->Nodes.Add(N);
	}
}

const FBBScene* FBBScripts::Get(const FString& Id) { Load(); return Scenes.Find(Id); }

// ============================================================================ Dialogue
bool FBBDialogue::Play(const FString& SceneId, TFunction<void()> OnDone)
{
	const FBBScene* S2 = FBBScripts::Get(SceneId);
	if (!S2) { UE_LOG(LogTemp, Warning, TEXT("BounceBack: missing scene %s"), *SceneId); if (OnDone) OnDone(); return false; }
	Sc = S2; Pc = 0; Done = OnDone; bActive = true; Choices.Empty(); Text.Empty(); Typed = 0.f;
	Next(); return true;
}

void FBBDialogue::Finish()
{
	bActive = false; Choices.Empty(); Text.Empty();
	TFunction<void()> D = Done; Done = nullptr; if (D) D();
}

void FBBDialogue::Abort() { bActive = false; Choices.Empty(); Text.Empty(); Done = nullptr; Sc = nullptr; }

void FBBDialogue::Next()
{
	for (int32 Guard = 0; Guard < 5000; ++Guard)
	{
		if (!Sc || Pc >= Sc->Nodes.Num()) { Finish(); return; }
		const FBBNode& N = Sc->Nodes[Pc++];
		if (!S->Check(N.Cond)) continue;
		switch (N.Kind)
		{
		case 0:
			SpeakerId = N.Speaker.ToLower(); Speaker = N.Speaker.IsEmpty() ? FString() : FBBState::NameOf(N.Speaker); bNarr = N.Speaker.IsEmpty();
			Text = N.Text; Typed = 0.f; Choices.Empty(); return;
		case 1: S->Apply(N.Effects); continue;
		case 2:
		{
			TArray<FBBChoice> Vis; for (const FBBChoice& C : N.Choices) if (S->Check(C.Cond)) Vis.Add(C);
			if (Vis.Num() == 0) continue;
			Choices = Vis; Text.Empty(); return;
		}
		case 3: { const int32* I = Sc->Labels.Find(N.Label); if (I) { Pc = *I; continue; } Finish(); return; }
		case 4: Finish(); return;
		default: break;
		}
	}
	Finish();
}

void FBBDialogue::Advance() { if (bActive && Choices.Num() == 0) Next(); }

void FBBDialogue::Pick(int32 Index)
{
	if (!Choices.IsValidIndex(Index)) return;
	const FBBChoice C = Choices[Index]; Choices.Empty();
	if (!C.Effects.IsEmpty()) S->Apply(C.Effects);
	if (!C.Goto.IsEmpty()) { const int32* I = Sc ? Sc->Labels.Find(C.Goto) : nullptr; if (I) Pc = *I; else { Finish(); return; } }
	Next();
}
