// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Common/TcpSocketBuilder.h"
#include "SocketThread.h"
#include "ClientObject.generated.h"

/**
 * 
 */
DECLARE_LOG_CATEGORY_EXTERN(LogTCPClient, Log, All);


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FConnectResultDelegate, bool, bSuccess);


UCLASS(BlueprintType)
class TCP_CS_UE_API UClientObject : public UObject
{
	GENERATED_BODY()

public:
	UClientObject(const FObjectInitializer& ObjectInitializer);
	
	UFUNCTION(BlueprintCallable, Category = NetWork)
		bool CreateAndConnect(const FString& ServerIP,int32 ServerPort,int32 ReceiveSize = 1024, int32 SendSize = 1024);

	UFUNCTION(BlueprintCallable, Category = NetWork)
		void SendDataToServer(FString Msg);

	UFUNCTION(BlueprintCallable, Category = NetWork)
		void ShutDownClient();

	UPROPERTY(BlueprintAssignable, VisibleAnywhere, Category = Network)
		FConnectResultDelegate ConnectResultDelegate;

	UPROPERTY(BlueprintAssignable, VisibleAnywhere, Category = Network)
		FReceiveSocketDataDelegate ReceiveSocketDataDelegate;

protected:
	virtual void BeginDestroy() override;

	UFUNCTION(BlueprintCallable, Category = NetWork)
		void ConnectServer(FString ServerIP,int32 ServerPort);

	UFUNCTION(BlueprintCallable, Category = NetWork)
		void ReconnectServer();

	UFUNCTION()
		void OnDisConnected(USocketThread* pThread);
	
	UFUNCTION()
		void SendHeartbeat();

	UFUNCTION()
		void ConnectTaskComplete(bool bSuccess);
private:
	bool bShutDown = false;
	bool bConnecting = false;
	int32 ReceiveDataSize = 1024;
	int32 SendDataSize = 1024;

	FString serverIP;
	int32 serverPort;

	FSocket* Socket;

	FTimerHandle SendHeartbeatHandle;
	
	// avoid gc
	UPROPERTY()
		USocketThread* RecThread;

	bool bIsReconnecting = false;
	int32 ReconnectCount = 0;// reconnect try count
	int32 ReconnectFailedCount = 12;// reconnect max try count

};
