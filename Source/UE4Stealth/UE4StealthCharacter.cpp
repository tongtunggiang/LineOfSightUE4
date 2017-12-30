// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "UE4StealthCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Materials/Material.h"
#include "ProceduralMeshComponent.h"

AUE4StealthCharacter::AUE4StealthCharacter()
{
	// Set size for player capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate character to camera direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Rotate character to moving direction
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	// Create a camera boom...
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->bAbsoluteRotation = true; // Don't want arm to rotate when character does
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->RelativeRotation = FRotator(-60.f, 0.f, 0.f);
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level

	// Create a camera...
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Create a decal in the world to show the cursor's location
	CursorToWorld = CreateDefaultSubobject<UDecalComponent>("CursorToWorld");
	CursorToWorld->SetupAttachment(RootComponent);
	static ConstructorHelpers::FObjectFinder<UMaterial> DecalMaterialAsset(TEXT("Material'/Game/TopDownCPP/Blueprints/M_Cursor_Decal.M_Cursor_Decal'"));
	if (DecalMaterialAsset.Succeeded())
	{
		CursorToWorld->SetDecalMaterial(DecalMaterialAsset.Object);
	}
	CursorToWorld->DecalSize = FVector(16.0f, 32.0f, 32.0f);
	CursorToWorld->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f).Quaternion());

	// Create the line of sight mesh
	LOSMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("LineOfSightMesh"));
	LOSMesh->bUseAsyncCooking = true;
	LOSMesh->ContainsPhysicsTriMeshData(false);
	LOSMesh->AttachToComponent(RootComponent, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true));

	if (ArcAngle == 0)
		ArcAngle = 120;

	if (AngleStep == 0)
		AngleStep = 1;

	if (Radius == 0)
		Radius = 500;

	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AUE4StealthCharacter::InitLOSMesh()
{
	int NumVertices = FMath::RoundToInt(ArcAngle / AngleStep) + 2;
	LOSVertices.Init(FVector::ZeroVector, NumVertices);

	int NumTriangles = (ArcAngle == 360) ? ((NumVertices - 1) * 3) : ((NumVertices - 2) * 3);
	LOSTriangles.Init(0, NumTriangles);

	FVector LineStartLocation = GetActorLocation();
	FVector CurrentActorForward = GetActorForwardVector();
	float MinAngle = -ArcAngle / 2;
	float MaxAngle = ArcAngle / 2;
	int VertexIndex = 1;
	for (float CurrentAngle = MinAngle;
		CurrentAngle <= MaxAngle;
		CurrentAngle += AngleStep)
	{
		FVector CurrentAngleDirection = CurrentActorForward.RotateAngleAxis(CurrentAngle, FVector(0, 0, 1));
		FVector LineEndLocation = LineStartLocation + CurrentAngleDirection * Radius;

		FVector HitResultInCharacterLocalSpace = GetActorTransform().InverseTransformPosition(LineEndLocation);
		LOSVertices[VertexIndex] = HitResultInCharacterLocalSpace;
		VertexIndex++;
	}

	VertexIndex = 0;
	for (int Triangle = 0; Triangle < LOSTriangles.Num(); Triangle += 3)
	{
		LOSTriangles[Triangle] = 0;
		LOSTriangles[Triangle + 1] = VertexIndex + 1;

		if (Triangle == (NumVertices - 2) * 3) // the arc has 360 angle, or in other words, it's a circle
		{
			LOSTriangles[Triangle + 2] = 1;
		}
		else
		{
			LOSTriangles[Triangle + 1] = VertexIndex + 2;
		}

		VertexIndex++;
	}

	UpdateLOSMeshData(LOSVertices, LOSTriangles);
	
	LOSMesh->SetRelativeLocation(FVector(0, 0, -90));
	LOSMesh->SetMaterial(0, LOSMaterial);
}

void AUE4StealthCharacter::BeginPlay()
{
	Super::BeginPlay();

	InitLOSMesh();
}

