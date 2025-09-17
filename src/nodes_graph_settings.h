#pragma once

class NodesGraphSettings {
private:
	inline static float _dpiScale = 1.0f;
	inline static bool _nodeSnappingEnabled = true;
	inline static int _nodeSnapping = 5;
	inline static bool _validateNodes = false;

public:
	inline static float GetDpiScale() { return _dpiScale; }
	inline static void SetDpiScale(float value) { _dpiScale = value; }

	inline static int NodeSnappingValue() { return _nodeSnapping; }
	inline static int& NodeSnappingValueRef() { return _nodeSnapping; }

	inline static bool NodeSnappingEnabled() { return _nodeSnappingEnabled; }
	inline static bool& NodeSnappingEnabledRef() { return _nodeSnappingEnabled; }

	inline static bool ValidateNodes() { return _validateNodes; }
	inline static bool& ValidateNodesRef() { return _validateNodes; }
};