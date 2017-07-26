// Fill out your copyright notice in the Description page of Project Settings.

#include "DownloadTask.h"
#include "Paths.h"
#include "FileHelper.h"
#include "PlatformFilemanager.h"

FString DownloadTask::TEMP_FILE_EXTERN = TEXT(".dlFile");
FString DownloadTask::TASK_JSON = TEXT(".task");

DownloadTask::DownloadTask()
{
	FString Dir = FPaths::GameContentDir() + TEXT("FileDownload");
	if (IFileManager::Get().DirectoryExists(*Dir) == false)
	{
		IFileManager::Get().MakeDirectory(*Dir);
	}
	SetDirectory(Dir);
}

DownloadTask::DownloadTask(const FString& InUrl, const FString& InDirectory, const FString& InFileName)
{
	FString Dir = InDirectory;
	if (IFileManager::Get().DirectoryExists(*Dir) == false)
	{
		if (IFileManager::Get().MakeDirectory(*Dir) == false)
		{
			UE_LOG(LogTemp, Warning, TEXT("Cannot create directory : %s"), *Dir);
		}
	}
	SetDirectory(Dir);

	SetSourceUrl(InUrl);
	SetFileName(InFileName);
}

DownloadTask::~DownloadTask()
{
	if (TargetFile != nullptr)
	{
		delete TargetFile;
		TargetFile = nullptr;
	}
}

FString DownloadTask::SetFileName(const FString& InFileName)
{
	TaskInfo.FileName = InFileName;
	return TaskInfo.FileName;
}

FString DownloadTask::GetFileName() const
{
	return TaskInfo.FileName;
}

FString DownloadTask::GetSourceUrl() const
{
	return TaskInfo.SourceUrl;
}

FString DownloadTask::SetSourceUrl(const FString& InUrl)
{
	TaskInfo.SourceUrl = InUrl;
	return GetSourceUrl();
}

FString DownloadTask::SetDirectory(const FString& InDirectory)
{
	Directory = InDirectory;
	return Directory;
}

FString DownloadTask::GetDirectory() const
{
	return Directory;
}

int32 DownloadTask::SetTotalSize(int32 InTotalSize)
{
	TaskInfo.TotalSize = InTotalSize;
	return TaskInfo.TotalSize;
}

int32 DownloadTask::GetTotalSize() const
{
	return TaskInfo.TotalSize;
}

int32 DownloadTask::SetCurrentSize(int32 InCurrentSize)
{
	TaskInfo.CurrentSize = InCurrentSize;
	return TaskInfo.CurrentSize;
}

int32 DownloadTask::GetCurrentSize() const
{
	return TaskInfo.CurrentSize;
}

int32 DownloadTask::GetPercentage() const
{
	int32 Total = TaskInfo.TotalSize;
	if (Total <= 0)
	{
		++Total;
	}

	return (int32)((float)TaskInfo.CurrentSize / Total);
}

FString DownloadTask::SetETag(const FString& ETag)
{
	TaskInfo.ETag = ETag;
	return TaskInfo.ETag;
}

FString DownloadTask::GetETag() const
{
	return TaskInfo.ETag;
}

bool DownloadTask::IsTargetFileExist() const
{
	FString FullFileName = GetDirectory() + TEXT("/") + GetFileName();
	return IFileManager::Get().FileExists(*FullFileName);
}

bool DownloadTask::Start()
{
	//Set target file name
	if (GetFileName().IsEmpty() == true)
	{
		SetFileName(FPaths::GetCleanFilename(GetSourceUrl()));
	}

	bShouldStop = false;

	//error occurs!!! URL and file cannot be empty
	if (TaskInfo.SourceUrl.IsEmpty() || TaskInfo.FileName.IsEmpty())
	{
		TaskState = ETaskState::ERROR;
		ProcessTaskEvent(ETaskEvent::ERROR_OCCUR, TaskInfo);
		return false;
	}

	//if target file already exist, make this task complete. 
	if (IsTargetFileExist() == true)
	{
		OnTaskCompleted();
		return true;
	}

	
	//ignore if already being downloading.
	if (IsDownloading())
	{
		return true;
	}


	/*every time we start download(include resume from pause), we should check task information,
	for the remote resource may be changed during pausing*/
	GetHead();

	return true;
}

