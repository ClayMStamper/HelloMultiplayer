// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HelloMultiplayerProjectile.generated.h"

UCLASS()
class HELLOMULTIPLAYER_API AHelloMultiplayerProjectile : public AActor
{
	GENERATED_BODY()
	
public:
	
	// Sets default values for this actor's properties
	AHelloMultiplayerProjectile();

	// Sphere component used to test collision.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	class USphereComponent* SphereComponent;

	// Static mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	class UStaticMeshComponent* StaticMesh;

	// Movement component
	// Automatically handles replication, like the Character component (assuming, bReplicates = true)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	class UProjectileMovementComponent* ProjectileMovementComponent;
	
	// Particle used for impact visuals
	UPROPERTY(EditAnywhere, Category="Effects")
	class UParticleSystem* ExplosionEffect;

	// damage type for damage events
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage")
	TSubclassOf<class UDamageType> DamageType;
	
	// damage dealt
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage")
	float Damage;

protected:
	
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void Destroyed() override;

	UFUNCTION()
	void OnProjectileImpact(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector ImpulseNormal, const FHitResult& Hit);
	
public:
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
