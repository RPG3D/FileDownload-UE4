// Fill out your copyright notice in the Description page of Project Settings.

#include "FileDownloadManager.h"
#include "DownloadTask.h"
#include "Misc/Paths.h"

void UFileDownloadManager::Tick(float DeltaTime)
{
	if (bStopAll)
	{
		return;
	}
	static float TimeCount = 0.f;
	TimeCount += DeltaTime;
	if (TimeCount >= TickInterval)
	{
		TimeCount = 0.f;
		//broadcast event

		//find task to do
		if (CurrentDoingWorks < MaxParallelTask && TaskList.Num())
		{
			int32 Idx = FindTaskToDo();
			if (Idx > INDEX_NONE)
			{
				TaskList[Idx]->Start();
				++CurrentDoingWorks;
			}
		}
	}
}

TStatId UFileDownloadManager::GetStatId() const
{
	return TStatId();
}


void UFileDownloadManager::BeginDestroy()
{
	StopAll();
	Super::BeginDestroy();
}

void UFileDownloadManager::StartAll()
{
	bStopAll = false;

	for (int32 i =0; i < TaskList.Num(); ++i)
	{
		TaskList[i]->SetNeedStop(false);
	}
}

void UFileDownloadManager::StartTask(int32 InGuid)
{
	int32 ret = FindTaskByGuid(InGuid);
	if (ret > INDEX_NONE)
	{
		TaskList[ret]->SetNeedStop(false);
		bStopAll = false;
	}
}

void UFileDownloadManager::StopAll()
{
	for (int32 i = 0; i < TaskList.Num(); ++i)
	{
		TaskList[i]->Stop();
	}

	bStopAll = true;
	CurrentDoingWorks = 0;
}

void UFileDownloadManager::StopTask(int32 InGuid)
{
	int32 ret = FindTaskByGuid(InGuid);
	if (ret >= 0)
	{
		if (TaskList[ret]->GetState() == ETaskState::DOWNLOADING)
		{
			TaskList[ret]->Stop();
			--CurrentDoingWorks;
		}
	}
}

int32 UFileDownloadManager::GetTotalPercent() const
{
	int64 CurrentSize = 0;
	int64 TotalSize = 0;

	for (int32 i = 0; i < TaskList.Num(); ++i)
	{
		CurrentSize += TaskList[i]->GetCurrentSize();
		TotalSize += TaskList[i]->GetTotalSize();
	}

	if (TotalSize < 1)
	{
		return 0;
	}

	return (float)(CurrentSize) / TotalSize * 100.f;
}


void UFileDownloadManager::GetByteSize(int64& OutCurrentSize, int64& OutTotalSize) const
{
	OutCurrentSize = 0;
	OutTotalSize = 0;

	for (int32 i = 0; i < TaskList.Num(); ++i)
	{
		OutCurrentSize += TaskList[i]->GetCurrentSize();
		OutTotalSize += TaskList[i]->GetTotalSize();
	}
}

void UFileDownloadManager::Clear()
{
	StopAll();
	TaskList.Reset();
	ErrorCount = 0;
}

bool UFileDownloadManager::SaveTaskToJsonFile(int32 InGuid, const FString& InFileName /*= TEXT("")*/)
{
	int32 ret = FindTaskByGuid(InGuid);
	if (ret < 0)
	{
		return false;
	}

	return TaskList[ret]->SaveTaskToJsonFile(InFileName);
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

FTaskInformation UFileDownloadManager::GetTaskInfoByGUID(int32 InGUID) const
{
	FTaskInformation Ret;
	for (int32 i = 0; i < TaskList.Num(); ++i)
	{
		if (TaskList[i]->GetTaskInformation().GetGuid() == InGUID)
		{
			Ret = TaskList[i]->GetTaskInformation();
			break;
		}
	}
	return Ret;
}

int32 UFileDownloadManager::AddTaskByUrl(const FString& InUrl, const FString& InDirectory, const FString& InFileName)
{
	FString TmpDir = InDirectory;
	if (TmpDir.IsEmpty())
	{
		//https://www.google.com/
		static int32 URLTag = 8;
		int32 StartSlash = InUrl.Find(FString("/"), ESearchCase::IgnoreCase, ESearchDir::FromStart, URLTag);
		int32 LastSlash = InUrl.Find(FString("/"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		FString UrlDirectory = InUrl.Mid(StartSlash, LastSlash - StartSlash);

		TmpDir = FPaths::ProjectDir() + UrlDirectory;
	}
	TSharedPtr<DownloadTask>Task = MakeShareable(new DownloadTask(InUrl, TmpDir, InFileName));
	Task->ReGenerateGUID();

	for (int32 i = 0; i < TaskList.Num(); ++i)
	{
		if (TaskList[i]->GetSourceUrl() == Task->GetSourceUrl())
		{
			//任务存在于任务列表
			return TaskList[i]->GetGuid();
		}
	}

	Task->ProcessTaskEvent = [this](ETaskEvent InEvent, const FTaskInformation& InInfo, int32 InHpptCode)
	{
		if (this != nullptr)
		{
			this->OnTaskEvent(InEvent, InInfo, InHpptCode);
		}
	};

	TaskList.Add(Task);
	return Task->GetGuid();
}

bool UFileDownloadManager::SetTotalSizeByIndex(int32 InIndex, int32 InTotalSize)
{
	if (InIndex < TaskList.Num() && InTotalSize > 1 && TaskList[InIndex]->GetTotalSize() < 1)
	{
		TaskList[InIndex]->SetTotalSize(InTotalSize);
		return true;
	}

	return false;
}

bool UFileDownloadManager::SetTotalSizeByGuid(int32 InGid, int32 InTotalSize)
{
	int32 Idx = FindTaskByGuid(InGid);
	return SetTotalSizeByIndex(Idx, InTotalSize);
}

void UFileDownloadManager::OnTaskEvent(ETaskEvent InEvent, const FTaskInformation& InInfo, int32 InHttpCode)
{
	OnDlManagerEvent.Broadcast(InEvent, InInfo.GetGuid(), InHttpCode);
	if (InEvent >= ETaskEvent::DOWNLOAD_COMPLETED)
	{
		if (CurrentDoingWorks > 0)
		{
			--CurrentDoingWorks;
		}

		if (InEvent == ETaskEvent::ERROR_OCCUR)
		{
			++ErrorCount;
		}

		if (CurrentDoingWorks < 1)
		{
			OnAllTaskCompleted.Broadcast(ErrorCount);
			ErrorCount = 0;
		}
	}
	return ;
}

int32 UFileDownloadManager::FindTaskToDo() const
{
	int32 ret = INDEX_NONE;
	for (int32 i = 0; i < TaskList.Num(); ++i)
	{
		if (TaskList[i]->GetState() == ETaskState::WAIT && TaskList[i]->GetNeedStop() == false)
		{
			ret = i;
		}
	}

	return ret;
}

int32 UFileDownloadManager::FindTaskByGuid(int32 InGuid) const
{
	int32 ret = INDEX_NONE;
	for (int32 i = 0; i < TaskList.Num(); ++i)
	{
		if (TaskList[i]->GetGuid() == InGuid)
		{
			ret = i;
		}
	}

	return ret;
}

