// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "UE4StealthCharacter.generated.h"

UCLASS(Blueprintable)
class AUE4StealthCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AUE4StealthCharacter();

	// Called every frame.
	virtual void Tick(float DeltaSeconds) override;

	// Called when start playing.
	virtual void BeginPlay() override;

	/** Returns TopDownCameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent; }
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns CursorToWorld subobject **/
	FORCEINLINE class UDecalComponent* GetCursorToWorld() { return CursorToWorld; }

private:
	/** Top down camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* TopDownCameraComponent;

	/** Camera boom positioning the camera above the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** A decal that projects to the cursor location. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UDecalComponent* CursorToWorld;

private:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Line of Sight", meta = (AllowPrivateAccess = "true"))
	class UProceduralMeshComponent * LOSMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Line of Sight", meta = (AllowPrivateAccess = "true"))
	UMaterial* LOSMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Line of Sight", meta = (AllowPrivateAccess = "true", UIMin = "1.0", UIMax = "360.0"))
	float ArcAngle;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Line of Sight", meta = (AllowPrivateAccess = "true", UIMin = "1.0", UIMax = "5.0"))
	float AngleStep;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Line of Sight", meta = (AllowPrivateAccess = "true", UIMin = "200", UIMax = "1000"))
	float Radius;

private:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Line of Sight", meta = (AllowPrivateAccess = "true"))
	TArray<FVector> LOSVertices;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Line of Sight", meta = (AllowPrivateAccess = "true"))
	TArray<int32> LOSTriangles;

private:

	void InitLOSMesh();

	void TickLOSMesh(float DeltaSeconds);

	void UpdateLOSMeshData(const TArray<FVector>& Vertices, const TArray<int32>& Triangles);

};

