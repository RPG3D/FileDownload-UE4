// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
UENUM(BlueprintType)
enum class ETaskEvent : uint8
{
	//start downloading task
	START_DOWNLOAD,
	//Update
	DOWNLOAD_UPDATE,
	//stop
	STOP,
	//download completed
	DOWNLOAD_COMPLETED,
	//meet error during downloading or get task information
	ERROR_OCCUR
};

UENUM(BlueprintType)
enum class ETaskState : uint8
{
	//wait for getting information
	WAIT,
	//is being downloading
	DOWNLOADING,
	//task completed
	COMPLETED,
	//error state
	ERROR
};