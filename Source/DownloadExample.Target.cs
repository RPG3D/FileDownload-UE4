// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class DownloadExampleTarget : TargetRules
{
	public DownloadExampleTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;

        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        DefaultBuildSettings = BuildSettingsVersion.V4;

		ExtraModuleNames.AddRange( new string[] { "DownloadExample" } );
	}
}
