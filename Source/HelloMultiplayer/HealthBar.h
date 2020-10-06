// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HealthBar.generated.h"

/**
 * 
 */
UCLASS()
class HELLOMULTIPLAYER_API UHealthBar : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	void SetBarValue(float percent);

protected:
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	class UProgressBar* ProgressBar;
};
