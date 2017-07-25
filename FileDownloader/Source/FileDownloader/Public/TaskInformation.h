// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TaskInformation.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FTaskInformation
{
	GENERATED_BODY()
	
public:

	bool SerializeToJsonString(FString& OutJsonString) const;

	bool DeserializeFromJsonString(const FString& InJsonString);

	FGuid GetGuid() const
	{
		return GUID;
	}

	UPROPERTY()
		FString FileName = FString("");

	UPROPERTY()
		FString SourceUrl = FString("");

	UPROPERTY()
		FString ETag = FString("");

	UPROPERTY()
		int32 TotalSize = 0;
	UPROPERTY()
		int32 CurrentSize = 0;

protected:
	UPROPERTY()
		FGuid GUID = FGuid::NewGuid();
};
