// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
enum class ETaskEvent
{
	//wait for download, no task information currently
	NONE,
	//start to get download task information
	GET_HEAD,
	//got task information
	GOT_HEAD,
	//start downloading task
	START_DOWNLOAD,
	//Update
	DOWNLOAD_UPDATE,
	//download completed
	DOWNLOAD_COMPLETED,

	//target file exist
	EXIST,
	//meet error during downloading or get task information
	ERROR_OCCUR
};

enum class ETaskState
{
	NONE,
	//wait for getting information
	WAIT,
	//waiting for starting
	READY,
	//is being downloading
	BEING_DOWNLOADING,

	//paused
	STOPED,

	//task completed
	COMPLETED,
	//no need to download
	NO_NEED,
	//error state
	ERROR
};