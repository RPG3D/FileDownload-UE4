// Fill out your copyright notice in the Description page of Project Settings.

#include "FileDownloadManager.h"
#include "DownloadTask.h"

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

UFileDownloadManager::UFileDownloadManager()
{

}

void UFileDownloadManager::StartAll()
{
	bStopAll = false;
}

void UFileDownloadManager::StartTask(const FGuid& InGuid)
{
	int32 ret = FindTaskByGuid(InGuid);
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

void UFileDownloadManager::StopTask(const FGuid& InGuid)
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

bool UFileDownloadManager::SaveTaskToJsonFile(const FGuid& InGuid, const FString& InFileName /*= TEXT("")*/)
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

FGuid UFileDownloadManager::AddTaskByUrl(const FString& InUrl, const FString& InDirectory, const FString& InFileName)
{
	FString TmpDir = InDirectory;
	if (TmpDir.IsEmpty())
	{
		TmpDir = FPaths::ProjectDir() + DefaultDirectory;
	}
	TSharedPtr<DownloadTask>Task = MakeShareable(new DownloadTask(InUrl, TmpDir, InFileName));

	if (Task.IsValid() == false)
	{
		FGuid ret;
		ret.Invalidate();
		return ret;
	}

	for (int32 i = 0; i < TaskList.Num(); ++i)
	{
		if (TaskList[i]->GetGuid() == Task->GetGuid())
		{
			//任务存在于任务列表
			return TaskList[i]->GetGuid();
		}
	}

	Task->ProcessTaskEvent = [this](ETaskEvent InEvent, const FTaskInformation& InInfo)
	{
		if (this != nullptr)
		{
			this->OnTaskEvent(InEvent, InInfo);
		}
	};

	TaskList.Add(Task);
	return Task->GetGuid();
}

void UFileDownloadManager::OnTaskEvent(ETaskEvent InEvent, const FTaskInformation& InInfo)
{
	OnDlManagerEvent.Broadcast(InEvent, InInfo);
	if (InEvent >= ETaskEvent::DOWNLOAD_COMPLETED)
	{
		if (MaxParallelTask > 0)
		{
			--MaxParallelTask;
		}

		int32 Idx = FindTaskByGuid(InInfo.GetGuid());
		if (Idx > INDEX_NONE)
		{
			TaskList.RemoveAt(Idx);
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
	int32 ret = INDEX_NONE;
	for (int32 i = 0; i < TaskList.Num(); ++i)
	{
		if (TaskList[i]->GetState() == ETaskState::WAIT && TaskList[i]->GetShouldStop() == false)
		{
			ret = i;
		}
	}

	return ret;
}

int32 UFileDownloadManager::FindTaskByGuid(const FGuid& InGuid) const
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

