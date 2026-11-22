using UnrealBuildTool;
using System.Collections.Generic;

public class BounceBackTarget : TargetRules
{
	public BounceBackTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("BounceBack");
	}
}
