// Fill out your copyright notice in the Description page of Project Settings.

#include "DownloadTask.h"
#include "Paths.h"
#include "PlatformFilemanager.h"

FString DownloadTask::TEMP_FILE_EXTERN = TEXT("dlFile");

DownloadTask::DownloadTask()
{
	FString Dir = FPaths::GameContentDir() + TEXT("FileDownload");
	SetDirectory(Dir);
}

DownloadTask::DownloadTask(const FString& InUrl, const FString& InFileName)
{
	FString Dir = FPaths::GameContentDir() + TEXT("FileDownload");
	SetDirectory(Dir);

	TaskInfo.SourceUrl = InUrl;
	SetFileName(InFileName);
}

DownloadTask::DownloadTask(const FString& InUrl)
{
	FString Dir = FPaths::GameContentDir() + TEXT("FileDownload");
	SetDirectory(Dir);

	TaskInfo.SourceUrl = InUrl;
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
	return (int32)((float)TaskInfo.CurrentSize / TaskInfo.TotalSize);
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
	bShouldStop = false;

	if (TaskInfo.SourceUrl.IsEmpty() || TaskInfo.FileName.IsEmpty())
	{
		return false;
	}

	if (IsTargetFileExist() == true)
	{
		return false;
	}

	TaskState = ETaskState::READY;

	if (IsDownloading())
	{
		return true;
	}

	GetHead();

	return true;
}

bool DownloadTask::Stop()
{
	TaskState = ETaskState::STOPED;
	bShouldStop = true;
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

void DownloadTask::GetHead()
{
	TSharedRef<IHttpRequest> DlRequest = FHttpModule::Get().CreateRequest();
	
	DlRequest->SetVerb("HEAD");
	DlRequest->SetURL(GetSourceUrl());
	DlRequest->OnProcessRequestComplete().BindRaw(this, &DownloadTask::OnGetHeadCompleted);
	DlRequest->ProcessRequest();
}

void DownloadTask::OnGetHeadCompleted(FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bWasSuccessful)
{
	int32 RetutnCode = InResponse->GetResponseCode();
	switch (RetutnCode)
	{
	case 404:
		break;
	}

	//the file has updated,need to re-dwonload
	if (InResponse->GetHeader("ETag") != TaskInfo.ETag)
	{
		SetETag(InResponse->GetHeader(FString("ETag")));
		SetTotalSize(InResponse->GetContentLength());
		SetCurrentSize(0);
	}

	if (GetFileName().IsEmpty() == true)
	{
		SetFileName(FPaths::GetCleanFilename(GetSourceUrl()));
	}

	TmpFullName = GetFullFileName() + TEMP_FILE_EXTERN;
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	TargetFile = PlatformFile.OpenWrite(*TmpFullName);
	if (TargetFile != nullptr)
	{
		
		//if (FileHandle->Seek(this->GetTotalSize()) == false)
		{
			//UE_LOG(LogTemp, Warning, TEXT("%s, %d, create temp file error !"));
		}
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
	TSharedRef<IHttpRequest> DlRequest = FHttpModule::Get().CreateRequest();

	DlRequest->SetVerb("GET");
	DlRequest->SetURL(GetSourceUrl());

	int32 StartPostion = GetCurrentSize();
	int32 EndPosition = StartPostion + ChunkSize - 1;
	//lastPosition = TotalSize-1 
	if (EndPosition >= GetTotalSize())
	{
		EndPosition = GetTotalSize();
	}
	FString RangeStr = FString("bytes=") + FString::FromInt(StartPostion) + FString(TEXT("-")) + FString::FromInt(EndPosition);
	DlRequest->AppendToHeader(FString("Range"), RangeStr);

	DlRequest->OnProcessRequestComplete().BindRaw(this, &DownloadTask::OnChunkCompleted);
	DlRequest->ProcessRequest();

}

FString DownloadTask::GetFullFileName() const
{
	return GetDirectory() + TEXT("/") + GetFileName();
}

void DownloadTask::OnChunkCompleted(FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bWasSuccessful)
{
	if (bShouldStop == true)
	{
		return;
	}

	DataBuffer = InResponse->GetContent();
	//write to file async
	
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
				UE_LOG(LogTemp, Warning, TEXT("%s, %d, Async write file error !"));
			}
		}
		/*if (FFileHelper::SaveArrayToFile(this->DataBuffer, *this->TmpFullName, &IFileManager::Get(), EFileWrite::FILEWRITE_Append | EFileWrite::FILEWRITE_EvenIfReadOnly) == true)
		{
			this->OnWriteChunkEnd(this->DataBuffer.Num());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("%s, %d, Async write file error !"));
		}*/
	}
	, TStatId(), nullptr, ENamedThreads::AnyThread);
}

void DownloadTask::OnTaskCompleted()
{
	TaskState = ETaskState::COMPLETED;

	if (TargetFile != nullptr)
	{
		delete TargetFile;
		TargetFile = nullptr;
	}

	//Change file name
	if (IFileManager::Get().Move(*GetFullFileName(), *this->TmpFullName) == true)
	{
;
		
	}
	else
	{
		TaskState = ETaskState::ERROR;

	}
	
}

void DownloadTask::OnWriteChunkEnd(int32 WroteSize)
{
	if (bShouldStop == true)
	{
		return;
	}
	//update progress
	SetCurrentSize(GetCurrentSize() + WroteSize);
	//broadcast event
	if (GetCurrentSize() < GetTotalSize())
	{
		//download next chunk when wrote ended
		StartChunk();
	}
	else
	{
		OnTaskCompleted();
	}
	
}
