// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TaskInformation.h"
#include "DownloadTask.h"
#include "FileDownloadManager.Generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDLManagerDelegate, ETaskEvent, InEvent, FTaskInformation, InInfo);

/**
 * FileDownloadManager, this class is the interface of the plugin, use this class download file as far as possible (both c++ & blueprint)
 */
UCLASS(BlueprintType)
class FILEDOWNLOADER_API UFileDownloadManager : public UObject
{
	GENERATED_BODY()
public:

	UFileDownloadManager();

	UFUNCTION(BlueprintCallable)
		void StartAll();

	UFUNCTION(BlueprintCallable)
		void StartLast();

	UFUNCTION(BlueprintCallable)
		void StartTask(const FGuid& InGuid);

	UFUNCTION(BlueprintCallable)
		void StopAll();

	UFUNCTION(BlueprintCallable)
		void StopTask(const FGuid& InGuid);

	/*save task information to a Json file, so you can load the task later.
	 @Param InGuid can not be invalid, identify a task
	 @Param InFileName figure out the target json file name, you can ignore this param
	*/
	UFUNCTION(BlueprintCallable)
		bool SaveTaskToJsonFile(const FGuid& InGuid, const FString& InFileName);

	/*
	*read a task describe file and assign it to current task
	@Param InFileName can not be empty. (example : "C:/FileDir/task_0.task")
	*/
	UFUNCTION(BlueprintCallable)
		bool ReadTaskFromJsonFile(const FGuid& InGuid, const FString& InFileName);

	UFUNCTION(BlueprintCallable)
		TArray<FTaskInformation> GetAllTaskInformation() const;

	
	/*Add a new task(exist task will be ignored, detected via Guid), first cannot be empty!!!
	 @ param : InUrl cannot be empty!
	 @ param : InDirectory ignore this param(Default directory will be used ../Content/FileDownload) 
   	 @ param : InFileName ignore this param(Default file name will be used, cutting & copy name from InUrl)
	 */
	UFUNCTION(BlueprintCallable)
		FGuid AddTaskByUrl(const FString& InUrl, const FString& InDirectory = TEXT(""), const FString& InFileName = TEXT(""));

	void OnTaskEvent(ETaskEvent InEvent, const FTaskInformation& InInfo);

	UFUNCTION(BlueprintCallable)
		FString GetDownloadDirectory() const;

	UPROPERTY(BlueprintAssignable)
		FDLManagerDelegate OnDlManagerEvent;

	FGuid AddTask(DownloadTask* InTask);


	static FString DefaultDirectory;

protected:

	int32 FindTaskToDo() const;

	int32 FindTaskByGuid(const FGuid& InGuid) const;

	TArray<DownloadTask*> TaskList;

	int32 CurrentDoingWorks = 0;
};
