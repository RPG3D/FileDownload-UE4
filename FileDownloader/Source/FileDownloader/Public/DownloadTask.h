// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HttpModule.h"
#include "IHttpRequest.h"
#include "IHttpResponse.h"
#include "TaskInformation.h"
#include "DownloadEvent.h"

/**
 * a download task, normally operated by FileDownloadManager, extreamly advise you to use FileDownloadManager.
 */
class FILEDOWNLOADER_API DownloadTask
{
public:
	DownloadTask();

	DownloadTask(const FString& InUrl, const FString& InDirectory, const FString& InFileName);

	virtual ~DownloadTask();

	virtual FString SetFileName(const FString& InFileName);

	virtual FString GetFileName() const;

	virtual FString GetSourceUrl() const;

	virtual FString SetSourceUrl(const FString& InUrl);

	virtual FString SetDirectory(const FString& InDirectory);

	virtual FString GetDirectory() const;

	virtual int32 SetTotalSize(int32 InTotalSize);
	
	virtual int32 GetTotalSize() const;

	virtual int32 SetCurrentSize(int32 InCurrentSize);

	virtual int32 GetCurrentSize() const;

	virtual int32 GetPercentage() const;

	virtual FString SetETag(const FString& ETag);

	virtual FString GetETag() const;

	virtual bool IsTargetFileExist() const;

	virtual bool Start();

	virtual bool Stop();

	FGuid GetGuid() const;

	virtual bool IsDownloading() const;

	FTaskInformation GetTaskInformation() const;

	ETaskState GetState() const;
	
	bool SaveTaskToJsonFile(const FString& InFileName) const;

	bool ReadTaskFromJsonFile(const FString& InFileName);

	//callback for notifying download events
	TFunction<void(ETaskEvent InEvent, const FTaskInformation& InInfo)> ProcessTaskEvent = [this](ETaskEvent InEvent, const FTaskInformation& InInfo) 
	{
		if (InEvent == ETaskEvent::START_DOWNLOAD)
		{
			UE_LOG(LogTemp, Warning, TEXT("%s  %d  Please use FileDownloadManager instead DownloadTask to download file."));
		}
		
	};

protected:

	virtual void GetHead();

	virtual void StartChunk();

	virtual FString GetFullFileName() const;

	virtual void OnGetHeadCompleted(FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bWasSuccessful);
	virtual void OnGetChunkCompleted(FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bWasSuccessful);

	virtual void OnTaskCompleted();

	virtual void OnWriteChunkEnd(int32 WroteSize);

	FTaskInformation TaskInfo;

	FString Directory;

	bool bShouldStop = false;

	ETaskState TaskState = ETaskState::NONE;

	static FString TEMP_FILE_EXTERN;
	static FString TASK_JSON;

	//2MB as one section to download
	const int32 ChunkSize = 2 * 1024 * 1024;

	TArray<uint8> DataBuffer;

	FString TmpFullName;

	IFileHandle* TargetFile = nullptr;

	FHttpRequestPtr Request = nullptr;
};
