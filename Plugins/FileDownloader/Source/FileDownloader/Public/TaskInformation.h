// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TaskInformation.generated.h"

/**
 * describe a task's information
 */
USTRUCT(BlueprintType)
struct FTaskInformation
{
	GENERATED_BODY()
	
public:

	bool SerializeToJsonString(FString& OutJsonString) const;

	bool DeserializeFromJsonString(const FString& InJsonString);

	int32 GetGuid() const
	{
		return GUID;
	}

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
		FString FileName = FString("");
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
		FString DestDirectory;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
		FString SourceUrl = FString("");
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
		FString ETag = FString("");

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
		int32 CurrentSize = 0;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
		int32 TotalSize = 0;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
		int32 GUID =0;
};
