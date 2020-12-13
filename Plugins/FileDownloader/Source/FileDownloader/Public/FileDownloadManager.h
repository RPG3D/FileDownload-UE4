// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TaskInformation.h"
#include "Tickable.h"
#include "FileDownloadManager.generated.h"


class DownloadTask;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FDLManagerDelegate, ETaskEvent, InEvent, int32, InTaskID, int32, InHttpCode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAllTaskCompleted, int32, ErrorCount);

/**
 * FileDownloadManager, this class is the interface of the plugin, use this class download file as far as possible (both c++ & blueprint)
 */
UCLASS(BlueprintType)
class FILEDOWNLOADER_API UFileDownloadManager : public UObject, public FTickableGameObject
{
	GENERATED_BODY()
public:

	virtual void BeginDestroy() override;
	/*
	 *start download action for all task by sequence
	 **/
	UFUNCTION(BlueprintCallable)
		void StartAll();

	/*
	 *start a task, only change state, if current works up to MaxDoingWorks, the task is wait
	 **/
	UFUNCTION(BlueprintCallable)
		void StartTask(int32 InGuid);

	/*
	 *stop all task, release file handle and cancel HTTP
	 **/
	UFUNCTION(BlueprintCallable)
		void StopAll();

	/*
	 *stop a task immediately
	 **/
	UFUNCTION(BlueprintCallable)
		void StopTask(int32 InGuid);


	/*
	 *get total progress of all tasks
	 *@return tasks percent [0, 100]
	 **/
	UFUNCTION(BlueprintCallable)
		int32 GetTotalPercent() const;

	UFUNCTION(BlueprintCallable)
		void GetByteSize(int64& OutCurrentSize, int64& OutTotalSize) const;

	/*
	 *stop and remove all tasks
	 **/
	UFUNCTION(BlueprintCallable)
		void Clear();

	/*save task information to a Json file, so you can load the task later.
	 @Param InGuid can not be invalid, identify a task
	 @Param InFileName figure out the target json file name, you can ignore this param
	*/
	UFUNCTION(BlueprintCallable)
		bool SaveTaskToJsonFile(int32 InGuid, const FString& InFileName);

	UFUNCTION(BlueprintCallable)
		TArray<FTaskInformation> GetAllTaskInformation() const;

	UFUNCTION(BlueprintCallable)
		FTaskInformation GetTaskInfoByGUID(int32 InGUID) const;

	
	/*Add a new task(exist task will be ignored, detected via Guid), first cannot be empty!!!
	 @ param : InUrl cannot be empty!
	 @ param : InDirectory ignore this param(Default directory will be used ../Content/FileDownload) 
   	 @ param : InFileName ignore this param(Default file name will be used, cutting & copy name from InUrl)
	 */
	UFUNCTION(BlueprintCallable)
		int32 AddTaskByUrl(const FString& InUrl, const FString& InDirectory = TEXT(""), const FString& InFileName = TEXT(""));

	UFUNCTION(BlueprintCallable)
		bool SetTotalSizeByIndex(int32 InIndex, int32 InTotalSize);

	UFUNCTION(BlueprintCallable)
		bool SetTotalSizeByGuid(int32 InGid, int32 InTotalSize);

	/************************************************************************/
	/* Interface for TickableObject                                         */
	/************************************************************************/
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	
	//tick interval
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float TickInterval = 0.1f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int32 MaxParallelTask = 5;
	UPROPERTY(BlueprintAssignable)
		FDLManagerDelegate OnDlManagerEvent;
	UPROPERTY(BlueprintAssignable)
		FOnAllTaskCompleted OnAllTaskCompleted;

protected:

	void OnTaskEvent(ETaskEvent InEvent, const FTaskInformation& InInfo, int32 InHttpCode);

	int32 FindTaskToDo() const;

	int32 FindTaskByGuid(int32 InGuid) const;

	TArray<TSharedPtr<DownloadTask>> TaskList;

	int32 CurrentDoingWorks = 0;

	bool bStopAll = false;

	int32 ErrorCount = 0;
};
