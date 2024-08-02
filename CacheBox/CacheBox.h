/*****************************************************************//**
 * \file   CacheBox.h
 * \brief  A box that records and manages loaded and generated objects collectively
 *
 * \date   07/31/2024
 * \author Lyuro
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Engine/StreamableManager.h"
#include "Blueprint/UserWidget.h"
#include "UtilsCommon.h"
#include "Logging/StructuredLog.h"
#include "CacheBox.generated.h"

EXPANSION_API DECLARE_LOG_CATEGORY_EXTERN(LogCacheBox, Log, All);

class UCacheBoxComponent;

/**
 * Memory cache box.
 * It will be destroyed together when someone else holds a reference and destroys it.
 * Please call ConditionalBeginDestroy when destroying. Refer to UCacheBoxComponent::DestroyCacheBox().
 */
UCLASS()
class EXPANSION_API UCacheBox final : public UObject
{
	GENERATED_BODY()

protected:
	explicit UCacheBox(const FObjectInitializer& ObjectInitializer);

	virtual void BeginDestroy() override;

public:
	/**
	 * Destruction process.
	 *
	 * \return
	 */
	bool DestroyBox();

public:
	/**
	* Synchronous loading of objects.
	*
	* \param SoftObject  BP object that needs to be loaded
	* \return Loaded UObject
	*/
	template <typename T = UObject, typename = std::enable_if_t<std::is_base_of_v<UObject, T>>>
	T* LoadSynchronous(TSoftObjectPtr<T> SoftObject)
	{
		UE_LOGFMT(LogCacheBox, Log, "UCacheBox::LoadSynchronous {0} TryLoad {1}", GetDebugIdentifyString(), SoftObject.GetAssetName());

		if (!SoftObject.IsNull())
		{
			if (T* LoadedObject = Cast<T>(SoftObject.LoadSynchronous()))
			{
				UE_LOGFMT(LogCacheBox, Log, "UCacheBox::LoadSynchronous {0} Loaded {1}", GetDebugIdentifyString(), SoftObject.GetAssetName());
				LoadedSoftObjects.Emplace(LoadedObject);
				LoadedObjects.Emplace(LoadedObject);
				return LoadedObject;
			}
		}
		UE_LOGFMT(LogCacheBox, Error, "UCacheBox::LoadSynchronous {0} Failed to load {1}", GetDebugIdentifyString(), SoftObject.GetAssetName());
		return nullptr;
	}

	/**
	* Synchronous loading of an array of objects.
	*
	* \param SoftObjectArray  Array of BP objects that need to be loaded
	* \return Loaded UObject
	*/
	void LoadSynchronous(const TArray<TSoftObjectPtr<UObject>>& SoftObjectArray);

	/**
	* Synchronous loading of classes.
	*
	* \param SoftClass BP class that needs to be loaded
	* \return Loaded UClass
	*/
	template <typename T = UClass, typename = std::enable_if_t<std::is_base_of_v<UObject, T>>>
	UClass* LoadSynchronous(TSoftClassPtr<T> SoftClass)
	{
		UE_LOGFMT(LogCacheBox, Log, "UCacheBox::LoadSynchronous {0} TryLoad {1}", GetDebugIdentifyString(), SoftClass.GetAssetName());
		if (!SoftClass.IsNull())
		{
			if (UClass* LoadedClass = SoftClass.LoadSynchronous())
			{
				if (LoadedClass->IsChildOf(T::StaticClass()))
				{
					UE_LOGFMT(LogCacheBox, Log, "UCacheBox::LoadSynchronous {0} Loaded {1}", GetDebugIdentifyString(), SoftClass.GetAssetName());
					LoadedSoftClasses.Emplace(LoadedClass);
					LoadedObjects.Emplace(LoadedClass);
					return LoadedClass;
				}
			}
		}
		UE_LOGFMT(LogCacheBox, Error, "UCacheBox::LoadSynchronous {0} Failed to load {1}", GetDebugIdentifyString(), SoftClass.GetAssetName());
		return nullptr;
	}

	/**
	* Synchronous loading of an array of classes.
	*
	* \param SoftClassArray  Array of BP objects that need to be loaded
	* \return Loaded UClass
	*/
	void LoadSynchronous(const TArray<TSoftClassPtr<UObject>>& SoftClassArray);

