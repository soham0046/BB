#include "BBTown.h"
#include "BBLayout.h"
#include "BBUtil.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"

ABBTown::ABBTown()
{
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetupAttachment(Root);
	Sun->SetAtmosphereSunLight(true);
	USkyAtmosphereComponent* Atmo = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("Atmosphere"));
	Atmo->SetupAttachment(Root);
	USkyLightComponent* Sky = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	Sky->SetupAttachment(Root);
	Sky->bRealTimeCapture = true;
}

void ABBTown::Build()
{
	USceneComponent* Root = GetRootComponent();

	// ground (a big flat box)
	BBU::Part(this, Root, TEXT("Cube"), FVector(0, 0, -50), FVector(1500, 1500, 1), BBU::Hex(0x6b7a45), FRotator::ZeroRotator, true);

	// ring road: lots of short flat boxes in a circle
	int Segments = 64;
	for (int i = 0; i < Segments; i++)
	{
		FVector2D A = BBL::Polar(i * 360.f / Segments, BBL::R);
		FVector2D B = BBL::Polar((i + 1) * 360.f / Segments, BBL::R);
		FVector2D Mid = (A + B) * 0.5f;
		FVector2D D = B - A;
		float Yaw = FMath::RadiansToDegrees(FMath::Atan2(D.Y, D.X));
		BBU::Part(this, Root, TEXT("Cube"), BBL::ToWorld(Mid, 0.05f), FVector(D.Size() + 0.6f, BBL::RW, 0.1f), BBU::Hex(0x3a3c40), FRotator(0, Yaw, 0));
	}
	// two straight roads across the middle
	BBU::Part(this, Root, TEXT("Cube"), BBL::ToWorld(0, 0, 0.06f), FVector(BBL::RW, BBL::R * 2, 0.1f), BBU::Hex(0x3a3c40));
	BBU::Part(this, Root, TEXT("Cube"), BBL::ToWorld(16, 0, 0.07f), FVector(BBL::R * 2 + 32, BBL::RW, 0.1f), BBU::Hex(0x3a3c40));

	// the red ball in the middle of the square
	BBU::Part(this, Root, TEXT("Sphere"), BBL::ToWorld(0, 0, 1.5f), FVector(1.5f, 1.5f, 1.5f), BBU::Hex(0xd8352a));

	// buildings: a box for the walls and a flatter box for the roof
	for (const BBL::FBuilding& B : BBL::Buildings())
	{
		float Yaw = BBL::RotToYaw(B.Rot);
		FVector Pos = BBL::ToWorld(B.X, B.Z);
		BBU::Part(this, Root, TEXT("Cube"), Pos + FVector(0, 0, B.H * 50), FVector(B.D, B.W, B.H), B.Wall, FRotator(0, Yaw, 0), true);
		BBU::Part(this, Root, TEXT("Cube"), Pos + FVector(0, 0, B.H * 100 + 20), FVector(B.D + 0.8f, B.W + 0.8f, 0.4f), B.Roof, FRotator(0, Yaw, 0));
	}

	// bus stops: a pole with a blue sign
	for (const BBL::FStop& S : BBL::Stops())
	{
		FVector Pos = BBL::ToWorld(S.X, S.Z);
		BBU::Part(this, Root, TEXT("Cylinder"), Pos + FVector(0, 0, 130), FVector(0.1f, 0.1f, 2.6f), BBU::Hex(0x555a62));
		BBU::Part(this, Root, TEXT("Cube"), Pos + FVector(0, 0, 270), FVector(0.1f, 0.9f, 0.5f), BBU::Hex(0x2f9aa8));
	}

	// a few trees
	FRandomStream Rand(42);
	for (int i = 0; i < 150; i++)
	{
		FVector2D P = BBL::Polar(Rand.FRand() * 360.f, 20.f + Rand.FRand() * 250.f);
		if (BBL::DistToRoad(P.X, P.Y) < 10.f) continue;
		if (FVector2D::Distance(P, BBL::LakeC()) < BBL::LakeR + 5.f) continue;
		BBU::Part(this, Root, TEXT("Cylinder"), BBL::ToWorld(P, 1.5f), FVector(0.5f, 0.5f, 3.f), BBU::Hex(0x5a4030));
		BBU::Part(this, Root, TEXT("Sphere"), BBL::ToWorld(P, 4.5f), FVector(3.5f, 3.5f, 4.f), BBU::Hex(0x4f7a38));
	}

	// the lake
	BBU::Part(this, Root, TEXT("Cylinder"), BBL::ToWorld(BBL::LakeC(), 0.04f), FVector(BBL::LakeR * 2, BBL::LakeR * 2, 0.08f), BBU::Hex(0x2f6f8f));
	SetHour(8.f);
}

void ABBTown::SetHour(float Hour)
{
	// sun goes up at 6 and down at 19 (very rough)
	float T = FMath::Clamp((Hour - 6.f) / 13.f, 0.f, 1.f);
	float Height = FMath::Sin(T * PI) * 60.f;
	Sun->SetWorldRotation(FRotator(-Height - 2.f, 180.f * T, 0));
	Sun->SetIntensity(Hour > 6.f && Hour < 19.f ? 10.f : 0.5f);
}
