using UnrealBuildTool;
using System.Collections.Generic;

public class BounceBackEditorTarget : TargetRules
{
	public BounceBackEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("BounceBack");
	}
}
