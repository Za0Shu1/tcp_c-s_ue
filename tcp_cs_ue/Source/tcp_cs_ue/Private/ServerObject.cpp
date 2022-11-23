// Fill out your copyright notice in the Description page of Project Settings.


#include "ServerObject.h"

DEFINE_LOG_CATEGORY(LogTCPServer);

UServerObject::UServerObject(const FObjectInitializer& ObjectInitializer)
{

}

bool UServerObject::CreateTCPServer(const FString& ServerIP, int32 ServerPort, int32 MaxConnection /*= 8*/, int32 ReceiveSize /*= 1024*/, int32 SendSize /*= 1024*/)
{
	this->SendDataSize = SendDataSize;
	this->ReceiveDataSize = ReceiveDataSize;

	FIPv4Address ServerAddr;
	if (!FIPv4Address::Parse(ServerIP, ServerAddr))
	{
		UE_LOG(LogTCPServer, Error, TEXT("Server Ip %s is illegal"), *ServerIP);
	}
	Socket = FTcpSocketBuilder(TEXT("Socket Listener"))
		.AsReusable()
		.AsBlocking()
		.BoundToAddress(ServerAddr)
		.BoundToPort(ServerPort)
		.Listening(MaxConnection)
		.WithReceiveBufferSize(SendDataSize)
		.WithSendBufferSize(ReceiveDataSize);

	if (Socket)
	{
		// check connection per seconds
		this->GetWorld()->GetTimerManager().SetTimer(ConnectCheckHandle, this, &UServerObject::ConnectTickCheck, 1, true);
		this->GetWorld()->GetTimerManager().SetTimer(HeartbeatCheckHandle, this, &UServerObject::HeartbeatCheck, 5, true);
	}
	else
	{
		UE_LOG(LogTCPServer, Error, TEXT("Create Socket Failed!"));
		return false;
	}
	return true;
}

void UServerObject::ConnectTickCheck()
{
	bool bPending = false;
	if (Socket->HasPendingConnection(bPending) && bPending)
	{
		TSharedRef<FInternetAddr> RemoteAddress = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
		RecSocket = Socket->Accept(*RemoteAddress, TEXT("Receive Socket"));
		USocketThread* ConnThread = NewObject<USocketThread>(this);
		ConnThreads.Add(ConnThread);
		ConnThread->ReceiveSocketDataDelegate = ReceiveSocketDataDelegate;
		ConnThread->LostConnectionDelegate.AddDynamic(this, &UServerObject::OnDisConnected);
		ConnThread->ReceiveHeartbeatDelegate.AddDynamic(this, &UServerObject::OnReceiveHeartbeat);
		ConnThread->InitializeThread(RecSocket, SendDataSize, ReceiveDataSize);
		ClientConnectDelegate.Broadcast(RemoteAddress->ToString(false), RemoteAddress->GetPort());
	}
	if (!ReceiveSocketDataDelegate.IsBound())
	{
		UE_LOG(LogTCPServer, Warning, TEXT("No ReceiveSocketData Delegate is bound."));
	}
}

void UServerObject::BeginDestroy()
{
	ShutDownServer();
	Super::BeginDestroy();
}

void UServerObject::OnDisConnected(USocketThread* pThread)
{
	UE_LOG(LogTemp, Warning, TEXT("Client lost"));
	ConnThreads.Remove(pThread);
	
}

void UServerObject::OnReceiveHeartbeat(USocketThread* ConnThread)
{
	// reset num
	SocketOnlineCheckCount.Add(ConnThread, 0);
}

void UServerObject::HeartbeatCheck()
{
	if (ConnThreads.Num() == 0) return;

	// 计数自增
	for (TPair<USocketThread*, int32>& element : SocketOnlineCheckCount) {
		++(element).Value;
	}

	// 寻找掉线的客户端
	TArray<USocketThread*> ClearSockets;
	for (TPair<USocketThread*, int32>& element : SocketOnlineCheckCount) {
		if ((element).Value >= ExpiryCount)
		{
			ClearSockets.Add((element).Key);
		}
	}

	// 清理掉线的客户端
	for (auto so : ClearSockets)
	{
		SocketOnlineCheckCount.Remove(so);
		ConnThreads.Remove(so);
		so->Stop();
	}
}

void UServerObject::ShutDownServer()
{
	if (Socket)
	{
		for (auto conn : ConnThreads)
		{
			conn->Stop();
		}
		Socket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
		Socket = nullptr;
	}

	if (RecSocket)
	{
		RecSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(RecSocket);
		RecSocket = nullptr;
	}
}

void UServerObject::SendDataToClient(FString Msg)
{
	for (auto SocketThread : ConnThreads)
	{
		SocketThread->SendData(Msg);
	}
}