bool DownloadTask::Stop()
{
	//release file handle when task stopped.
	if (TargetFile != nullptr)
	{
		delete TargetFile;
		TargetFile = nullptr;
	}

	TaskState = ETaskState::STOPED;
	bShouldStop = true;
	ProcessTaskEvent(ETaskEvent::STOP, TaskInfo);

	return true;
}

FGuid DownloadTask::GetGuid() const
{
	return TaskInfo.GetGuid();
}

bool DownloadTask::IsDownloading() const
{
	return TaskState >= ETaskState::READY && TaskState < ETaskState::COMPLETED && TaskState != ETaskState::STOPED;
}

FTaskInformation DownloadTask::GetTaskInformation() const
{
	return TaskInfo;
}

ETaskState DownloadTask::GetState() const
{
	return TaskState;
}

bool DownloadTask::SaveTaskToJsonFile(const FString& InFileName) const
{
	FString TmpName = InFileName;
	if (TmpName.IsEmpty() == true)
	{
		TmpName = GetFullFileName() + TASK_JSON;
	}
	
	FString OutStr;
	TaskInfo.SerializeToJsonString(OutStr);
	return FFileHelper::SaveStringToFile(OutStr, *TmpName);
}

bool DownloadTask::ReadTaskFromJsonFile(const FString& InFileName)
{
	FString FileData;
	if (FFileHelper::LoadFileToString(FileData, *InFileName) == false)
	{
		return false;
	}

	return TaskInfo.DeserializeFromJsonString(FileData);
}

void DownloadTask::GetHead()
{
	TSharedRef<IHttpRequest> dlRequest = FHttpModule::Get().CreateRequest();

	dlRequest->SetVerb("HEAD");
	dlRequest->SetURL(GetSourceUrl());
	dlRequest->OnProcessRequestComplete().BindRaw(this, &DownloadTask::OnGetHeadCompleted);
	dlRequest->ProcessRequest();

	TaskState = ETaskState::READY;
	ProcessTaskEvent(ETaskEvent::GET_HEAD, TaskInfo);
}

void DownloadTask::OnGetHeadCompleted(FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bWasSuccessful)
{
	//we should check return code first to ensure the URL & network is OK.
	if (InResponse.IsValid() == false || bWasSuccessful == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("OnGetHeadCompleted Response error"));
		ProcessTaskEvent(ETaskEvent::ERROR_OCCUR, TaskInfo);
		return;
	}
	int32 RetutnCode = InResponse->GetResponseCode();

	if (RetutnCode >= 400 || RetutnCode < 200)
	{
		UE_LOG(LogTemp, Warning, TEXT("Http return code error : %d"), RetutnCode);
		if (TargetFile != nullptr)
		{
			delete TargetFile;
			TargetFile = nullptr;
		}
		ProcessTaskEvent(ETaskEvent::ERROR_OCCUR, TaskInfo);
		return;
	}

	//the remote file has updated,we need to re-download
	if (InResponse->GetHeader("ETag") != TaskInfo.ETag)
	{
		SetETag(InResponse->GetHeader(FString("ETag")));
		SetTotalSize(InResponse->GetContentLength());
		SetCurrentSize(0);
	}

	TmpFullName = GetFullFileName() + TEMP_FILE_EXTERN;
	bool bIsTmpFIleExist = IFileManager::Get().FileExists(*TmpFullName);
	if (bIsTmpFIleExist == false)
	{
		SetCurrentSize(0);
	}

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	TargetFile = PlatformFile.OpenWrite(*TmpFullName, true);
	if (TargetFile == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s, %d, create temp file error !"));
		ProcessTaskEvent(ETaskEvent::ERROR_OCCUR, TaskInfo);
		bShouldStop = true;
		return;
	}

	if (bShouldStop == false)
	{
		TaskState = ETaskState::BEING_DOWNLOADING;
		StartChunk();
	}
}

