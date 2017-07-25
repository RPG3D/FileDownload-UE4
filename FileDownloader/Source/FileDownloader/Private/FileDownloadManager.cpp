// Fill out your copyright notice in the Description page of Project Settings.

#include "FileDownloadManager.h"

FileDownloadManager::FileDownloadManager()
{
}

FileDownloadManager::~FileDownloadManager()
{
}

void FileDownloadManager::StartAllTask()
{
	for (int32 i = 0; i < TaskList.Num(); ++i)
	{
		TaskList[i]->Start();
	}
}

void FileDownloadManager::StartLast()
{
	TaskList.Last()->Start();
}

void FileDownloadManager::StopAll()
{
	for (int32 i = 0; i < TaskList.Num(); ++i)
	{
		TaskList[i]->Stop();
	}
}

TArray<FTaskInformation> FileDownloadManager::GetAllTaskInformation() const
{
	TArray<FTaskInformation> Ret;
	for (int32 i = 0; i < TaskList.Num(); ++i)
	{
		Ret.Add(TaskList[i]->GetTaskInformation());
	}

	return Ret;
}

bool FileDownloadManager::AddTask(DownloadTask* InTask)
{
	for (int32 i = 0; i < TaskList.Num(); ++i)
	{
		if (TaskList[i]->GetGuid() == InTask->GetGuid())
		{
			//任务存在于任务列表
			return false;
		}
	}

	TaskList.Add(InTask);
	return true;
}

bool FileDownloadManager::AddTask(const FString& InUrl, const FString& InFileName)
{
	return AddTask(new DownloadTask(InUrl, InFileName));
}

bool FileDownloadManager::AddTask(const FString& InUrl)
{
	return AddTask(new DownloadTask(InUrl));
}


