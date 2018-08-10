// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ModuleManager.h"

class FFileDownloaderModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	//virtual void AddTask(struct FTaskInfomation& InTaskInfo);
};

DECLARE_LOG_CATEGORY_EXTERN(LogFileDownloader, Log, All);