	/**
	* Asynchronous loading of objects.
	*
	* \param SoftObject BP object that needs to be loaded
	* \param DelegateToCall Callback delegate upon load completion
	*/
	template <typename T = UObject, typename = std::enable_if_t<std::is_base_of_v<UObject, T>>>
	void RequestAsyncLoad(TSoftObjectPtr<T> SoftObject, const FStreamableDelegate& DelegateToCall)
	{
		UE_LOGFMT(LogCacheBox, Log, "UCacheBox::RequestAsyncLoad {0} TryLoad {1}", GetDebugIdentifyString(), SoftObject.GetAssetName());

		if (!SoftObject.IsNull())
		{
			FStreamableDelegate Delegate;
			Delegate.BindWeakLambda(this, [this, DelegateToCall, SoftObject]()
				{
					if (T* LoadedObject = Cast<T>(SoftObject.Get()))
					{
						UE_LOGFMT(LogCacheBox, Log, "UCacheBox::RequestAsyncLoad {0} Loaed {1}", GetDebugIdentifyString(), SoftObject.GetAssetName());
						LoadedSoftObjects.Emplace(LoadedObject);
						LoadedObjects.Emplace(LoadedObject);
					}
					DelegateToCall.ExecuteIfBound();
				});
			Streamable.RequestAsyncLoad(SoftObject.ToSoftObjectPath(), MoveTemp(Delegate));
		}
		else
		{
			DelegateToCall.ExecuteIfBound();
		}
	}

	/**
	* Asynchronous loading of an array of objects.
	*
	* \param SoftObjectArray Array of BP objects that need to be loaded
	* \param DelegateToCall Callback delegate upon load completion
	*/
	void RequestAsyncLoad(const TArray<TSoftObjectPtr<UObject>>& SoftObjectArray, const FStreamableDelegate& DelegateToCall);

	/**
	* Asynchronous loading of classes.
	*
	* \param SoftClass BP class that needs to be loaded
	* \param DelegateToCall Callback delegate upon load completion
	*/
	template <typename T = UClass, typename = std::enable_if_t<std::is_base_of_v<UObject, T>>>
	void RequestAsyncLoad(TSoftClassPtr<T> SoftClass, const FStreamableDelegate& DelegateToCall)
	{
		UE_LOGFMT(LogCacheBox, Log, "UCacheBox::RequestAsyncLoad {0} TryLoad {1}", GetDebugIdentifyString(), SoftClass.GetAssetName());
		if (!SoftClass.IsNull())
		{
			FStreamableDelegate Delegate;
			Delegate.BindWeakLambda(this, [this, DelegateToCall, SoftClass]()
				{
					if (ensure(SoftClass.IsValid()))
					{
						if (SoftClass->IsChildOf(T::StaticClass()))
						{
							UE_LOGFMT(LogCacheBox, Log, "UCacheBox::RequestAsyncLoad {0} TryLoad {1}", GetDebugIdentifyString(), SoftClass.GetAssetName());
							LoadedSoftClasses.Emplace(SoftClass);
						}
					}
					DelegateToCall.ExecuteIfBound();
				});
			Streamable.RequestAsyncLoad(SoftClass.ToSoftObjectPath(), MoveTemp(Delegate));
		}
		else
		{
			DelegateToCall.ExecuteIfBound();
		}
	}

