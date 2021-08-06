#pragma once

struct VisibilityCategory
{
	static constexpr int cockpitView = 1;
	static constexpr int virtualCockpitView = 1 << 1;
	static constexpr int externalView = 1 << 2;
};
