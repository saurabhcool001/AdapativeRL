// Fill out your copyright notice in the Description page of Project Settings.


#include "VRUDPComponent.h"
#include "SocketSubsystem.h"
#include "Networking.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "VRUDPComponent.h"

// Sets default values for this component's properties
UVRUDPComponent::UVRUDPComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

}


// Called when the game starts
void UVRUDPComponent::BeginPlay()
{
	Super::BeginPlay();

	SetupUDPSocket();
	GetWorld()->GetTimerManager().SetTimer(TickHandle, this, &UVRUDPComponent::TickReceive, 0.05f, true);
	
}

void UVRUDPComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	if (UDPSocket)
	{
		UDPSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(UDPSocket);
		UDPSocket = nullptr;
	}
	GetWorld()->GetTimerManager().ClearTimer(TickHandle);
}

void UVRUDPComponent::SetupUDPSocket()
{
	// Create a new UDP socket
	UDPSocket = FUdpSocketBuilder(TEXT("VRUDPComponent"))
		.AsReusable()
		.WithBroadcast()
		.WithReceiveBufferSize(2 * 1024 * 1024)
		.WithSendBufferSize(2 * 1024 * 1024)
		.BoundToPort(ListenPort);

	TSharedRef<FInternetAddr> Addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	Addr->SetAnyAddress();
	Addr->SetPort(ListenPort);
	UDPSocket->Bind(*Addr);
	UDPSocket->SetNonBlocking(true);

	UE_LOG(LogTemp, Warning, TEXT("[UDP] Listening on port %d"), ListenPort);
	/*if (!UDPSocket)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create UDP socket on port %d"), ListenPort);
		return;
	}
	PythonAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	PythonAddr->SetAnyAddress();
	PythonAddr->SetPort(ListenPort);*/
}

void UVRUDPComponent::TickReceive()
{
	if (!UDPSocket) return;

	uint8 Buffer[1024];
	int32 BytesRead = 0;
	TSharedRef<FInternetAddr> Sender = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();

	while (UDPSocket->RecvFrom(Buffer, sizeof(Buffer), BytesRead, *Sender))
	{
		FString Received = FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(Buffer))).Left(BytesRead);
		UE_LOG(LogTemp, Warning, TEXT("[UDP] Received from Python: %s"), *Received);

		PythonAddr = Sender;

		// Trigger Blueprint Event
		OnMessageReceived.Broadcast(Received);
	}
}

void UVRUDPComponent::SendUDPMessage(const FString& Message)
{
	if (UDPSocket && PythonAddr.IsValid())
	{
		int32 Sent = 0;
		UDPSocket->SendTo((uint8*)TCHAR_TO_UTF8(*Message), Message.Len(), Sent, *PythonAddr);
		UE_LOG(LogTemp, Warning, TEXT("[UDP] Sent to Python: %s"), *Message);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[UDP] Socket not initialized or Python address not set"));
	}
}