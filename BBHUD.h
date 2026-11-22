#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BBHUD.generated.h"

// Draws all the text and boxes on screen.
UCLASS()
class ABBHUD : public AHUD
{
	GENERATED_BODY()
public:
	virtual void DrawHUD() override;
private:
	void Text(const FString& T, float X, float Y, FLinearColor Color, float Scale = 1.f);
	void Box(float X, float Y, float W, float H);
	TArray<FString> Wrap(const FString& T, float MaxWidth, float Scale);
};
