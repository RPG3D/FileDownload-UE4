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

	for (auto It :TaskList)
	{
		It.Value->SetNeedStop(false);
	}
}

void UFileDownloadManager::StartTask(int32 InIndex)
{
	if (TaskList.Contains(InIndex))
	{
		TaskList[InIndex]->SetNeedStop(false);
		bStopAll = false;
	}
}

void UFileDownloadManager::StopAll()
{
	for (auto It : TaskList)
	{
		It.Value->Stop();
	}

	bStopAll = true;
	CurrentDoingWorks = 0;
}

void UFileDownloadManager::StopTask(int32 InIndex)
{
	if (TaskList.Contains(InIndex))
	{
		if (TaskList[InIndex]->GetState() == ETaskState::DOWNLOADING)
		{
			TaskList[InIndex]->Stop();
			--CurrentDoingWorks;
		}
	}
}

int32 UFileDownloadManager::GetTotalPercent() const
{
	int64 CurrentSize = 0;
	int64 TotalSize = 0;

	for (const auto It : TaskList)
	{
		CurrentSize += It.Value->GetCurrentSize();
		TotalSize += It.Value->GetTotalSize();
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

	for (const auto It: TaskList)
	{
		OutCurrentSize += It.Value->GetCurrentSize();
		OutTotalSize += It.Value->GetTotalSize();
	}
}

void UFileDownloadManager::Clear()
{
	StopAll();
	TaskList.Reset();
	ErrorCount = 0;
}

bool UFileDownloadManager::SaveTaskToJsonFile(int32 InIndex, const FString& InFileName /*= TEXT("")*/)
{
	if (TaskList.Contains(InIndex) == false)
	{
		return false;
	}

	return TaskList[InIndex]->SaveTaskToJsonFile(InFileName);
}

TArray<FTaskInformation> UFileDownloadManager::GetAllTaskInformation() const
{
	TArray<FTaskInformation> Ret;
	for (auto It: TaskList)
	{
		Ret.Add(It.Value->GetTaskInformation());
	}

	return Ret;
}

FTaskInformation UFileDownloadManager::GetTaskInfo(int32 InIndex) const
{
	FTaskInformation Ret;

	if (TaskList.Contains(InIndex))
	{
		Ret = TaskList[InIndex]->GetTaskInformation();
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

	for (const auto It: TaskList)
	{
		if (It.Value->GetSourceUrl() == InUrl)
		{
			//任务存在于任务列表
			return It.Value->GetGuid();
		}
	}

	TSharedPtr<DownloadTask>Task = MakeShareable(new DownloadTask(InUrl, TmpDir, InFileName));
	Task->ReGenerateGUID();
	Task->ProcessTaskEvent = [this](ETaskEvent InEvent, const FTaskInformation& InInfo, int32 InHpptCode)
	{
		if (this != nullptr)
		{
			this->OnTaskEvent(InEvent, InInfo, InHpptCode);
		}
	};

	TaskList.Add(Task->GetGuid(), Task);
	return Task->GetGuid();
}

bool UFileDownloadManager::SetTotalSizeByIndex(int32 InIndex, int32 InTotalSize)
{
	if (TaskList.Contains(InIndex) && TaskList[InIndex]->GetTotalSize() < 1)
	{
		TaskList[InIndex]->SetTotalSize(InTotalSize);
		return true;
	}

	return false;
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
	for (auto It: TaskList)
	{
		if (It.Value->GetState() == ETaskState::WAIT && It.Value->GetNeedStop() == false)
		{
			ret = It.Key;
		}
	}

	return ret;
}
