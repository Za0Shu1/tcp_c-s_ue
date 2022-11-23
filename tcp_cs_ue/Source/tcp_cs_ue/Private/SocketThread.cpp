// Fill out your copyright notice in the Description page of Project Settings.


#include "SocketThread.h"
#include "SocketSubsystem.h"
#include <string>

DEFINE_LOG_CATEGORY(LogTCPSocketThread);


void USocketThread::BeginDestroy()
{
	Stop();
	Super::BeginDestroy();
}

bool USocketThread::Init()
{
	return true;
}

uint32 USocketThread::Run()
{
	while (!bThreadStop && ConnectSocket)
	{
		uint32 size;
		bool bLostConnection = false;
		ConnectSocket->HasPendingConnection(bLostConnection);
		ConnectSocket->Wait(ESocketWaitConditions::WaitForReadOrWrite, FTimespan(0, 0, 5));
		if (bLostConnection && !ConnectSocket->HasPendingData(size))
		{
			ReceiveData.Init(0, 100);
			int32 temp;
			if (!ConnectSocket->Recv(ReceiveData.GetData(), 0, temp))
			{
				UE_LOG(LogTCPSocketThread, Warning, TEXT("Connection Lost!"));
				Stop();
				AsyncTask(ENamedThreads::GameThread, [this]()
					{
						LostConnectionDelegate.Broadcast(this);
					});
				continue;
			}
		}

		if (ConnectSocket && ConnectSocket->HasPendingData(size))
		{
			int32 minSize = FMath::Min(size, ReceiveDataSize);
			ReceiveData.Init(0, minSize + 1);
			int32 readBytes;

			if (!ConnectSocket->Recv(ReceiveData.GetData(), minSize, readBytes))
			{
				UE_LOG(LogTCPSocketThread, Warning, TEXT("Connection Lost!"));
				AsyncTask(ENamedThreads::GameThread, [this]()
					{
						LostConnectionDelegate.Broadcast(this);
					});
				continue;
			}

			// convert to string
			FString ReceiveStr = FString(UTF8_TO_TCHAR(ReceiveData.GetData()));
			
			if (ReceiveStr == "HeartbeatCheck")
			{
				if (ReceiveHeartbeatDelegate.IsBound())
				{
					ReceiveHeartbeatDelegate.Broadcast(this);
				}
			}
			else
			{
				if (ReceiveSocketDataDelegate.IsBound())
				{
					AsyncTask(ENamedThreads::GameThread, [=]()
						{
							ReceiveSocketDataDelegate.Broadcast(ReceiveStr);
						});
				}
				else
				{
					UE_LOG(LogTCPSocketThread, Warning, TEXT("No ReceiveSocketData Delegate bound."));
				}
			}
		}
		ReceiveData.Empty();
	}
	return 0;
}

void USocketThread::Stop()
{
	bThreadStop = false;
	ConnectSocket = nullptr;
}

void USocketThread::Exit()
{

}

void USocketThread::InitializeThread(FSocket* Socket, uint32 SizeSend, uint32 SizeRec)
{
	this->ConnectSocket = Socket;
	this->SendDataSize = SizeSend;
	this->ReceiveDataSize = SizeRec;
	FRunnableThread::Create(this, TEXT("Connection Thread"));
}

void USocketThread::SendData(FString Msg)
{
	std::string strSend(TCHAR_TO_UTF8(*Msg));
	SendDataBuffer.Init(0, strSend.size() + 1);
	memcpy(SendDataBuffer.GetData(), strSend.data(), strSend.size());
	int32 sent = 0;
	if (SendDataBuffer.Num() >= (int32)SendDataSize)
	{
		UE_LOG(LogTCPSocketThread, Error, TEXT("Send Data Size is Larger than Max Size for set"));
	}
	else
	{
		if (ConnectSocket && ConnectSocket->Send(SendDataBuffer.GetData(), SendDataBuffer.Num(), sent))
		{
			UE_LOG(LogTCPSocketThread, Warning, TEXT("Send data succeed! msg = %s messageSize = %d sended = %d"), *Msg, SendDataBuffer.Num(), sent);

		}
		else
		{
			UE_LOG(LogTCPSocketThread, Error, TEXT("Send data failed!"));
		}
	}
}
