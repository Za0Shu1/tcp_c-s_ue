// Fill out your copyright notice in the Description page of Project Settings.


#include "ClientObject.h"

DEFINE_LOG_CATEGORY(LogTCPClient);

UClientObject::UClientObject(const FObjectInitializer& ObjectInitializer)
{

}

bool UClientObject::CreateAndConnect(const FString& ServerIP, int32 ServerPort, int32 ReceiveSize /*= 1024*/, int32 SendSize /*= 1024*/)
{
	this->SendDataSize = SendDataSize;
	this->ReceiveDataSize = ReceiveDataSize;

	Socket = FTcpSocketBuilder(TEXT("Client Socket"))
		.AsBlocking()
		.AsReusable()
		.WithReceiveBufferSize(ReceiveDataSize)
		.WithSendBufferSize(SendDataSize);

	if (Socket)
	{
		ConnectServer(ServerIP, ServerPort);
	}
	else
	{
		UE_LOG(LogTCPClient, Error, TEXT("Create Socket Failed!"));
		return false;
	}
	return true;
}

void UClientObject::SendDataToServer(FString Msg)
{
	if (RecThread)
	{
		RecThread->SendData(Msg);
	}
}

void UClientObject::ShutDownClient()
{
	if (Socket)
	{
		if (RecThread)
		{
			RecThread->Stop();
		}

		// Destroy Socket
		Socket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
		Socket = nullptr;
	}
}

void UClientObject::BeginDestroy()
{
	if (!bConnecting)
		ShutDownClient();
	bShutDown = true;

	Super::BeginDestroy();
}

void UClientObject::ConnectServer(FString ServerIP, int32 ServerPort)
{
	serverIP = ServerIP;
	serverPort = ServerPort;
	AsyncTask(ENamedThreads::AnyThread, [&]()
		{
			FIPv4Endpoint ServerEndpoint;
			FIPv4Endpoint::Parse(serverIP, ServerEndpoint);
			TSharedRef<FInternetAddr> Addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
			bool bSuccess = true;
			Addr->SetIp(*serverIP, bSuccess);
			if (!bSuccess)
			{
				UE_LOG(LogTCPClient, Error, TEXT("Server IP %s is invalid!"), *serverIP);
				ConnectResultDelegate.Broadcast(false);
				return;
			}
			Addr->SetPort(serverPort);

			// Connect Server
			if (!bShutDown && Socket->Connect(*Addr))
			{
				RecThread = NewObject<USocketThread>();
				RecThread->ReceiveSocketDataDelegate = ReceiveSocketDataDelegate;
				RecThread->LostConnectionDelegate.AddDynamic(this, &UClientObject::OnDisConnected);
				RecThread->InitializeThread(Socket, SendDataSize, ReceiveDataSize);

				if (ConnectResultDelegate.IsBound())
				{
					ConnectResultDelegate.Broadcast(true);
				}
			}
			else
			{
				ESocketErrors LastErr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLastErrorCode();

				UE_LOG(LogTCPClient, Warning, TEXT("Connect failed with error code (%d) error (%s)"), LastErr, ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetSocketError(LastErr));
				if (!bShutDown && ConnectResultDelegate.IsBound())
				{
					ConnectResultDelegate.Broadcast(false);
				}
			}
			bConnecting = false;
			return;
		});
}

void UClientObject::ReconnectServer()
{
	ShutDownClient();
	Socket = FTcpSocketBuilder(TEXT("Client Socket"))
		.AsBlocking()
		.AsReusable()
		.WithReceiveBufferSize(ReceiveDataSize)
		.WithSendBufferSize(SendDataSize);

	ConnectServer(serverIP,serverPort);
}

void UClientObject::OnDisConnected(USocketThread* pThread)
{
	UE_LOG(LogTCPClient, Warning, TEXT("Disconnect to server."));
	RecThread = nullptr;
	if (ConnectResultDelegate.IsBound())
	{
		ConnectResultDelegate.Broadcast(false);
	}
}