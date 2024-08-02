/*****************************************************************//**
 * \file   CacheBox.h
 * \brief  A box that records and manages loaded and generated objects collectively
 *
 * \date   07/31/2024
 * \author Lyuro
 *********************************************************************/


#include "CacheBox.h"

#include "LogCategory.h"
#include "Blueprint/UserWidget.h"

DEFINE_LOG_CATEGORY(LogCacheBox);

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * UCacheBox
 */
UCacheBox::UCacheBox(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

void UCacheBox::BeginDestroy()
{
	UObject::BeginDestroy();
	UE_LOGFMT(LogCacheBox, Log, "UCacheBox::BeginDestroy {0}", GetDebugIdentifyString());
	DestroyAllObjects();
	UnloadAllObjects();
}

bool UCacheBox::DestroyBox()
{
	const bool bResult = ConditionalBeginDestroy();
	UE_LOGFMT(LogCacheBox, Log, "UCacheBox::DestroyBox: {0} {1}", GetDebugIdentifyString(), bResult);
	return bResult;
}

void UCacheBox::LoadSynchronous(const TArray<TSoftObjectPtr<UObject>>& SoftObjectArray)
{
	UE_LOGFMT(LogCacheBox, Log, "UCacheBox::LoadSynchronous {0} TryLoad {1} Objects", GetDebugIdentifyString(), SoftObjectArray.Num());

	for (const auto& ObjectToLoad : SoftObjectArray)
	{
		if (ObjectToLoad.IsNull())
		{
			UE_LOGFMT(LogCacheBox, Warning, "UCacheBox::LoadSynchronous {0} TryCreate {1} Invalid Path", GetDebugIdentifyString(), ObjectToLoad.GetAssetName());
			continue;
		}
		LoadSynchronous(ObjectToLoad);
	}
}

void UCacheBox::LoadSynchronous(const TArray<TSoftClassPtr<UObject>>& SoftClassArray)
{
	UE_LOGFMT(LogCacheBox, Log, "UCacheBox::LoadSynchronous {0} TryLoad {1} Classs", GetDebugIdentifyString(), SoftClassArray.Num());

	for (const auto& ClassToLoad : SoftClassArray)
	{
		if (ClassToLoad.IsNull())
		{
			UE_LOGFMT(LogCacheBox, Warning, "UCacheBox::LoadSynchronous {0} TryCreate {1} Invalid Path", GetDebugIdentifyString(), ClassToLoad.GetAssetName());
			continue;
		}
		LoadSynchronous(ClassToLoad);
	}
}

void UCacheBox::RequestAsyncLoad(const TArray<TSoftObjectPtr<UObject>>& SoftObjectArray, const FStreamableDelegate& DelegateToCall)
{
	UE_LOGFMT(LogCacheBox, Log, "UCacheBox::RequestAsyncLoad {0} TryLoad {1} Objects", GetDebugIdentifyString(), SoftObjectArray.Num());

	TArray<FSoftObjectPath> SoftObjectPathTemp;

	for (const auto& ObjectsToLoad : SoftObjectArray)
	{
		if (ObjectsToLoad.IsNull())
		{
			UE_LOGFMT(LogCacheBox, Warning, "UCacheBox::RequestAsyncLoad {0} TryLoad {1} Invalid Path", GetDebugIdentifyString(), ObjectsToLoad.GetAssetName());
			continue;
		}
		SoftObjectPathTemp.Emplace(ObjectsToLoad.ToSoftObjectPath());
	}

	if (!SoftObjectPathTemp.IsEmpty())
	{
		FStreamableDelegate Delegate;
		Delegate.BindWeakLambda(this, [this, DelegateToCall, SoftObjectPathTemp]()
			{
				for (const auto& LoadedObjectPath : SoftObjectPathTemp)
				{
					if (UObject* LoadedObject = Cast<UObject>(LoadedObjectPath.ResolveObject()))
					{
						UE_LOGFMT(LogCacheBox, Log, "UCacheBox::RequestAsyncLoad {0} Loaded {1} Objects", GetDebugIdentifyString(), LoadedObjectPath.ToString());
						LoadedSoftObjects.Emplace(LoadedObject);
						LoadedObjects.Emplace(LoadedObject);
					}
				}
				DelegateToCall.ExecuteIfBound();
			});
		Streamable.RequestAsyncLoad(MoveTemp(SoftObjectPathTemp), MoveTemp(Delegate));
	}
	else
	{
		DelegateToCall.ExecuteIfBound();
	}
}

void UCacheBox::RequestAsyncLoad(const TArray<TSoftClassPtr<UObject>>& SoftClassArray, const FStreamableDelegate& DelegateToCall)
{
	UE_LOGFMT(LogCacheBox, Log, "UCacheBox::RequestAsyncLoad {0} TryLoad {1} Classes", GetDebugIdentifyString(), SoftClassArray.Num());

	TArray<FSoftObjectPath> SoftClassPathTemp;

	for (const auto& ClassToLoad : SoftClassArray)
	{
		if (ClassToLoad.IsNull())
		{
			UE_LOGFMT(LogCacheBox, Warning, "UCacheBox::RequestAsyncLoad {0} TryLoad {1} Invalid Path", GetDebugIdentifyString(), ClassToLoad.GetAssetName());
			continue;
		}
		SoftClassPathTemp.Emplace(ClassToLoad.ToSoftObjectPath());
	}

	if (!SoftClassPathTemp.IsEmpty())
	{
		FStreamableDelegate Delegate;
		Delegate.BindWeakLambda(this, [this, DelegateToCall, SoftClassPathTemp]()
			{
				for (const auto& SoftObjectPath : SoftClassPathTemp)
				{
					if (UObject* LoadedObject = Cast<UObject>(SoftObjectPath.ResolveObject()))
					{
						UE_LOGFMT(LogCacheBox, Log, "UCacheBox::RequestAsyncLoad {0} Loaed {1} Classes", GetDebugIdentifyString(), LoadedObject->GetName());
						LoadedSoftClasses.Emplace(LoadedObject);
						LoadedObjects.Emplace(LoadedObject);
					}
				}
				DelegateToCall.ExecuteIfBound();
			});
		Streamable.RequestAsyncLoad(MoveTemp(SoftClassPathTemp), MoveTemp(Delegate));
	}
	else
	{
		DelegateToCall.ExecuteIfBound();
	}
}


TWeakObjectPtr<UMaterialInstanceDynamic> UCacheBox::CreateMaterialInstanceDynamic(UMaterialInterface* ParentMaterial, UObject* InOuter)
{
	const auto MaterialInstanceDynamic = UMaterialInstanceDynamic::Create(ParentMaterial, InOuter);
	if(ensure(IsValid(MaterialInstanceDynamic)))
	{
		CreatedObjects.Emplace(MaterialInstanceDynamic);
		return MaterialInstanceDynamic;
	}
	return nullptr;
}

void UCacheBox::DestroyAllObjects()
{
	UE_LOGFMT(LogCacheBox, Log, "UCacheBox::DestroyAllObjects");
	/** Identify the type and erase it using the appropriate method */
	for (UObject* CreatedObject : CreatedObjects)
	{
		if (IsValid(CreatedObject))
		{
			if (UUserWidget* UI = Cast<UUserWidget>(CreatedObject))
			{
				UI->RemoveFromParent();
				continue;
			}
			if (AActor* Actor = Cast<AActor>(CreatedObject))
			{
				Actor->Destroy();
				continue;
			}
			CreatedObject->ConditionalBeginDestroy();
		}
	}

	/** Hard reference clear */
	CreatedObjects.Empty();
}

void UCacheBox::UnloadAllObjects()
{
	UE_LOGFMT(LogCacheBox, Log, "UCacheBox::UnloadAllObjects");
	/** Soft Pointer Reset */
	for (TSoftObjectPtr<UObject> LoadedSoftObject : LoadedSoftObjects)
	{
		LoadedSoftObject.Reset();
	}

	/** Soft Pointer Reset */
	for (TSoftClassPtr<UObject> LoadedSoftClass : LoadedSoftClasses)
	{
		LoadedSoftClass.Reset();
	}

	LoadedObjects.Empty();
	LoadedSoftObjects.Empty();
	LoadedSoftClasses.Empty();
}

void UCacheBox::DestroyObject(const TWeakObjectPtr<UObject> InObject, const bool bPure)
{
	/** Delete from Set first */
	CreatedObjects.Remove(InObject.Get());

	/** Run the discard process */
	if (InObject.IsValid())
	{
		UE_LOGFMT(LogCacheBox, Log, "UCacheBox::DestroyObject {0}", InObject->GetName());
		if (UUserWidget* UI = Cast<UUserWidget>(InObject))
		{
			UI->RemoveFromParent();
			return;
		}
		if (AActor* Actor = Cast<AActor>(InObject))
		{
			Actor->Destroy();
			return;
		}
		if (bPure)
		{
			InObject->ConditionalBeginDestroy();
		}
		else
		{
			InObject->MarkAsGarbage();
		}
	}
}

void UCacheBox::UnloadObject(TSoftObjectPtr<UObject> InObject)
{
	if (InObject.IsValid())
	{
		UE_LOGFMT(LogCacheBox, Log, "UCacheBox::UnloadObject {0}", InObject->GetFullName());
		/** Remove the reference from the Set and wait for GC */
		LoadedObjects.Remove(InObject.Get());
		LoadedSoftObjects.Remove(InObject);

		/** Reset the soft pointer */
		InObject.Reset();
	}
}

void UCacheBox::UnloadClass(TSoftClassPtr<UObject> InObject)
{
	if (InObject.IsValid())
	{
		UE_LOGFMT(LogCacheBox, Log, "UCacheBox::UnloadClass {0}", InObject->GetFullName());
		/** Remove the reference from the Set and wait for GC */
		LoadedSoftClasses.Remove(InObject);

		/** Reset the soft pointer */
		InObject.Reset();
	}
}

FString UCacheBox::GetDebugIdentifyString() const
{
	FString Result{};
	if (GetOuter())
	{
		Result += TEXT(" OuterName: ");
		Result += GetOuter()->GetName();
	}
	Result += TEXT(" SelfName: ");
	Result += GetName();
	return Result;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * UCacheBoxComponent
 */
UCacheBoxComponent::UCacheBoxComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCacheBoxComponent::BeginPlay()
{
	UE_LOGFMT(LogCacheBox, Log, "AMvcUiController::MakeCacheBox {0}", GetDebugIdentifyString());
	Super::BeginPlay();
	MakeCacheBox();
}

void UCacheBoxComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UE_LOGFMT(LogCacheBox, Log, "AMvcUiController::MakeCacheBox {0}", GetDebugIdentifyString());
	Super::EndPlay(EndPlayReason);
	DestroyCacheBox();
}

UCacheBox* UCacheBoxComponent::GetCacheBox() const
{
	UE_LOGFMT(LogCacheBox, Verbose, "AMvcUiController::GetCacheBox {0}", GetDebugIdentifyString());
	ensure(CacheBox);
	return CacheBox;
}

void UCacheBoxComponent::MakeCacheBox()
{
	UE_LOGFMT(LogCacheBox, Log, "AMvcUiController::MakeCacheBox {0}", GetDebugIdentifyString());

	CacheBox = NewObject<UCacheBox>(this, UCacheBox::StaticClass());
	ensure(CacheBox);
}

void UCacheBoxComponent::DestroyCacheBox()
{
	UE_LOGFMT(LogCacheBox, Log, "AMvcUiController::DestroyCacheBox {0}", GetDebugIdentifyString());

	if (IsValid(CacheBox))
	{
		CacheBox->DestroyBox();
	}
	CacheBox = nullptr;
}

FString UCacheBoxComponent::GetDebugIdentifyString() const
{
	FString Result{};
	if (GetOuter())
	{
		Result += TEXT("OuterName: ");
		Result += GetOuter()->GetName();
	}
	Result += TEXT("SelfName: ");
	Result += GetName();
	return Result;
}
