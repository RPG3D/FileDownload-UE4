// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HttpModule.h"
#include "IHttpRequest.h"
#include "IHttpResponse.h"
#include "TaskInformation.h"
#include "DownloadEvent.h"

/**
 * 
 */
class FILEDOWNLOADER_API DownloadTask
{
public:
	DownloadTask();

	DownloadTask(const FString& InUrl);

	DownloadTask(const FString& InUrl, const FString& InFileName);

	~DownloadTask();

	virtual FString SetFileName(const FString& InFileName);

	virtual FString GetFileName() const;

	virtual FString GetSourceUrl() const;

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

	virtual FGuid GetGuid() const;

	virtual bool IsDownloading() const;

	FTaskInformation GetTaskInformation() const;
	
protected:

	virtual void GetHead();

	virtual void StartChunk();

	virtual FString GetFullFileName() const;

	virtual void OnGetHeadCompleted(FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bWasSuccessful);
	virtual void OnChunkCompleted(FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bWasSuccessful);

	virtual void OnTaskCompleted();

	virtual void OnWriteChunkEnd(int32 WroteSize);

	TArray<TSharedRef<class IHttpRequest>> RequestList;

	FTaskInformation TaskInfo;

	FString Directory = FString("");

	bool bShouldStop = false;

	ETaskState TaskState = ETaskState::NONE;

	static FString TEMP_FILE_EXTERN;

	//4MB as one section to download
	const int32 ChunkSize = 2 * 1024 * 1024;

	TArray<uint8> DataBuffer;

	FString TmpFullName;

	IFileHandle* TargetFile = nullptr;
};
