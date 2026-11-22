#include "BBHUD.h"
#include "BBGameMode.h"
#include "BBBus.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Engine/Engine.h"

void ABBHUD::Text(const FString& T, float X, float Y, FLinearColor Color, float Scale)
{
	UFont* Font = GEngine->GetMediumFont();
	DrawText(T, FLinearColor(0, 0, 0, 0.8f), X + 2, Y + 2, Font, Scale * 1.3f, false);   // shadow
	DrawText(T, Color, X, Y, Font, Scale * 1.3f, false);
}

void ABBHUD::Box(float X, float Y, float W, float H) { DrawRect(FLinearColor(0.03f, 0.04f, 0.06f, 0.8f), X, Y, W, H); }

// splits a long line into several lines that fit the box
TArray<FString> ABBHUD::Wrap(const FString& T, float MaxWidth, float Scale)
{
	TArray<FString> Lines, Words;
	T.ParseIntoArray(Words, TEXT(" "), true);
	FString Line;
	for (const FString& Word : Words)
	{
		FString Try = Line.IsEmpty() ? Word : Line + TEXT(" ") + Word;
		float W = 0, H = 0;
		GetTextSize(Try, W, H, GEngine->GetMediumFont(), Scale * 1.3f);
		if (W > MaxWidth && !Line.IsEmpty()) { Lines.Add(Line); Line = Word; }
		else Line = Try;
	}
	Lines.Add(Line);
	return Lines;
}

void ABBHUD::DrawHUD()
{
	Super::DrawHUD();
	ABBGameMode* GM = Cast<ABBGameMode>(GetWorld()->GetAuthGameMode());
	if (!GM || !Canvas) return;
	float W = Canvas->SizeX, H = Canvas->SizeY;
	FLinearColor White(1, 1, 1, 1), Gold(1, 0.78f, 0.3f, 1), Grey(0.8f, 0.82f, 0.86f, 1);

	// ---- title
	if (GM->Screen == EScreen::Title)
	{
		DrawRect(FLinearColor(0, 0, 0, 0.5f), 0, 0, W, H);
		Text(TEXT("BOUNCE BACK"), W * 0.38f, H * 0.2f, Gold, 3.f);
		Text(TEXT("EVERY FALL HAS A COMEBACK"), W * 0.37f, H * 0.35f, White, 1.2f);
		Text(TEXT("Enter - new game"), W * 0.42f, H * 0.55f, White);
		Text(TEXT("C - continue"), W * 0.42f, H * 0.6f, White);
		Text(TEXT("W/S = gas and brake, A/D = steer, E = pick up passengers, Space = handbrake"), W * 0.2f, H * 0.85f, Grey, 0.8f);
		return;
	}

	// ---- ending
	if (GM->Screen == EScreen::Ending)
	{
		DrawRect(FLinearColor(0, 0, 0, 0.7f), 0, 0, W, H);
		Text(GM->EndTitle, W * 0.35f, H * 0.12f, Gold, 2.f);
		float Y = H * 0.28f;
		for (const FString& L : Wrap(GM->EndBody, W * 0.6f, 1.f)) { Text(L, W * 0.2f, Y, White); Y += 34; }
		Text(TEXT("Press Enter"), W * 0.44f, H * 0.9f, Grey, 0.8f);
		return;
	}

	// ---- summary
	if (GM->Screen == EScreen::Summary)
	{
		DrawRect(FLinearColor(0, 0, 0, 0.6f), 0, 0, W, H);
		Text(FString::Printf(TEXT("DAY %d COMPLETE"), GM->S.Day), W * 0.38f, H * 0.2f, Gold, 2.f);
		Text(FString::Printf(TEXT("Passengers today: %d"), GM->TodayRiders), W * 0.38f, H * 0.4f, White);
		Text(FString::Printf(TEXT("Money earned today: $%d"), GM->TodayMoney), W * 0.38f, H * 0.46f, White);
		Text(FString::Printf(TEXT("Cash: $%d"), GM->S.Cash), W * 0.38f, H * 0.52f, White);
		Text(TEXT("Press Enter to sleep"), W * 0.4f, H * 0.8f, Gold);
		return;
	}

	// ---- top bar
	Box(10, 10, 330, 70);
	Text(GM->S.Clock(), 20, 14, White, 1.3f);
	Text(FString::Printf(TEXT("%s, day %d of 18"), *GM->S.DayName(), GM->S.Day), 20, 50, Grey, 0.8f);
	Box(W - 240, 10, 230, 70);
	Text(FString::Printf(TEXT("$%d"), GM->S.Cash), W - 225, 14, GM->S.Cash < 10 ? FLinearColor(1, 0.4f, 0.3f) : FLinearColor(0.5f, 1, 0.6f), 1.3f);
	Text(FString::Printf(TEXT("Score %d  Pages %d/24"), GM->S.Score, GM->S.Pages.Num()), W - 225, 50, Grey, 0.8f);

	// objective
	if (!GM->Objective.IsEmpty()) Text(GM->Objective, W * 0.5f - 250, 20, Gold, 0.9f);

	// speed
	if (GM->Bus) Text(FString::Printf(TEXT("%d km/h"), (int32)FMath::Abs(GM->Bus->Speed * 0.036f)), 20, H - 60, White, 1.3f);

	// prompt and messages
	if (!GM->Prompt.IsEmpty() && !GM->Dlg.bActive) Text(GM->Prompt, W * 0.3f, H - 130, White);
	if (GM->MessageTime > 0.f) Text(GM->Message, W * 0.4f, 100, Gold);

	// ---- conversation box
	if (GM->Dlg.bActive)
	{
		float Bw = W * 0.8f, Bx = W * 0.1f, By = H * 0.65f, Bh = H * 0.3f;
		Box(Bx, By, Bw, Bh);
		if (GM->Dlg.Choices.Num() > 0)
		{
			for (int32 i = 0; i < GM->Dlg.Choices.Num(); i++)
			{
				FString Line = FString::Printf(TEXT("%d. %s"), i + 1, *GM->Dlg.Choices[i].Text);
				Text(Line, Bx + 20, By + 15 + i * 40, i == GM->Choice ? Gold : White, 0.9f);
			}
		}
		else
		{
			if (!GM->Dlg.Speaker.IsEmpty()) Text(GM->Dlg.Speaker, Bx + 20, By + 10, Gold, 1.1f);
			FString Shown = GM->Dlg.Text.Left((int32)GM->Dlg.Typed);
			float Y = By + (GM->Dlg.Speaker.IsEmpty() ? 20 : 55);
			for (const FString& L : Wrap(Shown, Bw - 40, 0.9f)) { Text(L, Bx + 20, Y, White, 0.9f); Y += 32; }
			Text(TEXT("Enter"), Bx + Bw - 90, By + Bh - 40, Grey, 0.7f);
		}
	}
}
