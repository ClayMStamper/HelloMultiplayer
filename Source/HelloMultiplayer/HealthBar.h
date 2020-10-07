// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"

#include "HealthBar.generated.h"

/**
 * 
 */
UCLASS()
class HELLOMULTIPLAYER_API UHealthBar : public UWidgetComponent
{
	GENERATED_BODY()

public:
	void SetBarValue(float percent);

protected:
	UPROPERTY(BlueprintReadWrite, meta=(BindWidget), Category="Health")
	class UProgressBar* ProgressBar;
};
