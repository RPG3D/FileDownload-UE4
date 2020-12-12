// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TaskInformation.h"
#include "DownloadEvent.h"
#include "FileDownloader.h"
#include "IHttpRequest.h"
#include "IHttpResponse.h"


/**
 * a download task, normally operated by FileDownloadManager, extreamly advise you to use FileDownloadManager.
 */
class DownloadTask
{
public:
	DownloadTask();

	DownloadTask(const FString& InUrl, const FString& InDirectory, const FString& InFileName);

	DownloadTask(const FTaskInformation& InTaskInfo);

	virtual ~DownloadTask();

	virtual void SetFileName(const FString& InFileName);

	virtual const FString& GetFileName() const;

	virtual const FString& GetSourceUrl() const;

	virtual void SetSourceUrl(const FString& InUrl);

	virtual void SetDirectory(const FString& InDirectory);

	virtual const FString& GetDirectory() const;

	virtual void SetTotalSize(int32 InTotalSize);
	
	virtual int32 GetTotalSize() const;

	virtual void SetCurrentSize(int32 InCurrentSize);

	virtual int32 GetCurrentSize() const;

	virtual int32 GetPercentage() const;

	virtual void SetETag(const FString& ETag);

	virtual const FString& GetETag() const;

	virtual bool Start();

	virtual bool Stop();

	FGuid GetGuid() const;

	virtual bool IsDownloading() const;

	FTaskInformation GetTaskInformation() const;

	ETaskState GetState() const;
	
	virtual bool GetNeedStop() const;

	virtual void SetNeedStop(bool bStop);

	bool SaveTaskToJsonFile(const FString& InFileName) const;

	//callback for notifying download events
	TFunction<void(ETaskEvent InEvent, const FTaskInformation& InInfo, int32 InHttpCode)> ProcessTaskEvent = [this](ETaskEvent InEvent, const FTaskInformation& InInfo, int32 InHttpCode)
	{
		if (InEvent == ETaskEvent::START_DOWNLOAD)
		{
			UE_LOG(LogFileDownloader, Warning, TEXT("%s  %d  Please use FileDownloadManager instead DownloadTask to download file."), __FUNCTION__, __LINE__);
		}
		
	};

protected:

	DownloadTask(const DownloadTask& rhs) = delete;
	DownloadTask& operator=(const DownloadTask& rhs) = delete;

	virtual void GetHead();

	virtual void StartChunk();

	virtual FString GetFullFileName() const;

	virtual void OnGetHeadCompleted(FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bWasSuccessful);
	virtual void OnGetChunkCompleted(FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bWasSuccessful);

	virtual void OnTaskCompleted();

	virtual void OnWriteChunkEnd(int32 DataSize);

	FTaskInformation TaskInfo;

	ETaskState TaskState = ETaskState::WAIT;
	
	//2MB as one section to download
	int32 ChunkSize = 2 * 1024 * 1024;

	TArray<uint8> DataBuffer;
	
	FString EncodedUrl;
	
	IFileHandle* TargetFile = nullptr;

	FHttpRequestPtr Request = nullptr;

	bool bNeedStop = false;

	int32 CurrentTryCount = 0;
	int32 MaxTryCount = 5;
};
