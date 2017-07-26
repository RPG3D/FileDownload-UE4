// Fill out your copyright notice in the Description page of Project Settings.

#include "TaskInformation.h"
#include "JsonObjectConverter.h"



bool FTaskInformation::SerializeToJsonString(FString& OutJsonString) const
{
	return FJsonObjectConverter::UStructToJsonObjectString(this->StaticStruct(), this, OutJsonString, 0, 0);
}

bool FTaskInformation::DeserializeFromJsonString(const FString& InJsonString)
{
	return FJsonObjectConverter::JsonObjectStringToUStruct(InJsonString, this, 0, 0);
}
