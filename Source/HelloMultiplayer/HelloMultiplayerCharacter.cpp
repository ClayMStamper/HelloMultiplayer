// Copyright Epic Games, Inc. All Rights Reserved.

#include "HelloMultiplayerCharacter.h"
#include "HelloMultiplayerProjectile.h"
#include "HealthBar.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "UObject/ConstructorHelpers.h"

//networking includes
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"

#define NET_LOG(msg) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, msg)
#define NET_LOG_C(msg, color) GEngine->AddOnScreenDebugMessage(-1, 5.f, color, msg)
#define NET_LOG_LOCAL(msg) if (IsLocallyControlled()) NET_LOG_C(msg, FColor::Green)
#define NET_LOG_SERVER(msg) if (GetLocalRole() == ROLE_Authority) NET_LOG_C(msg, FColor::Blue)

//////////////////////////////////////////////////////////////////////////
// AHelloMultiplayerCharacter

AHelloMultiplayerCharacter::AHelloMultiplayerCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)

	//init health
	CurrentHealth = MaxHealth;
	// HealthBar = CreateDefaultSubobject<UHealthBar>("HealthBar");
	// HealthBar->SetupAttachment(RootComponent);
	// HealthBar->SetRelativeLocation(FVector(0.f, 0.f, 85.f));

	//Initialize projectile class
	ProjectileClass = AHelloMultiplayerProjectile::StaticClass();
	//Initialize fire rate
	FireRate = 0.25f;
	bIsCasting1H = false;
	
}

//////////////////////////////////////////////////////////////////////////
// Input

void AHelloMultiplayerCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	// check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &AHelloMultiplayerCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AHelloMultiplayerCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AHelloMultiplayerCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AHelloMultiplayerCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AHelloMultiplayerCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AHelloMultiplayerCharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AHelloMultiplayerCharacter::OnResetVR);
	
	// Handle firing projectiles
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AHelloMultiplayerCharacter::StartFire);

	// Handle dodge input
	PlayerInputComponent->BindAction("Roll", IE_Pressed, this, &AHelloMultiplayerCharacter::Client_StartRoll);
}


void AHelloMultiplayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
}

void AHelloMultiplayerCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AHelloMultiplayerCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
		Jump();
}

void AHelloMultiplayerCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
		StopJumping();
}

void AHelloMultiplayerCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AHelloMultiplayerCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AHelloMultiplayerCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AHelloMultiplayerCharacter::MoveRight(float Value)
{
	if ( (Controller != nullptr) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

/* Only callable by server */
void AHelloMultiplayerCharacter::SetCurrentHealth(float healthValue)
{
	if (GetLocalRole() == ROLE_Authority)
	{
		CurrentHealth = FMath::Clamp(healthValue, 0.f, MaxHealth);
		OnHealthUpdate();
	}
}

float AHelloMultiplayerCharacter::TakeDamage(float DamageTaken, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float newHealth = CurrentHealth - DamageTaken;
	SetCurrentHealth(newHealth);
	return newHealth;
}

void AHelloMultiplayerCharacter::OnRep_CurrentHealth()
{
	OnHealthUpdate();
}

void AHelloMultiplayerCharacter::OnHealthUpdate()
{

	// client specific logic
	if (IsLocallyControlled())
	{
		NET_LOG_LOCAL(FString::Printf(TEXT("You now have %f health remaining"), CurrentHealth));
		// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, healthMessage);

		if (CurrentHealth <= 0)
		{
			NET_LOG_LOCAL(FString::Printf(TEXT("Your health is below zero!")));
			bIsDead = true;
		}
		
	}

	// server specific logic
	NET_LOG_SERVER(FString::Printf(TEXT("%s now has %f health remaining"), *GetFName().ToString(), CurrentHealth));

	// Universal logic
	/*functionality that should occur as a result of damage or death goes here*/
	
}

void AHelloMultiplayerCharacter::OnRep_IsDead()
{
	HandleDeath();
	NET_LOG_LOCAL(FString::Printf(TEXT("You are now dead!")));
	GetWorld()->GetTimerManager().SetTimer(DeathTimer, this, &AHelloMultiplayerCharacter::HandleRespawn, RespawnCooldown, false);
}

//called locally on each client
void AHelloMultiplayerCharacter::HandleDeath()
{
	if (IsLocallyControlled())
	{
		//deactivate player controls
		GetMovementComponent()->Deactivate();

		//play death animation
	}
}

void AHelloMultiplayerCharacter::HandleRespawn()
{
	NET_LOG_LOCAL(FString::Printf(TEXT("You are now RESPAWNING!!")));
	bIsDead = false;
}


// called on client
void AHelloMultiplayerCharacter::StartFire()
{

	// NET_LOG("Trying to fire");
	
	// can fire
	if (!bIsCasting1H)
	{
		// fire
		Blueprint_OnFire();
		UWorld* World = GetWorld();
		//manages requests sent to the server
		World->GetTimerManager().SetTimer(FiringTimer, this, &AHelloMultiplayerCharacter::StopFire, FireRate,false);
		Server_HandleFire();
		bIsCasting1H = true;
	} else
	{
		// NET_LOG("Couldn't fire, already firing!");
	}
}

void AHelloMultiplayerCharacter::StopFire()
{
	// NET_LOG("Firing finished");
	bIsCasting1H = false;
}

void AHelloMultiplayerCharacter::Client_StartRoll()
{
	UE_LOG(LogTemp, Warning, TEXT("Roll input receieved!"));
	if (CanRoll()) {
		bIsRolling = true;
		GetWorld()->GetTimerManager().SetTimer(RollTimer, this, &AHelloMultiplayerCharacter::StopRoll, RollCooldown, false);
		Server_SetRollDirection();
	}
}



void AHelloMultiplayerCharacter::Server_SetRollDirection_Implementation()
{
	if (!CanRoll())
	{
		UE_LOG(LogTemp, Warning, TEXT("Declining roll request on server"));
		return;
	}

	RollDirection = GetActorRotation();
	PlayAnimMontage(RollMontage);
	
}

void AHelloMultiplayerCharacter::OnRep_RollDirection()
{
	SetActorRotation(RollDirection);
	PlayAnimMontage(RollMontage);
}


bool AHelloMultiplayerCharacter::CanRoll()
{

	UCharacterMovementComponent* MoveComponent = GetCharacterMovement();

	if (!MoveComponent)
		return false;

	if (bIsRolling)
		return false;

	const bool bCurrentlyFalling = MoveComponent->IsFalling();
	const bool bHasMoveInputRights = !MoveComponent->GetLastInputVector().Equals(FVector::ZeroVector);

	const bool bCanRoll = !bCurrentlyFalling && (bHasMoveInputRights || HasAuthority());
	
	NET_LOG_LOCAL(FString::Printf(TEXT("CanRoll() = %hs"), bCanRoll ? "true" : "false"));
	
	return bCanRoll;
	
}



void AHelloMultiplayerCharacter::StopRoll()
{
	NET_LOG("Roll is finished");
	bIsRolling = false;
}


// called on server
void AHelloMultiplayerCharacter::Server_HandleFire_Implementation()
{

	//spawn projectile
	const FVector cameraForward = GetControlRotation().Vector() * 100.0f;
	const FVector actorUp = GetActorUpVector() * 50.f;
	const FVector spawnLocation = GetActorLocation() + cameraForward + actorUp;
	const FRotator spawnRotation = GetControlRotation();

	FActorSpawnParameters spawnParameters;
	spawnParameters.Instigator = GetInstigator();
	spawnParameters.Owner = this;

	AHelloMultiplayerProjectile* spawnedProjectile = GetWorld()->SpawnActor<AHelloMultiplayerProjectile>(spawnLocation, spawnRotation, spawnParameters);
	
}


void AHelloMultiplayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//Replicate current health
	DOREPLIFETIME(AHelloMultiplayerCharacter, CurrentHealth);
	DOREPLIFETIME(AHelloMultiplayerCharacter, bIsDead);
	DOREPLIFETIME(AHelloMultiplayerCharacter, RollDirection);
}
