#pragma once
#include "CoreMinimal.h"

// Town layout in METERS (web/Unity-compatible): ring road R=120 + N-S and E-W spokes. World cm = meters * 100, UE X = layout x, UE Y = layout z.
namespace BBL
{
	constexpr float R = 120.f, RW = 10.f;
	inline FVector2D Polar(float Deg, float Rad) { const float A = FMath::DegreesToRadians(Deg); return FVector2D(FMath::Cos(A) * Rad, FMath::Sin(A) * Rad); }
	inline FVector ToWorld(float X, float Z, float H = 0.f) { return FVector(X * 100.f, Z * 100.f, H * 100.f); }
	inline FVector ToWorld(const FVector2D& P, float H = 0.f) { return ToWorld(P.X, P.Y, H); }
	inline float DistToRoad(float X, float Z)
	{
		float D = FMath::Abs(FMath::Sqrt(X * X + Z * Z) - R);
		if (FMath::Abs(Z) <= R + 2.f) D = FMath::Min(D, FMath::Abs(X));
		if (X >= -R - 2.f && X <= 152.f) D = FMath::Min(D, FMath::Abs(Z));
		return D;
	}
	inline bool OnRoad(float X, float Z) { return DistToRoad(X, Z) < RW * 0.5f + 0.5f; }
	inline FVector2D LakeC() { return FVector2D(192.f, 0.f); }
	constexpr float LakeR = 46.f;

	struct FBuilding { FString Id, Kind; float X, Z, Rot, W, D, H; FColor Wall, Roof; };
	struct FStop { FString Id, Name; float X, Z, Yaw; };

	// yaw (UE degrees) from a layout 'rot' (three.js style: front vector = (sin rot, cos rot) in x,z)
	inline float RotToYaw(float Rot) { return FMath::RadiansToDegrees(FMath::Atan2(FMath::Cos(Rot), FMath::Sin(Rot))); }

	inline FBuilding Bld(const TCHAR* Id, const TCHAR* Kind, float Deg, float Rad, float W, float D, float H, uint32 Wall, uint32 Roof)
	{
		const FVector2D P = Polar(Deg, Rad); const float Rot = (float)FMath::Atan2(-P.X, -P.Y);
		return { Id, Kind, (float)P.X, (float)P.Y, Rot, W, D, H, FColor((uint8)((Wall >> 16) & 255), (uint8)((Wall >> 8) & 255), (uint8)(Wall & 255)), FColor((uint8)((Roof >> 16) & 255), (uint8)((Roof >> 8) & 255), (uint8)(Roof & 255)) };
	}
	inline FBuilding BldAt(const TCHAR* Id, const TCHAR* Kind, float X, float Z, float Rot, float W, float D, float H, uint32 Wall, uint32 Roof)
	{
		return { Id, Kind, X, Z, Rot, W, D, H, FColor((uint8)((Wall >> 16) & 255), (uint8)((Wall >> 8) & 255), (uint8)(Wall & 255)), FColor((uint8)((Roof >> 16) & 255), (uint8)((Roof >> 8) & 255), (uint8)(Roof & 255)) };
	}

	inline const TArray<FBuilding>& Buildings()
	{
		static TArray<FBuilding> B;
		if (B.Num() == 0)
		{
			B.Add(Bld(TEXT("depot"), TEXT("depot"), 100, 138, 26, 16, 7.5f, 0xc9b99a, 0x8a3d2e));
			B.Add(Bld(TEXT("yard"), TEXT("yard"), 55, 140, 10, 7, 3.6f, 0xb27a4a, 0x555a60));
			B.Add(Bld(TEXT("mill"), TEXT("mill"), 20, 142, 12, 12, 13, 0xa08a70, 0x5a4636));
			B.Add(Bld(TEXT("farmhouse"), TEXT("house"), 242, 146, 12, 9, 6, 0xe4d6b8, 0x7a3b2b));
			B.Add(Bld(TEXT("barn"), TEXT("barn"), 252, 160, 16, 10, 8, 0x9c3a2c, 0x4a4a50));
			B.Add(Bld(TEXT("ruth_home"), TEXT("house"), 208, 102, 9, 8, 5.5f, 0xcfc3e4, 0x6b4d6e));
			B.Add(Bld(TEXT("church"), TEXT("church"), 285, 102, 10, 18, 8, 0xf0ead8, 0x6a5a4a));
			B.Add(Bld(TEXT("school"), TEXT("school"), 330, 102, 24, 11, 6, 0xc67a52, 0x4f5a6a));
			B.Add(BldAt(TEXT("townhall"), TEXT("townhall"), 0, -30, 0, 22, 13, 9, 0xe8e0d0, 0x4a5a6a));
			B.Add(BldAt(TEXT("diner"), TEXT("diner"), -30, 8, PI / 2, 14, 9, 4.6f, 0xd9544a, 0x3a3a42));
			B.Add(BldAt(TEXT("store"), TEXT("store"), 30, 10, -PI / 2, 14, 10, 5.2f, 0x4f8a8a, 0x3a3a42));
			B.Add(BldAt(TEXT("pharmacy"), TEXT("store"), 22, -18, -PI / 2, 10, 8, 5, 0xe6e6ee, 0x3a4a5a));
		}
		return B;
	}

