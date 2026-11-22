#include "BBBus.h"
#include "BBUtil.h"
#include "Components/BoxComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"

ABBBus::ABBBus()
{
	PrimaryActorTick.bCanEverTick = true;
	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->SetBoxExtent(FVector(520, 125, 100));
	Box->SetCollisionProfileName(TEXT("BlockAll"));
	SetRootComponent(Box);
	Body = CreateDefaultSubobject<USceneComponent>(TEXT("Body"));
	Body->SetupAttachment(Box);
	// camera follows behind the bus
	Arm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Arm"));
	Arm->SetupAttachment(Box);
	Arm->TargetArmLength = 1700.f;
	Arm->SetRelativeRotation(FRotator(-15, 0, 0));
	Arm->bDoCollisionTest = false;
	Arm->bEnableCameraLag = true;
	Cam = CreateDefaultSubobject<UCameraComponent>(TEXT("Cam"));
	Cam->SetupAttachment(Arm);
}

void ABBBus::BeginPlay()
{
	Super::BeginPlay();
	// build the bus out of boxes (the engine has a built in Cube shape)
	BBU::Part(this, Body, TEXT("Cube"), FVector(0, 0, 0), FVector(10.4f, 2.5f, 1.6f), BBU::Hex(0x2f7f86));      // main body
	BBU::Part(this, Body, TEXT("Cube"), FVector(0, 0, 100), FVector(10.4f, 2.5f, 0.15f), BBU::Hex(0xe9e1cc));   // roof
	BBU::Part(this, Body, TEXT("Cube"), FVector(520, 0, 40), FVector(0.1f, 2.3f, 0.8f), BBU::Hex(0x9fc7d4));    // windscreen
	for (int i = 0; i < 4; i++)  // wheels
	{
		float X = (i < 2) ? 340.f : -340.f;
		float Y = (i % 2 == 0) ? -125.f : 125.f;
		BBU::Part(this, Body, TEXT("Cylinder"), FVector(X, Y, -80), FVector(1.0f, 1.0f, 0.3f), BBU::Hex(0x1b1b1d), FRotator(0, 0, 90));
	}
}

void ABBBus::Drive(float Dt, float Throttle, float Steer)
{
	// speed
	if (Throttle != 0.f) Speed += Throttle * 600.f * Dt;
	else Speed -= FMath::Sign(Speed) * FMath::Min(FMath::Abs(Speed), 350.f * Dt);   // slow down by itself
	Speed = FMath::Clamp(Speed, -500.f, 1800.f);

	// turning (only when moving)
	float Turn = Steer * (Speed / 1800.f) * 70.f * Dt;
	AddActorWorldRotation(FRotator(0, Turn, 0));

	// move, and stop if we hit a building
	FHitResult Hit;
	AddActorWorldOffset(GetActorForwardVector() * Speed * Dt, true, &Hit);
	if (Hit.bBlockingHit) Speed = -Speed * 0.3f;

	// bouncy body
	Bounce += Dt * (6.f + FMath::Abs(Speed) / 200.f);
	Body->SetRelativeLocation(FVector(0, 0, FMath::Sin(Bounce) * FMath::Abs(Speed) / 400.f));
}
