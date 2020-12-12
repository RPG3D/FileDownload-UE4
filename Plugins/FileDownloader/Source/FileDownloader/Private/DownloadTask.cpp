// Fill out your copyright notice in the Description page of Project Settings.

#include "DownloadTask.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"
#include "HttpModule.h"
#include "HttpManager.h"
#include "GenericPlatform/GenericPlatformHttp.h"

const FString TEMP_FILE_EXTERN = TEXT(".dlFile");
const FString TASK_JSON = TEXT(".task");

IPlatformFile* PlatformFile = nullptr;


DownloadTask::DownloadTask()
{
	if (PlatformFile == nullptr)
	{
		PlatformFile = &FPlatformFileManager::Get().GetPlatformFile();
		if (PlatformFile->GetLowerLevel())
		{
			PlatformFile = PlatformFile->GetLowerLevel();
		}
	}
}

DownloadTask::DownloadTask(const FString& InUrl, const FString& InDirectory, const FString& InFileName)
	: DownloadTask()
{
	const FString& Dir = InDirectory;

	if (PlatformFile->DirectoryExists(*Dir) == false)
	{
		if (PlatformFile->CreateDirectoryTree(*Dir) == false)
		{
			UE_LOG(LogFileDownloader, Warning, TEXT("Cannot create directory : %s"), *Dir);
		}
	}

	SetDirectory(Dir);
	SetSourceUrl(InUrl);
	SetFileName(InFileName);
}

DownloadTask::DownloadTask(const FTaskInformation& InTaskInfo)
	: DownloadTask()
{
	const FString& Dir = InTaskInfo.DestDirectory;

	if (PlatformFile->DirectoryExists(*Dir) == false)
	{
		if (PlatformFile->CreateDirectoryTree(*Dir) == false)
		{
			UE_LOG(LogFileDownloader, Warning, TEXT("Cannot create directory : %s"), *Dir);
		}
	}

	TaskInfo = InTaskInfo;
}

DownloadTask::~DownloadTask()
{
	Stop();
}

void DownloadTask::SetFileName(const FString& InFileName)
{
	TaskInfo.FileName = InFileName;
}

const FString& DownloadTask::GetFileName() const
{
	return TaskInfo.FileName;
}

const FString& DownloadTask::GetSourceUrl() const
{
	return TaskInfo.SourceUrl;
}

void DownloadTask::SetSourceUrl(const FString& InUrl)
{
	TaskInfo.SourceUrl = InUrl;
}

void DownloadTask::SetDirectory(const FString& InDirectory)
{
	TaskInfo.DestDirectory = InDirectory;
}

const FString& DownloadTask::GetDirectory() const
{
	return TaskInfo.DestDirectory;
}

void DownloadTask::SetTotalSize(int32 InTotalSize)
{
	TaskInfo.TotalSize = InTotalSize;
}

int32 DownloadTask::GetTotalSize() const
{
	return TaskInfo.TotalSize;
}

void DownloadTask::SetCurrentSize(int32 InCurrentSize)
{
	TaskInfo.CurrentSize = InCurrentSize;
}

int32 DownloadTask::GetCurrentSize() const
{
	return TaskInfo.CurrentSize;
}

int32 DownloadTask::GetPercentage() const
{
	int32 Total = TaskInfo.TotalSize;
	if (Total < 1)
	{
		return 0;
	}
	else
	{
		float progress = GetCurrentSize() / GetTotalSize();
		return (int)(progress * 100);
	}
	
}

void DownloadTask::SetETag(const FString& ETag)
{
	TaskInfo.ETag = ETag;
}

const FString& DownloadTask::GetETag() const
{
	return TaskInfo.ETag;
}

bool DownloadTask::Start()
{
	SetNeedStop(false);
	//Set target file name
	if (GetFileName().IsEmpty() == true)
	{
		SetFileName(FPaths::GetCleanFilename(GetSourceUrl()));
	}

	//error occurs!!! URL and file cannot be empty
	if (GetSourceUrl().IsEmpty() || GetFileName().IsEmpty())
	{
		TaskState = ETaskState::ERROR;
		ProcessTaskEvent(ETaskEvent::ERROR_OCCUR, TaskInfo, -1);
		return false;
	}

	//ignore if already being downloading.
	if (IsDownloading())
	{
		return false;
	}


	/*every time we start download(include resume from pause), we should check task information,
	for the remote resource may be changed during pausing*/
	GetHead();

	return true;
}

