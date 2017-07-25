// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class IBaseDownloadTask;

/**
 * 
 */
class FILEDOWNLOADER_API FileDownloadManager
{
public:
	FileDownloadManager();
	~FileDownloadManager();

	void StartAllTask();

	void StartLast();

	void StopAll();

	TArray<FTaskInformation> GetAllTaskInformation() const;

	//添加任务，不会添加重复任务(已经在任务队列里)
	bool AddTask(DownloadTask* InTask);

	bool AddTask(const FString& InUrl, const FString& InFileName);

	bool AddTask(const FString& InUrl);

protected:
	TArray<DownloadTask*> TaskList;

	int32 CurrentDoingWorks = 0;
};