void DownloadTask::StartChunk()
{
	//start download a chunk
	TSharedRef<IHttpRequest> dlRequest = FHttpModule::Get().CreateRequest();
	
	dlRequest->SetVerb("GET");
	dlRequest->SetURL(GetSourceUrl());

	int32 StartPostion = GetCurrentSize();
	int32 EndPosition = StartPostion + ChunkSize - 1;
	//lastPosition = TotalSize-1 
	if (EndPosition >= GetTotalSize())
	{
		EndPosition = GetTotalSize();
	}
	FString RangeStr = FString("bytes=") + FString::FromInt(StartPostion) + FString(TEXT("-")) + FString::FromInt(EndPosition);
	dlRequest->AppendToHeader(FString("Range"), RangeStr);

	dlRequest->OnProcessRequestComplete().BindRaw(this, &DownloadTask::OnGetChunkCompleted);
	dlRequest->ProcessRequest();
}

FString DownloadTask::GetFullFileName() const
{
	return GetDirectory() + TEXT("/") + GetFileName();
}

void DownloadTask::OnGetChunkCompleted(FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bWasSuccessful)
{
	if (bShouldStop == true)
	{
		return;
	}
	if (InResponse.IsValid() == false || bWasSuccessful == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("OnGetHeadCompleted Response error"));
		ProcessTaskEvent(ETaskEvent::ERROR_OCCUR, TaskInfo);
		return;
	}
	int32 RetCode = InResponse->GetResponseCode();

	if (RetCode >= 400 || RetCode < 200)
	{
		UE_LOG(LogTemp, Warning, TEXT("%d, Return code error !"), InResponse->GetResponseCode());
		if (TargetFile != nullptr)
		{
			delete TargetFile;
			TargetFile = nullptr;
		}
		ProcessTaskEvent(ETaskEvent::ERROR_OCCUR, TaskInfo);
		return;
	}

	DataBuffer = InResponse->GetContent();

	//Async write chunk buffer to file 
	FFunctionGraphTask::CreateAndDispatchWhenReady([this]()
	{
		if (this->TargetFile != nullptr)
		{
			this->TargetFile->Seek(this->GetCurrentSize());
			if (this->TargetFile->Write(DataBuffer.GetData(), DataBuffer.Num()) == true)
			{
				this->OnWriteChunkEnd(this->DataBuffer.Num());
				
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("%s, %d, Async write file error !"), __FUNCTION__, __LINE__);
				this->ProcessTaskEvent(ETaskEvent::ERROR_OCCUR, this->TaskInfo);
			}
		}
	}
	, TStatId(), nullptr, ENamedThreads::AnyThread);
}

void DownloadTask::OnTaskCompleted()
{
	//release file handle, so we can change file name via IFileManager.
	if (TargetFile != nullptr)
	{
		delete TargetFile;
		TargetFile = nullptr;
	}

	//Change file name if target file does not exist.
	if (IsTargetFileExist() == false)
	{
		//change temp file name to target file name.
		if (IFileManager::Get().Move(*GetFullFileName(), *this->TmpFullName) == true)
		{
			TaskState = ETaskState::COMPLETED;
			ProcessTaskEvent(ETaskEvent::DOWNLOAD_COMPLETED, TaskInfo);
		}
		else
		{
			//error when changing file name.
			UE_LOG(LogTemp, Warning, TEXT("%s, %d, Change temp file name error !"), __FUNCTION__, __LINE__);
			ProcessTaskEvent(ETaskEvent::ERROR_OCCUR, TaskInfo);
			TaskState = ETaskState::ERROR;
		}
	}
	else
	{
		SetTotalSize(IFileManager::Get().FileSize(*GetFullFileName()));
		if (GetCurrentSize() != GetTotalSize())
		{
			SetCurrentSize(GetTotalSize());
		}

		TaskState = ETaskState::COMPLETED;
		ProcessTaskEvent(ETaskEvent::DOWNLOAD_COMPLETED, TaskInfo);
	}
}

void DownloadTask::OnWriteChunkEnd(int32 WroteSize)
{
	if (bShouldStop == true)
	{
		TaskState = ETaskState::STOPED;
		ProcessTaskEvent(ETaskEvent::STOP, TaskInfo);
		return;
	}
	//update progress
	SetCurrentSize(GetCurrentSize() + WroteSize);
	//broadcast event
	ProcessTaskEvent(ETaskEvent::DOWNLOAD_UPDATE, TaskInfo);

	if (GetCurrentSize() < GetTotalSize())
	{
		//download next chunk when wrote ended
		StartChunk();
	}
	else
	{
		//task have completed.
		OnTaskCompleted();
	}
	
}