bool DownloadTask::Stop()
{
	if (Request.IsValid())
	{
		if (Request->OnProcessRequestComplete().IsBound())
		{
			Request->OnProcessRequestComplete().Unbind();
		}

		Request->CancelRequest();
		FHttpModule::Get().GetHttpManager().RemoveRequest(Request.ToSharedRef());
	}

	if (TargetFile)
	{
		delete TargetFile;
		TargetFile = nullptr;
	}

	if (GetState() == ETaskState::DOWNLOADING)
	{
		SetNeedStop(true);	
		return true;
	}

	return false;
}

FGuid DownloadTask::GetGuid() const
{
	return TaskInfo.GetGuid();
}

bool DownloadTask::IsDownloading() const
{
	return TaskState == ETaskState::DOWNLOADING;
}

FTaskInformation DownloadTask::GetTaskInformation() const
{
	return TaskInfo;
}

ETaskState DownloadTask::GetState() const
{
	return TaskState;
}

bool DownloadTask::GetNeedStop() const
{
	return bNeedStop;
}

void DownloadTask::SetNeedStop(bool bStop)
{
	bNeedStop = bStop;
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

void DownloadTask::GetHead()
{
#if PLATFORM_IOS
	Request = FHttpModule::Get().CreateRequest();
#else
	if (Request.IsValid() == false)
	{
		Request = FHttpModule::Get().CreateRequest();
		FHttpModule::Get().GetHttpManager().AddRequest(Request.ToSharedRef());
	}
#endif


	EncodedUrl = GetSourceUrl();

	//https://www.google.com/
	static int32 URLTag = 8;
	int32 StartSlash = GetSourceUrl().Find(FString("/"), ESearchCase::IgnoreCase, ESearchDir::FromStart, URLTag);
	if (StartSlash > INDEX_NONE)
	{
		FString UrlLeft = GetSourceUrl().Left(StartSlash);
		FString UrlRight = GetSourceUrl().Mid(StartSlash);
		TArray<FString> UrlDirectory;
		UrlRight.ParseIntoArray(UrlDirectory, *FString("/"));

		EncodedUrl = UrlLeft;

		for (int32 i = 0; i < UrlDirectory.Num(); ++i)
		{
			UrlDirectory[i] = FGenericPlatformHttp::UrlEncode(UrlDirectory[i]);
			EncodedUrl += FString("/");
			EncodedUrl += UrlDirectory[i];
		}
	}

	Request->SetVerb("HEAD");
	Request->SetURL(EncodedUrl);
	Request->OnProcessRequestComplete().BindRaw(this, &DownloadTask::OnGetHeadCompleted);
	Request->ProcessRequest();

	TaskState = ETaskState::DOWNLOADING;
	ProcessTaskEvent(ETaskEvent::START_DOWNLOAD, TaskInfo, -1);
}

void DownloadTask::OnGetHeadCompleted(FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bWasSuccessful)
{
	//we should check return code first to ensure the URL & network is OK.
	if (InResponse.IsValid() == false || bWasSuccessful == false)
	{
		UE_LOG(LogFileDownloader, Warning, TEXT("%s:%d"), UTF8_TO_TCHAR(__FUNCTION__), __LINE__);

		if (CurrentTryCount >= MaxTryCount)
		{
			TaskState = ETaskState::ERROR;
			ProcessTaskEvent(ETaskEvent::ERROR_OCCUR, TaskInfo, InResponse.IsValid() ? InResponse->GetResponseCode() : 0);
		}
		else
		{
			TaskState = ETaskState::WAIT;
			++CurrentTryCount;

			Start();
		}

		return;
	}
	int32 RetutnCode = InResponse->GetResponseCode();

	if (EHttpResponseCodes::IsOk(RetutnCode) == false)
	{
		UE_LOG(LogFileDownloader, Warning, TEXT("Http return code error : %d"), RetutnCode);
		if (TargetFile != nullptr)
		{
			delete TargetFile;
			TargetFile = nullptr;
		}

		TaskState = ETaskState::ERROR;
		ProcessTaskEvent(ETaskEvent::ERROR_OCCUR, TaskInfo, RetutnCode);
		return;
	}

	if (RetutnCode == 200)
	{
		SetTotalSize(InResponse->GetContentLength());
	}

	if (TargetFile != nullptr)
	{
		delete TargetFile;
		TargetFile = nullptr;
	}
	TargetFile = PlatformFile->OpenWrite(*FString(GetFullFileName() + TEMP_FILE_EXTERN), true);

	if (TargetFile == nullptr)
	{
		UE_LOG(LogFileDownloader, Warning, TEXT("%s, %d, create temp file error !"));
		ProcessTaskEvent(ETaskEvent::ERROR_OCCUR, TaskInfo, RetutnCode);
		TaskState = ETaskState::ERROR;
		return;
	}
	else
	{
		SetCurrentSize(TargetFile->Size());
	}

	FString TempJsonStr;
	FTaskInformation ExistTaskInfo;
	if (FFileHelper::LoadFileToString(TempJsonStr, *FString(GetFullFileName() + TASK_JSON)))
	{
		ExistTaskInfo.DeserializeFromJsonString(TempJsonStr);
	}

	//the remote file has updated,we need to re-download
	FString NewETag = InResponse->GetHeader("ETag");
	SetETag(NewETag);
	if (NewETag.IsEmpty() || NewETag != ExistTaskInfo.ETag)
	{
		SetCurrentSize(0);
	}

	//if target file already exist, make this task complete. 
	bool bExist = PlatformFile->FileExists(*GetFullFileName());
	if (bExist && !NewETag.IsEmpty() && NewETag == ExistTaskInfo.ETag)
	{
		delete TargetFile;
		TargetFile = nullptr;
		PlatformFile->DeleteFile(*FString(GetFullFileName() + TEMP_FILE_EXTERN));

		SetCurrentSize(GetTotalSize());

		OnTaskCompleted();
		return;
	}

	//save task info to disk
	SaveTaskToJsonFile(FString(""));

	StartChunk();
    
}

void DownloadTask::StartChunk()
{
	//start download a chunk
#if PLATFORM_IOS
	Request = FHttpModule::Get().CreateRequest();
#else
	if (Request.IsValid() == false)
	{
		Request = FHttpModule::Get().CreateRequest();
		FHttpModule::Get().GetHttpManager().AddRequest(Request.ToSharedRef());
	}
#endif
    
	Request->SetVerb("GET");
	Request->SetURL(EncodedUrl);

	int32 StartPostion = GetCurrentSize();
	int32 EndPosition = StartPostion + ChunkSize - 1;
	//lastPosition = TotalSize-1 
	if (EndPosition >= GetTotalSize())
	{
		EndPosition = GetTotalSize() - 1;
	}

	if (StartPostion >= EndPosition)
	{
		UE_LOG(LogFileDownloader, Warning, TEXT("Error! StartPostion >= EndPosition"));
		return;
	}

	FString RangeStr = FString("bytes=") + FString::FromInt(StartPostion) + FString(TEXT("-")) + FString::FromInt(EndPosition);
	Request->SetHeader(FString("Range"), RangeStr);

	Request->OnProcessRequestComplete().BindRaw(this, &DownloadTask::OnGetChunkCompleted);
	Request->ProcessRequest();
}

FString DownloadTask::GetFullFileName() const
{
	return GetDirectory() + TEXT("/") + GetFileName();
}

void DownloadTask::OnGetChunkCompleted(FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bWasSuccessful)
{
	if (bNeedStop)
	{
		TaskState = ETaskState::WAIT;
		ProcessTaskEvent(ETaskEvent::STOP, TaskInfo, InResponse->GetResponseCode());

		if (TargetFile)
		{
			delete TargetFile;
			TargetFile = nullptr;
		}
		return;
	}

	if (InResponse.IsValid() == false || bWasSuccessful == false)
	{
		UE_LOG(LogFileDownloader, Warning, TEXT("%s:%d"), UTF8_TO_TCHAR(__FUNCTION__), __LINE__);

		if (CurrentTryCount >= MaxTryCount)
		{
			TaskState = ETaskState::ERROR;
			ProcessTaskEvent(ETaskEvent::ERROR_OCCUR, TaskInfo, InResponse.IsValid() ? InResponse->GetResponseCode() : 0);
		}
		else
		{
			TaskState = ETaskState::WAIT;
			++CurrentTryCount;

			Start();
		}

		return;
	}
	int32 RetCode = InResponse->GetResponseCode();

	if (EHttpResponseCodes::IsOk(RetCode) == false)
	{
		UE_LOG(LogFileDownloader, Warning, TEXT("%s, Return code error: %d"), *GetSourceUrl(), InResponse->GetResponseCode());
		if (TargetFile != nullptr)
		{
			delete TargetFile;
			TargetFile = nullptr;
		}
		TaskState = ETaskState::ERROR;
		ProcessTaskEvent(ETaskEvent::ERROR_OCCUR, TaskInfo, RetCode);
		return;
	}

	DataBuffer = InResponse->GetContent();


#if 0
	//Async write chunk buffer to file 
	Async<void>(EAsyncExecution::ThreadPool, [this]()
	{
		if (this->TargetFile != nullptr)
		{
			this->TargetFile->Seek(this->GetCurrentSize());
			bool bWriteRet = this->TargetFile->Write(DataBuffer.GetData(), DataBuffer.Num());
			if (bWriteRet)
			{
				this->TargetFile->Flush();
				//return to game thread
				FFunctionGraphTask::CreateAndDispatchWhenReady([this]() {
					this->OnWriteChunkEnd(this->DataBuffer.Num());
				}, TStatId(), nullptr, ENamedThreads::GameThread);
			}
			else
			{
				//return to game thread
				FFunctionGraphTask::CreateAndDispatchWhenReady([this]() {
					UE_LOG(LogFileDownloader, Warning, TEXT("%s, %d, Async write file error !"), __FUNCTION__, __LINE__);
					this->TaskState = ETaskState::ERROR;
					this->ProcessTaskEvent(ETaskEvent::ERROR_OCCUR, this->TaskInfo, -1);
				}, TStatId(), nullptr, ENamedThreads::GameThread);

			}
		}
	});

#else

	//Sync write chunk buffer to file 

	if (this->TargetFile != nullptr)
	{
		this->TargetFile->Seek(this->GetCurrentSize());
		bool bWriteRet = this->TargetFile->Write(DataBuffer.GetData(), DataBuffer.Num());
		if (bWriteRet)
		{
			this->TargetFile->Flush();
			this->OnWriteChunkEnd(this->DataBuffer.Num());
		}
		else
		{
			UE_LOG(LogFileDownloader, Warning, TEXT("%s, %d, Sync write file error !"), __FUNCTION__, __LINE__);
			this->TaskState = ETaskState::ERROR;
			this->ProcessTaskEvent(ETaskEvent::ERROR_OCCUR, this->TaskInfo, -1);
		}
	}

#endif
	
}

void DownloadTask::OnTaskCompleted()
{
	//release file handle, so we can change file name via IFileManager.
	if (TargetFile != nullptr)
	{
		delete TargetFile;
		TargetFile = nullptr;
	}

	FString TmpFileName = GetFullFileName() + TEMP_FILE_EXTERN;
	bool bOldExist = PlatformFile->FileExists(*GetFullFileName());
	bool bNewExist = PlatformFile->FileExists(*TmpFileName);

	//error!
	if (!bOldExist && !bNewExist)
	{
		UE_LOG(LogFileDownloader, Warning, TEXT("%s, file not eixst !"), *GetFileName());
		TaskState = ETaskState::ERROR;
		ProcessTaskEvent(ETaskEvent::ERROR_OCCUR, TaskInfo, -1);
		return;
	}

	if (!bOldExist)
	{
		//change temp file name to target file name.
		if (PlatformFile->MoveFile(*GetFullFileName(), *TmpFileName))
		{
			UE_LOG(LogFileDownloader, Warning, TEXT("%s, completed !"), *GetFileName());
			TaskState = ETaskState::COMPLETED;
			ProcessTaskEvent(ETaskEvent::DOWNLOAD_COMPLETED, TaskInfo, -1);
			return;
		}
		else
		{
			//error when changing file name.
			UE_LOG(LogFileDownloader, Warning, TEXT("%s, Change temp file name error !"), *GetFileName());
			TaskState = ETaskState::ERROR;
			ProcessTaskEvent(ETaskEvent::ERROR_OCCUR, TaskInfo, -1);
			return;
		}
	}
	else if (bNewExist)
	{
		if (PlatformFile->DeleteFile(*GetFullFileName()) && PlatformFile->MoveFile(*GetFullFileName(), *TmpFileName))
		{
			TaskState = ETaskState::COMPLETED;
			ProcessTaskEvent(ETaskEvent::DOWNLOAD_COMPLETED, TaskInfo, -1);
			return;
		}
		else
		{
			//error when changing file name.
			UE_LOG(LogFileDownloader, Warning, TEXT("%s, Canot delete old file or rename temp file!"), *GetFileName());
			TaskState = ETaskState::ERROR;
			ProcessTaskEvent(ETaskEvent::ERROR_OCCUR, TaskInfo, -1);
			return;
		}
	}

	TaskState = ETaskState::COMPLETED;
	ProcessTaskEvent(ETaskEvent::DOWNLOAD_COMPLETED, TaskInfo, -1);
	return;
}

void DownloadTask::OnWriteChunkEnd(int32 DataSize)
{
	if (GetState() != ETaskState::DOWNLOADING)
	{
		return;
	}
	//update progress
	SetCurrentSize(GetCurrentSize() + DataSize);

	if (GetCurrentSize() < GetTotalSize())
	{
		ProcessTaskEvent(ETaskEvent::DOWNLOAD_UPDATE, TaskInfo, -1);
		//download next chunk
		StartChunk();
	}
	else
	{
		//task have completed.
		OnTaskCompleted();
	}
	
}