	inline const TMap<FString, FVector2D>& Places()
	{
		static TMap<FString, FVector2D> P;
		if (P.Num() == 0)
		{
			for (const FBuilding& B : Buildings())
				P.Add(B.Id, FVector2D(B.X + FMath::Sin(B.Rot) * (B.D / 2 + 3), B.Z + FMath::Cos(B.Rot) * (B.D / 2 + 3)));
			P.Add(TEXT("plaza"), FVector2D(0, 0)); P.Add(TEXT("dock"), FVector2D(146, 6));
			P.Add(TEXT("shed"), Polar(100, 160)); P.Add(TEXT("gate"), Polar(180, 128)); P.Add(TEXT("cabin2"), Polar(300, 150));
			P.Add(TEXT("bridge"), Polar(170, 150)); P.Add(TEXT("silo"), Polar(238, 168)); P.Add(TEXT("house_e5"), Polar(140, 100));
		}
		return P;
	}

	inline FStop MakeStop(const TCHAR* Id, const TCHAR* Name, float Deg, float Rad = 112.f)
	{
		const FVector2D P = Polar(Deg, Rad); return { Id, Name, (float)P.X, (float)P.Y, (float)FMath::RadiansToDegrees(FMath::Atan2(P.Y, P.X)) };   // UE yaw: outward radial
	}
	inline const TArray<FStop>& Stops()
	{
		static TArray<FStop> S;
		if (S.Num() == 0)
		{
			S.Add(MakeStop(TEXT("stop_depot"), TEXT("Depot"), 108)); S.Add(MakeStop(TEXT("stop_maple"), TEXT("Maple St"), 150));
			S.Add(MakeStop(TEXT("stop_orchard"), TEXT("Orchard Row"), 200)); S.Add(MakeStop(TEXT("stop_farm"), TEXT("Orchard Farm"), 240));
			S.Add(MakeStop(TEXT("stop_church"), TEXT("Church"), 285)); S.Add(MakeStop(TEXT("stop_lake"), TEXT("Lake"), 356));
			S.Add(MakeStop(TEXT("stop_mill"), TEXT("Mill"), 28));
			S.Add({ TEXT("stop_square"), TEXT("The Square"), -8.f, 14.f, 90.f });
		}
		return S;
	}
	inline const FStop* FindStop(const FString& Id) { for (const FStop& S : Stops()) if (S.Id == Id) return &S; return nullptr; }
	inline FString StopName(const FString& Id) { const FStop* S = FindStop(Id); return S ? S->Name : Id; }

	// Where a place is, in world cm. Stops/places fall back to the plaza.
	inline FVector PlaceWorld(const FString& Id)
	{
		if (const FStop* S = FindStop(Id)) return ToWorld(S->X, S->Z);
		if (const FVector2D* P = Places().Find(Id)) return ToWorld(*P);
		return ToWorld(0, 0);
	}
	inline FVector2D PlaceXZ(const FString& Id) { const FVector W = PlaceWorld(Id); return FVector2D(W.X / 100.f, W.Y / 100.f); }

	// depot-relative anchors (bed/desk/pump/spawn/bus) in the depot's local frame
	inline FVector DepotAt(float Lx, float Lz)
	{
		const FBuilding* D = nullptr; for (const FBuilding& B : Buildings()) if (B.Id == TEXT("depot")) D = &B;
		const float Sn = FMath::Sin(D->Rot), Cs = FMath::Cos(D->Rot);
		const float Ox = D->X + Sn * (D->D / 2 + 3.f), Oz = D->Z + Cs * (D->D / 2 + 3.f);
		return ToWorld(Ox + Cs * Lx + Sn * Lz, Oz - Sn * Lx + Cs * Lz);
	}
}
