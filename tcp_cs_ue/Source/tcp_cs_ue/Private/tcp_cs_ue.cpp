// Copyright Epic Games, Inc. All Rights Reserved.

#include "tcp_cs_ue.h"

#define LOCTEXT_NAMESPACE "Ftcp_cs_ueModule"

void Ftcp_cs_ueModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void Ftcp_cs_ueModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(Ftcp_cs_ueModule, tcp_cs_ue)