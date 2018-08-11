// Fill out your copyright notice in the Description page of Project Settings.

#include "GenerateVSPeoject.h"
#include "FileDownloadManager.h"


UFileDownloadManager* GlobalDownloadMgr = nullptr;

// Sets default values
AGenerateVSPeoject::AGenerateVSPeoject()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AGenerateVSPeoject::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AGenerateVSPeoject::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