	/**
	* Asynchronous loading of an array of classes.
	*
	* \param SoftClassArray Array of BP classes that need to be loaded
	* \param DelegateToCall Callback delegate upon load completion
	*/
	void RequestAsyncLoad(const TArray<TSoftClassPtr<UObject>>& SoftClassArray, const FStreamableDelegate& DelegateToCall);

public:
	/**
	* Create and hold an Object.
	*
	* \return Created instance
	*/
	template <typename T = UObject, typename = std::enable_if_t<std::is_base_of_v<UObject, T>>>
	[[nodiscard]]
	T* CreateObject(const TSubclassOf<UObject> ObjectClass = T::StaticClass())
	{
		if(IsValid(ObjectClass))
		{
			UE_LOGFMT(LogCacheBox, Log, "UCacheBox::CreateObject {0} TryCreate {1}", GetDebugIdentifyString(), ObjectClass->GetName());
		}
		else
		{
			UE_LOGFMT(LogCacheBox, Error, "UCacheBox::CreateObject {0} TryCreate {1}", GetDebugIdentifyString(), TEXT("Invalid class"));
			return nullptr;
		}

		if (T* Object = NewObject<T>(this, ObjectClass))
		{
			UE_LOGFMT(LogCacheBox, Log, "UCacheBox::CreateObject {0} Created {1}", GetDebugIdentifyString(), Object->GetName());
			CreatedObjects.Emplace(Object);
			return Object;
		}
		UE_LOGFMT(LogCacheBox, Warning, "UCacheBox::CreateObject {0} Create {1} Failed.", GetDebugIdentifyString(), ObjectClass->GetName());
		return nullptr;
	}

/**
 * Create and hold a UserWidget.
 *
 * \param UserWidgetClass
 * \return Created instance
 */
	template <typename WidgetT = UUserWidget, typename = std::enable_if_t<std::is_base_of_v<UObject, UUserWidget>>>
	[[nodiscard]]
	WidgetT* CreateWidget(const TSubclassOf<UUserWidget> UserWidgetClass = WidgetT::StaticClass())
	{
		if (IsValid(UserWidgetClass))
		{
			UE_LOGFMT(LogCacheBox, Log, "UCacheBox::CreateWidget {0} TryCreate {1}", GetDebugIdentifyString(), UserWidgetClass->GetName());
		}
		else
		{
			UE_LOGFMT(LogCacheBox, Error, "UCacheBox::CreateWidget {0} TryCreate {1}", GetDebugIdentifyString(), TEXT("Invalid class"));
			return nullptr;
		}

		if (Utils::Common::GetWorld())
		{
			if (WidgetT* Widget = CreateWidget<WidgetT>(Utils::Common::GetWorld(), UserWidgetClass))
			{
				UE_LOGFMT(LogCacheBox, Log, "UCacheBox::CreateWidget {0} Created {1}", GetDebugIdentifyString(), Widget->GetName());
				CreatedObjects.Emplace(Widget);
				return Widget;
			}
		}
		UE_LOGFMT(LogCacheBox, Warning, "UCacheBox::CreateWidget {0} Create {1} Failed.", GetDebugIdentifyString(), UserWidgetClass->GetName());
		return nullptr;
	}

/**
 * Create and hold an Actor.
 *
 * \return Spawned Actor, use after casting
 */
	template <typename T = AActor, typename = std::enable_if_t<std::is_base_of_v<UObject, AActor>>>
	[[nodiscard]]
	T* CreateActor(const TSubclassOf<AActor> InClass = T::StaticClass())
	{
		if (IsValid(InClass))
		{
			UE_LOGFMT(LogCacheBox, Verbose, "UCacheBox::CreateActor {0} TryCreate {1}", GetDebugIdentifyString(), InClass->GetName());
		}
		else
		{
			UE_LOGFMT(LogCacheBox, Error, "UCacheBox::CreateActor {0} TryCreate {1}", GetDebugIdentifyString(), TEXT("Invalid class"));
			return nullptr;
		}

		if (UWorld* World = Utils::Common::GetWorld())
		{
			if (AActor* Actor = World->SpawnActor(InClass))
			{
				if (T* CastedActor = Cast<T>(Actor))
				{
					UE_LOGFMT(LogCacheBox, Log, "UCacheBox::CreateActor {0} Created {1}", GetDebugIdentifyString(), CastedActor->GetName());
					CreatedObjects.Emplace(CastedActor);
					return CastedActor;
				}
			}
		}
		UE_LOGFMT(LogCacheBox, Warning, "UCacheBox::CreateActor {0} Create {1} Failed.", GetDebugIdentifyString(), InClass->GetName());
		return nullptr;
	}

/**
 * Create and hold a UMaterialInstanceDynamic.
 *
 * \return
 */
	 TWeakObjectPtr<UMaterialInstanceDynamic> CreateMaterialInstanceDynamic(class UMaterialInterface* ParentMaterial, class UObject* InOuter);

public:
/**
 * Destroy all managed objects.
 */
	void DestroyAllObjects();

/**
 * Unload all managed objects.
 */
	void UnloadAllObjects();

/**
 * Destroy a single managed object.
 *
 * \param InObject Object to be destroyed
 * \param bPure True for immediate destruction, False to wait for GC
 */
	void DestroyObject(const TWeakObjectPtr<UObject> InObject, const bool bPure = false);
	
/**
 * Unload a managed object.
 *
 * \param InObject Object to be unloaded
 */
	void UnloadObject(TSoftObjectPtr<UObject> InObject);

/**
 * Unload a managed class.
 *
 * \param InObject Object to be unloaded
 */
	void UnloadClass(TSoftClassPtr<UObject> InObject);

	FString GetDebugIdentifyString() const;

private:
	UPROPERTY()
	TSet<UObject*> CreatedObjects;

	UPROPERTY()
	TSet<UObject*> LoadedObjects;

	UPROPERTY()
	TSet<TSoftObjectPtr<UObject>> LoadedSoftObjects;

	UPROPERTY()
	TSet<TSoftClassPtr<UObject>> LoadedSoftClasses;

	FStreamableManager Streamable;
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Component that writes the creation and destruction of CacheBox.
 * If the owner of CacheBox is an actor, you don't have to write the creation of objects every time.
 */
UCLASS()
class EXPANSION_API UCacheBoxComponent : public UActorComponent
{
	GENERATED_BODY()

protected:
	UCacheBoxComponent(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
/** Please use this for all loading and instance processing. */
	UFUNCTION()
	UCacheBox* GetCacheBox()const;

private:
	UFUNCTION()
	void MakeCacheBox();

	UFUNCTION()
	void DestroyCacheBox();

/** Box that holds created instances. */
	UPROPERTY(Transient)
	TObjectPtr<UCacheBox> CacheBox;

	FString GetDebugIdentifyString() const;

};
