// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthBar.h"

#include "Components/ProgressBar.h"

void UHealthBar::NativeConstruct()
{
    Super::NativeConstruct();
}

void UHealthBar::SetBarValue(float percent)
{
    UE_LOG(LogTemp, Warning, TEXT("Setting Health Bar value to: %f"), percent);
    ProgressBar->SetPercent(percent);
}
