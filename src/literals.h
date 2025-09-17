#pragma once
#include "nodes_graph_settings.h"

inline float operator ""_dpi(unsigned long long int value)
{
	return (float)(value * NodesGraphSettings::GetDpiScale());
}

inline float operator ""_dpi(long double value)
{
	return (float)(value * NodesGraphSettings::GetDpiScale());
}