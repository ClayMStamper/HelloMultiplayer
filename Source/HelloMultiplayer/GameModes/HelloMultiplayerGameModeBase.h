// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HelloMultiplayerGameModeBase.generated.h"

UCLASS()
class HELLOMULTIPLAYER_API AHelloMultiplayerGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	
private:

	void HandleGameStart();
	void HandleGameOver();
	
public:

	void ActorDied(AActor* DeadActor);
	
protected:

	virtual void BeginPlay() override;
	UFUNCTION(BlueprintImplementableEvent)
	void GameStart();
	UFUNCTION(BlueprintImplementableEvent)
	void GameOver(bool PlayerWon);
	
};
