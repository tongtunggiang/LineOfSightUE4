// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "UE4StealthGameMode.h"
#include "UE4StealthPlayerController.h"
#include "UE4StealthCharacter.h"
#include "UObject/ConstructorHelpers.h"

AUE4StealthGameMode::AUE4StealthGameMode()
{
	// use our custom PlayerController class
	PlayerControllerClass = AUE4StealthPlayerController::StaticClass();

	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/TopDownCPP/Blueprints/TopDownCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}