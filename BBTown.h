#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBTown.generated.h"

class UDirectionalLightComponent;

// The town of Pelham Hollow, made from boxes.
UCLASS()
class ABBTown : public AActor
{
	GENERATED_BODY()
public:
	ABBTown();
	void Build();
	void SetHour(float Hour);   // moves the sun
	UPROPERTY() UDirectionalLightComponent* Sun;
};
