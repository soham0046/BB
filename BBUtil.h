#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

// Tiny helpers for building actors out of the engine's built-in basic shapes (no imported assets needed).
namespace BBU
{
	inline UStaticMesh* Mesh(const TCHAR* Name)   // "Cube" "Cylinder" "Sphere" "Cone" "Plane"
	{
		return Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, *FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), Name, Name)));
	}

	inline UMaterialInstanceDynamic* Mat(UObject* Outer, const FColor& C)
	{
		UMaterialInterface* Base = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")));
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Base, Outer);
		if (M) M->SetVectorParameterValue(TEXT("Color"), FLinearColor(C));
		return M;
	}

	// Cube/Cylinder/Sphere/Cone are 100 uu; Scale is in those units (1 = 1 m). Loc in cm relative to Parent.
	inline UStaticMeshComponent* Part(AActor* Owner, USceneComponent* Parent, const TCHAR* MeshName, const FVector& Loc, const FVector& Scale, const FColor& Color,
		const FRotator& Rot = FRotator::ZeroRotator, bool bCollide = false)
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Owner);
		C->SetStaticMesh(Mesh(MeshName));
		C->RegisterComponent();
		if (Parent) C->AttachToComponent(Parent, FAttachmentTransformRules::KeepRelativeTransform);
		C->SetRelativeLocation(Loc); C->SetRelativeRotation(Rot); C->SetRelativeScale3D(Scale);
		C->SetMaterial(0, Mat(Owner, Color));
		if (bCollide) C->SetCollisionProfileName(TEXT("BlockAll")); else C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return C;
	}

	inline FColor Hex(uint32 H) { return FColor((uint8)((H >> 16) & 255), (uint8)((H >> 8) & 255), (uint8)(H & 255)); }
	inline FColor Lerp(const FColor& A, const FColor& B, float T) { return FLinearColor::LerpUsingHSV(FLinearColor(A), FLinearColor(B), T).ToFColor(true); }
}
