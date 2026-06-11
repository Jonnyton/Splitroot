#include "ArchonCombatHealthComponent.h"
#include "Net/UnrealNetwork.h"

UArchonCombatHealthComponent::UArchonCombatHealthComponent()
{
	SetIsReplicatedByDefault(true);
}

void UArchonCombatHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UArchonCombatHealthComponent, TeamId);
	DOREPLIFETIME(UArchonCombatHealthComponent, MaxHealth);
	DOREPLIFETIME(UArchonCombatHealthComponent, CurrentHealth);
	DOREPLIFETIME(UArchonCombatHealthComponent, ArmorModifier);
	DOREPLIFETIME(UArchonCombatHealthComponent, TotalHitsTaken);
	DOREPLIFETIME(UArchonCombatHealthComponent, TotalDeaths);
	DOREPLIFETIME(UArchonCombatHealthComponent, LastDamageApplication);
}

void UArchonCombatHealthComponent::ConfigureHealth(int32 InTeamId, float InMaxHealth, float InArmorModifier)
{
	TeamId = InTeamId;
	MaxHealth = FMath::Max(1.0f, InMaxHealth);
	CurrentHealth = MaxHealth;
	ArmorModifier = FMath::Max(0.0f, InArmorModifier);
	TotalHitsTaken = 0;
	TotalDeaths = 0;
	LastDamageApplication = FArchonDamageApplicationResult();
}

FArchonDamageApplicationResult UArchonCombatHealthComponent::ApplyHit(const FArchonHitResult& HitResult)
{
	FArchonDamageApplicationResult Result;
	Result.PreviousHealth = CurrentHealth;
	Result.NewHealth = CurrentHealth;

	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		Result.Reason = TEXT("not_authority");
		return Result;
	}

	if (!IsAlive())
	{
		Result.Reason = TEXT("already_dead");
		return Result;
	}

	if (!HitResult.bShouldHit)
	{
		Result.Reason = HitResult.Reason;
		return Result;
	}

	Result.bAccepted = true;
	Result.DamageApplied = FMath::Max(0.0f, HitResult.FinalDamage);
	Result.NewHealth = FMath::Max(0.0f, CurrentHealth - Result.DamageApplied);
	Result.bCausedDeath = Result.NewHealth <= 0.0f && CurrentHealth > 0.0f;
	Result.Reason = TEXT("damage_applied");

	CurrentHealth = Result.NewHealth;
	++TotalHitsTaken;
	LastDamageApplication = Result;
	if (!HitResult.ShotOrigin.IsNearlyZero())
	{
		LastAcceptedShotOrigin = HitResult.ShotOrigin;
	}

	if (Result.bCausedDeath)
	{
		++TotalDeaths;
		OnDeath.Broadcast(Result);
	}

	OnHealthChanged.Broadcast(Result);

	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: CombatHealthApplied team=%d damage=%.1f health=%.1f->%.1f death=%s"),
		TeamId,
		Result.DamageApplied,
		Result.PreviousHealth,
		Result.NewHealth,
		Result.bCausedDeath ? TEXT("true") : TEXT("false"));

	return Result;
}

FArchonDamageApplicationResult UArchonCombatHealthComponent::ApplyDirectDamage(
	float DamageAmount,
	EArchonDamageType DamageType,
	int32 InstigatorTeamId,
	int32 InstigatorPlayerId)
{
	FArchonHitResult Hit;
	Hit.bShouldHit = DamageAmount > 0.0f;
	Hit.FinalDamage = DamageAmount;
	Hit.HitType = EArchonHitType::Body;
	Hit.DamageType = DamageType;
	Hit.InstigatorTeamId = InstigatorTeamId;
	Hit.InstigatorPlayerId = InstigatorPlayerId;
	Hit.Reason = Hit.bShouldHit ? FName(TEXT("direct_damage")) : FName(TEXT("non_positive_damage"));
	return ApplyHit(Hit);
}

void UArchonCombatHealthComponent::HealToFull()
{
	const float PreviousHealth = CurrentHealth;
	CurrentHealth = MaxHealth;

	FArchonDamageApplicationResult Result;
	Result.bAccepted = true;
	Result.DamageApplied = 0.0f;
	Result.PreviousHealth = PreviousHealth;
	Result.NewHealth = CurrentHealth;
	Result.bCausedDeath = false;
	Result.Reason = TEXT("healed_to_full");
	LastDamageApplication = Result;
	OnHealthChanged.Broadcast(Result);
}

void UArchonCombatHealthComponent::ResetProofState()
{
	CurrentHealth = MaxHealth;
	TotalHitsTaken = 0;
	TotalDeaths = 0;
	LastDamageApplication = FArchonDamageApplicationResult();
}
