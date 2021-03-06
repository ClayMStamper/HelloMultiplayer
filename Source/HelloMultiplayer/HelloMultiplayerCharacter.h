// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Character.h"
#include "HelloMultiplayerCharacter.generated.h"

UCLASS(config=Game)
class AHelloMultiplayerCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;
	
public:

	AHelloMultiplayerCharacter();

	/**responsible for replicating any properties we designate with "Replicated"
	enables us to configure how a property will replicate*/
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

	// UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Health")
	// UHealthBar* HealthBar = nullptr;
	
	UFUNCTION(BlueprintPure, Category="Health")
	FORCEINLINE float GetMaxHealth() const {return MaxHealth;}
	UFUNCTION(BlueprintPure, Category="Health")
    FORCEINLINE float GetCurrentHealth() const {return CurrentHealth;}
	UFUNCTION(BlueprintPure, Category = "Spell Casting")
	FORCEINLINE float GetMaxMana() const { return MaxMana; };
	UFUNCTION(BlueprintPure, Category = "Spell Casting")
	FORCEINLINE float GetCurrentMana() const { return CurrentMana; };

	/** Setter for Current Health.
	 *Clamps the value between 0 and MaxHealth and calls OnHealthUpdate. Should only be called on the server.*/
	UFUNCTION(BlueprintCallable, Category="Health")
    void SetCurrentHealth(float healthValue);

	/** Event for taking damage. Overridden from APawn.*/
	UFUNCTION(BlueprintCallable, Category = "Health")
	virtual float TakeDamage( float DamageTaken, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser ) override;

	UPROPERTY(EditAnywhere)
	UAnimMontage* RollMontage;
	UPROPERTY(EditAnywhere)
	UAnimMontage* AttackMontage;
	UPROPERTY(EditAnywhere)
	UAnimMontage* DeathMontage;
	
protected:

	virtual void BeginPlay() override;
	
	/** Resets HMD orientation in VR. */
	void OnResetVR();

	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	/** 
	 * Called via input to turn at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	/** Handler for when a touch input begins. */
	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);

	/** Handler for when a touch input stops. */
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);

	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// End of APawn interface

	// START HEALTH / DEATH CODE
	
	/** The player's maximum health. This is the highest that their health can be, and the value that their health starts at when spawned.*/
	UPROPERTY(EditAnywhere, Category="Health")
	float MaxHealth = 100.f;
	UPROPERTY(EditAnywhere, Category = "Spell Casting")
	float MaxMana = 100.f;
	
	/** The player's current health - dead when reduced to zero*/
	UPROPERTY(ReplicatedUsing=OnRep_CurrentHealth)
	float CurrentHealth;
	UPROPERTY(ReplicatedUsing = OnRep_CurrentMana)
	float CurrentMana;

	UFUNCTION()
	void OnRep_CurrentHealth();
	UFUNCTION()
	void OnRep_CurrentMana();

	/** Response to health being updated - NOT REPLICATED - called on each device
	 * Server: Immediately after modification
	 * Client: Response to RepNotify
	 */
	void Client_OnHealthUpdate();
	void Client_OnManaUpdate();

	UPROPERTY(ReplicatedUsing=OnRep_IsDead)
	bool bIsDead = false;

	UFUNCTION()
	void OnRep_IsDead();

	UPROPERTY(Transient)
	FTimerHandle DeathTimer;

	UPROPERTY(EditAnywhere, Category="Death")
	float RespawnCooldown = 3.f;

	/**
	 * Response to IsDead being updated
	 */
	UFUNCTION()
	void HandleDeath();

	UFUNCTION()
	void HandleRespawn();

	// END HEALTH / DEATH CODE

	// START WEAPON CODE
	
	UPROPERTY(EditDefaultsOnly, Category="Gameplay|Combat")
	TSubclassOf<class AHelloMultiplayerProjectile> ProjectileClass;

	/** Delay between shots in seconds. Used to control fire rate for our test projectile, but also to prevent an overflow of server functions from binding SpawnProjectile directly to input.*/
	UPROPERTY(EditDefaultsOnly, Category="Gameplay|Combat")
	float FireRate;

	/** If true, we are in the process of firing projectiles. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	bool bIsCasting1H;

	/** Function for beginning weapon fire.*/
	UFUNCTION(BlueprintCallable, Category="Gameplay|Combat")
    void StartFire();

	UFUNCTION(BlueprintImplementableEvent)
	void Blueprint_OnFire();

	/** Function for ending weapon fire. Once this is called, the player can use StartFire again.*/
	UFUNCTION(BlueprintCallable, Category = "Gameplay|Combat")
    void StopFire();  

	/** Server function for spawning projectiles.*/
	UFUNCTION(Server, Reliable)
    void Server_HandleFire();

	/** A timer handle used for providing the fire rate delay in-between spawns.*/
	UPROPERTY(Transient)
	FTimerHandle FiringTimer;

	// END WEAPON CODE

	// START DODGE-ROLL CODE
	UPROPERTY(ReplicatedUsing = OnRep_RollDirection)
	FRotator RollDirection;
	UPROPERTY(BlueprintReadWrite)
	float RollCooldown = 1.5f;
	UPROPERTY(BlueprintReadWrite);
	bool bIsRolling = false;

	UFUNCTION()
	void Client_StartRoll();
	UFUNCTION(BlueprintCallable, Category = "Gameplay|Movement")
	bool CanRoll();
	UFUNCTION(Server, Reliable)
	void Server_SetRollDirection();
	UFUNCTION()
	void OnRep_RollDirection();
	UFUNCTION(Category="Gameplay|Movement")
	void StopRoll();

	UFUNCTION(BlueprintImplementableEvent)
	void BlueprintDodgeRollCallback();
	
	UPROPERTY(Transient)
	FTimerHandle RollTimer;

	// END DODGE-ROLL CODE
	
public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};