void AUE4StealthCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (CursorToWorld != nullptr)
	{
		if (UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled())
		{
			if (UWorld* World = GetWorld())
			{
				FHitResult HitResult;
				FCollisionQueryParams Params(NAME_None, FCollisionQueryParams::GetUnknownStatId());
				FVector StartLocation = TopDownCameraComponent->GetComponentLocation();
				FVector EndLocation = TopDownCameraComponent->GetComponentRotation().Vector() * 2000.0f;
				Params.AddIgnoredActor(this);
				World->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility, Params);
				FQuat SurfaceRotation = HitResult.ImpactNormal.ToOrientationRotator().Quaternion();
				CursorToWorld->SetWorldLocationAndRotation(HitResult.Location, SurfaceRotation);
			}
		}
		else if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			FHitResult TraceHitResult;
			PC->GetHitResultUnderCursor(ECC_Visibility, true, TraceHitResult);
			FVector CursorFV = TraceHitResult.ImpactNormal;
			FRotator CursorR = CursorFV.Rotation();
			CursorToWorld->SetWorldLocation(TraceHitResult.Location);
			CursorToWorld->SetWorldRotation(CursorR);
		}
	}

	TickLOSMesh(DeltaSeconds);
}



void AUE4StealthCharacter::TickLOSMesh(float DeltaSeconds)
{
	UWorld* World = GetWorld();
	if (World == nullptr)
		return;

	const FName TraceTag("LoSTraceTag");
	FCollisionQueryParams TraceParams = FCollisionQueryParams(TraceTag, false, this);

	FVector LineStartLocation = GetActorLocation();
	FVector CurrentActorForward = GetActorForwardVector();

	float MinAngle = -ArcAngle / 2;
	float MaxAngle = ArcAngle / 2;
	int VertexIndex = 1;
	for (float CurrentAngle = MaxAngle;
		CurrentAngle >= MinAngle;
		CurrentAngle -= AngleStep)
	{
		FVector CurrentAngleDirection = CurrentActorForward.RotateAngleAxis(CurrentAngle, FVector(0, 0, 1));
		FVector LineEndLocation = LineStartLocation + CurrentAngleDirection * Radius;

		FHitResult HitResult;
		FVector HitPoint;

		// In DefaultEngine.ini: Channel=ECC_GameTraceChannel1,Name="Obstacle"
		bool bHit = World->LineTraceSingleByChannel(HitResult, LineStartLocation, LineEndLocation, ECollisionChannel::ECC_GameTraceChannel1, TraceParams, FCollisionResponseParams());
		if (bHit)
		{
			HitPoint = HitResult.ImpactPoint;
		}
		else
		{
			HitPoint = LineEndLocation;
		}

		FVector HitResultInCharacterLocalSpace = GetActorTransform().InverseTransformPosition(HitPoint);
		LOSVertices[VertexIndex] = HitResultInCharacterLocalSpace;
		VertexIndex++;
	}

	VertexIndex = 0;
	int NumVertices = LOSVertices.Num();
	for (int Triangle = 0; Triangle < LOSTriangles.Num(); Triangle += 3)
	{
		LOSTriangles[Triangle] = 0;
		LOSTriangles[Triangle + 1] = VertexIndex + 1;

		if (Triangle == (NumVertices - 2) * 3) // the arc has 360 angle, or in other words, it's a circle
		{
			LOSTriangles[Triangle + 2] = 1;
		}
		else
		{
			LOSTriangles[Triangle + 2] = VertexIndex + 2;
		}

		VertexIndex++;
	}

	UpdateLOSMeshData(LOSVertices, LOSTriangles);
}


void AUE4StealthCharacter::UpdateLOSMeshData(const TArray<FVector>& Vertices, const TArray<int32>& Triangles)
{
	TArray<FVector> TempNormals;
	TArray<FVector2D> TempUV0;
	TArray<FProcMeshTangent> TempTangents;
	TArray<FLinearColor> TempVertexColors;
	LOSMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, TempNormals, TempUV0, TempVertexColors, TempTangents, false);
}
