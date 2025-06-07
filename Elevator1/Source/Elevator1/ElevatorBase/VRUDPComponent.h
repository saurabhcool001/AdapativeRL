// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sockets.h"
#include "VRUDPComponent.generated.h"
// This class is responsible for handling UDP communication in the VR Elevator system.

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUDPMessageReceived, const FString&, Message);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ELEVATOR1_API UVRUDPComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UVRUDPComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    /** Blueprint Event triggered when message is received from Python */
    UPROPERTY(BlueprintAssignable, Category = "UDP")
    FOnUDPMessageReceived OnMessageReceived;

    /** Port Unreal will listen on (can be set in Editor) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UDP")
    int32 ListenPort = 5005;

    /** Send message to last known Python address */
    UFUNCTION(BlueprintCallable, Category = "UDP")
    void SendUDPMessage(const FString& Message);

private:
    void SetupUDPSocket();
    void TickReceive();

    FSocket* UDPSocket;
    TSharedPtr<FInternetAddr> PythonAddr;
    FTimerHandle TickHandle;
};
