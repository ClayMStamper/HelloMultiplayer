// Copyright Epic Games, Inc. All Rights Reserved.

#include "HelloMultiplayerGameMode.h"
#include "HelloMultiplayerCharacter.h"
#include "UObject/ConstructorHelpers.h"

AHelloMultiplayerGameMode::AHelloMultiplayerGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
