// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TaskInformation.h"
#include "Tickable.h"
#include "FileDownloadManager.Generated.h"


class DownloadTask;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDLManagerDelegate, ETaskEvent, InEvent, FTaskInformation, InInfo);

/**
 * FileDownloadManager, this class is the interface of the plugin, use this class download file as far as possible (both c++ & blueprint)
 */
UCLASS(BlueprintType)
class FILEDOWNLOADER_API UFileDownloadManager : public UObject, public FTickableGameObject
{
	GENERATED_BODY()
public:

	UFileDownloadManager();

	UFUNCTION(BlueprintCallable)
		void StartAll();

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

	UFUNCTION(BlueprintCallable)
		TArray<FTaskInformation> GetAllTaskInformation() const;

	
	/*Add a new task(exist task will be ignored, detected via Guid), first cannot be empty!!!
	 @ param : InUrl cannot be empty!
	 @ param : InDirectory ignore this param(Default directory will be used ../Content/FileDownload) 
   	 @ param : InFileName ignore this param(Default file name will be used, cutting & copy name from InUrl)
	 */
	UFUNCTION(BlueprintCallable)
		FGuid AddTaskByUrl(const FString& InUrl, const FString& InDirectory = TEXT(""), const FString& InFileName = TEXT(""));

	UFUNCTION(BlueprintCallable)
		FString GetDownloadDirectory() const;

	virtual void Tick(float DeltaTime) override;

	virtual TStatId GetStatId() const override;
	
	//tick interval
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float TickInterval = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int32 MaxParallelTask = 5;
	UPROPERTY(BlueprintAssignable)
		FDLManagerDelegate OnDlManagerEvent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FString DefaultDirectory = TEXT("FileDownload");

protected:

	void OnTaskEvent(ETaskEvent InEvent, const FTaskInformation& InInfo);

	int32 FindTaskToDo() const;

	int32 FindTaskByGuid(const FGuid& InGuid) const;

	TArray<TSharedPtr<DownloadTask>> TaskList;

	int32 CurrentDoingWorks = 0;

	bool bStopAll = false;
};
