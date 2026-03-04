// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class DialogueMakerTarget : TargetRules
{
	public DialogueMakerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
		ExtraModuleNames.Add("DialogueMaker");
		
		// IWYU 스타일 PCH을 기본값으로: 모듈들의 PCHUsage 기본을 Explicit/Shared로 전환
        bEnforceIWYU = true; // 경고/검사 활성화
        // Engine.h / UnrealEd.h 같은 모놀리식 헤더 포함 시 경고
        bWarnAboutMonolithicHeadersIncluded = true;

        // 이 모듈들만 유니티(합본) 빌드 끄기 — IWYU 누락을 더 잘 잡아냄

        // 최신 include 순서(업그레이드 경고 줄이기)
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

        // 필요 시 전체를 비유니티로
        bUseUnityBuild = false;
	}
}
