// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Common/TcpSocketBuilder.h"
#include "SocketThread.h"
#include "ServerObject.generated.h"

/**
 * 
 */

DECLARE_LOG_CATEGORY_EXTERN(LogTCPServer, Log, All);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FClientConnectDelegate, FString, RemoteIP, int32, RemotePort);


UCLASS(BlueprintType)
class TCP_CS_UE_API UServerObject : public UObject
{
	GENERATED_BODY()
	
public:
	UServerObject(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = NetWork)
		bool CreateTCPServer(const FString& ServerIP, int32 ServerPort, int32 MaxConnection = 8,int32 ReceiveSize = 1024, int32 SendSize = 1024);

	UFUNCTION()
		void ConnectTickCheck();

	UPROPERTY(BlueprintAssignable, VisibleAnywhere, Category = Network)
		FReceiveSocketDataDelegate ReceiveSocketDataDelegate;

	UPROPERTY(BlueprintAssignable, VisibleAnywhere, Category = Network)
		FClientConnectDelegate ClientConnectDelegate;


	UFUNCTION(BlueprintCallable, Category = NetWork)
		void ShutDownServer();

	UFUNCTION(BlueprintCallable, Category = NetWork)
		void SendDataToClient(FString Msg);

protected:
	virtual void BeginDestroy() override;

	UFUNCTION()
		void OnDisConnected(USocketThread* pThread);

	UFUNCTION()
		void OnReceiveHeartbeat(USocketThread* ConnThread);

	UPROPERTY()
		TMap<USocketThread*, int32> SocketOnlineCheckCount;

	UFUNCTION()
		void HeartbeatCheck();
	
private:
	int32 SendDataSize = 1024;
	int32 ReceiveDataSize = 1024;

	// Connect check timer handle
	FTimerHandle ConnectCheckHandle;
	// Heartbeat check timer handle
	FTimerHandle HeartbeatCheckHandle;

	FSocket* Socket;
	FSocket* RecSocket;
	int32 ExpiryCount = 5;// heartbeart check count

	// avoid gc
	UPROPERTY()
		TArray<class USocketThread*> ConnThreads;
};
