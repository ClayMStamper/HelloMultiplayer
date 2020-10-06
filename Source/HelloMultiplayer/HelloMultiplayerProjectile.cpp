// Fill out your copyright notice in the Description page of Project Settings.


#include "HelloMultiplayerProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "UObject/ConstructorHelpers.h"

// Sets default values
AHelloMultiplayerProjectile::AHelloMultiplayerProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	//Definition for the SphereComponent that will serve as the Root component for the projectile and its collision.
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("RootComponent"));
	SphereComponent->InitSphereRadius(37.5f);
	SphereComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	RootComponent = SphereComponent;

	//Registering the Projectile Impact function on a Hit event.
	if (GetLocalRole() == ROLE_Authority)
	{
		SphereComponent->OnComponentHit.AddDynamic(this, &AHelloMultiplayerProjectile::OnProjectileImpact);
	}
	
	//Definition for the Mesh that will serve as our visual representation.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshObj(TEXT("/Game/Meshes/Rock/Rock"));
	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	StaticMesh->SetupAttachment(RootComponent);

	//Set the Static Mesh and its position/scale if we successfully found a mesh asset to use.
	if (MeshObj.Succeeded())
	{
		StaticMesh->SetStaticMesh(MeshObj.Object);
		StaticMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -37.5f));
		StaticMesh->SetRelativeScale3D(FVector(0.75f, 0.75f, 0.75f));
	}

	static ConstructorHelpers::FObjectFinder<UParticleSystem> DefaultExplosionEffect(TEXT("/Game/StarterContent/Particles/P_Explosion.P_Explosion"));
	if (DefaultExplosionEffect.Succeeded())
	{
		ExplosionEffect = DefaultExplosionEffect.Object;
	}

	//Definition for the Projectile Movement Component.
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovementComponent->SetUpdatedComponent(SphereComponent);
	ProjectileMovementComponent->InitialSpeed = 1500.0f;
	ProjectileMovementComponent->MaxSpeed = 1500.0f;
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->ProjectileGravityScale = 0.0f;

	//Set damage type params
	DamageType = UDamageType::StaticClass();
	Damage = 10.0f;
	
}

// Called when the game starts or when spawned
void AHelloMultiplayerProjectile::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AHelloMultiplayerProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

//particle emitter doesn't replicated, but destruction is... so this call is synced indirectly
void AHelloMultiplayerProjectile::Destroyed()
{
	FVector spawnLocation = GetActorLocation();
	UGameplayStatics::SpawnEmitterAtLocation(this, ExplosionEffect, spawnLocation, FRotator::ZeroRotator, true,
		EPSCPoolMethod::AutoRelease);
}

/*
 * This is the function that we are going to call when the Projectile impacts with an object.
 * If the object it impacts with is a valid Actor,
 * it will call the ApplyPointDamage function to damage it at the point where the collision takes place. Meanwhile,
 * any collision regardless of the impacted surface will destroy this Actor, causing the explosion effect to appear.
 */
void AHelloMultiplayerProjectile::OnProjectileImpact(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector ImpulseNormal, const FHitResult& Hit)
{
	if (OtherActor)
	{
		UGameplayStatics::ApplyPointDamage(OtherActor, Damage, ImpulseNormal, Hit, GetInstigator()->Controller, this, DamageType);
	}

	Destroy();
}

