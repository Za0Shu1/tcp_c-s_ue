// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Sockets.h"
#include "SocketThread.generated.h"

/**
 * 
 */

DECLARE_LOG_CATEGORY_EXTERN(LogTCPSocketThread, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReceiveSocketDataDelegate, FString, Data); 
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLostConnectionDelegate, USocketThread*, Thread);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnReceiveHeartbeat, USocketThread*, SocketThread);

UCLASS()
class TCP_CS_UE_API USocketThread : public UObject,public FRunnable
{
	GENERATED_BODY()
public:
	FReceiveSocketDataDelegate ReceiveSocketDataDelegate;
	FLostConnectionDelegate	LostConnectionDelegate;
	FOnReceiveHeartbeat ReceiveHeartbeatDelegate;

	void InitializeThread(FSocket* Socket, uint32 SizeSend, uint32 SizeRec);
	void SendData(FString Msg);

	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	virtual void Exit() override;

protected:
	virtual void BeginDestroy() override;

private:
	FSocket* ConnectSocket;
	uint32 SendDataSize;
	uint32 ReceiveDataSize;
	TArray<uint8> ReceiveData;
	TArray<uint8> SendDataBuffer;
	FRunnableThread* pThread;
	bool bThreadStop;
	
};
