// Fill out your copyright notice in the Description page of Project Settings.

#include "FileDownloadManager.h"

FString UFileDownloadManager::DefaultDirectory = TEXT("FileDownload");

UFileDownloadManager::UFileDownloadManager()
{

}

void UFileDownloadManager::StartAll()
{
	for (int32 i = 0; i < TaskList.Num(); ++i)
	{
		TaskList[i]->Start();
	}
}

void UFileDownloadManager::StartLast()
{
	TaskList.Last()->Start();
}

void UFileDownloadManager::StartTask(const FGuid& InGuid)
{
	int32 ret = FindTaskByGuid(InGuid);
	if (ret >= 0)
	{
		TaskList[ret]->Start();
	}
}

void UFileDownloadManager::StopAll()
{
	for (int32 i = 0; i < TaskList.Num(); ++i)
	{
		TaskList[i]->Stop();
	}
}

void UFileDownloadManager::StopTask(const FGuid& InGuid)
{
	int32 ret = FindTaskByGuid(InGuid);
	if (ret >= 0)
	{
		TaskList[ret]->Stop();
	}
}

bool UFileDownloadManager::SaveTaskToJsonFile(const FGuid& InGuid, const FString& InFileName /*= TEXT("")*/)
{
	int32 ret = FindTaskByGuid(InGuid);
	if (ret < 0)
	{
		return false;
	}

	return TaskList[ret]->SaveTaskToJsonFile(InFileName);
}

bool UFileDownloadManager::ReadTaskFromJsonFile(const FGuid& InGuid, const FString& InFileName)
{
	int32 ret = FindTaskByGuid(InGuid);
	if (ret < 0)
	{
		return false;
	}

	return TaskList[ret]->ReadTaskFromJsonFile(InFileName);
}

TArray<FTaskInformation> UFileDownloadManager::GetAllTaskInformation() const
{
	TArray<FTaskInformation> Ret;
	for (int32 i = 0; i < TaskList.Num(); ++i)
	{
		Ret.Add(TaskList[i]->GetTaskInformation());
	}

	return Ret;
}

FGuid UFileDownloadManager::AddTask(DownloadTask* InTask)
{
	if (InTask == nullptr)
	{
		FGuid ret;
		ret.Invalidate();
		return ret;
	}

	for (int32 i = 0; i < TaskList.Num(); ++i)
	{
		if (TaskList[i]->GetGuid() == InTask->GetGuid())
		{
			//任务存在于任务列表
			return TaskList[i]->GetGuid();
		}
	}

	InTask->ProcessTaskEvent = [this](ETaskEvent InEvent, const FTaskInformation& InInfo)
	{
		if (this != nullptr)
		{
			this->OnTaskEvent(InEvent, InInfo);
		}
	};

	TaskList.Add(InTask);
	return InTask->GetGuid();
}

FGuid UFileDownloadManager::AddTaskByUrl(const FString& InUrl, const FString& InDirectory, const FString& InFileName)
{
	FString TmpDir = InDirectory;
	if (TmpDir.IsEmpty())
	{
		TmpDir = FPaths::GameContentDir() + DefaultDirectory;
	}
	return AddTask(new DownloadTask(InUrl, TmpDir, InFileName));
}

void UFileDownloadManager::OnTaskEvent(ETaskEvent InEvent, const FTaskInformation& InInfo)
{
	OnDlManagerEvent.Broadcast(InEvent, InInfo);
	if (InEvent == ETaskEvent::DOWNLOAD_COMPLETED)
	{
		int32 n = FindTaskToDo();
		if (n >= 0)
		{
			TaskList[n]->Start();
		}
	}
	return ;
}

FString UFileDownloadManager::GetDownloadDirectory() const
{
	return DefaultDirectory;
}

int32 UFileDownloadManager::FindTaskToDo() const
{
	int32 ret = -1;
	for (int32 i = 0; i < TaskList.Num(); ++i)
	{
		if (TaskList[i]->GetState() < ETaskState::READY)
		{
			ret = i;
			
		}
	}

	return ret;
}

int32 UFileDownloadManager::FindTaskByGuid(const FGuid& InGuid) const
{
	int32 ret = -1;
	for (int32 i = 0; i < TaskList.Num(); ++i)
	{
		if (TaskList[i]->GetGuid() == InGuid)
		{
			ret = i;
		}
	}

	return ret;
}

