#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "BBBus.generated.h"

class UBoxComponent;
class USpringArmComponent;
class UCameraComponent;

// The bus. Very simple driving: speed goes up/down and the bus turns.
UCLASS()
class ABBBus : public APawn
{
	GENERATED_BODY()
public:
	ABBBus();
	virtual void BeginPlay() override;
	void Drive(float Dt, float Throttle, float Steer);

	float Speed = 0.f;      // cm per second (1800 = about 65 km/h)
	float Bounce = 0.f;     // little up/down wobble so it feels bouncy
	UPROPERTY() UBoxComponent* Box;
	UPROPERTY() USceneComponent* Body;
	UPROPERTY() USpringArmComponent* Arm;
	UPROPERTY() UCameraComponent* Cam;
};
