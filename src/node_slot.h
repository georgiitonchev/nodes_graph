#pragma once

#include "literals.h"

// external
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "json.h"

// std
#include <string>

class NodeSlot {
private:
	ImVec2 _positionRelative;
	ImVec2 _position;

	std::string _id;

	bool _isInput;
	bool _isOutput;

	bool _isHovered = false;
	bool _isPressed = false;

	int _connectionsFrom = 0;
	int _connectionsTo = 0;

	float _radius = 4.0_dpi;
	ImColor _colorDefault = IM_COL32(150, 150, 150, 150);
	ImColor _colorPressed = IM_COL32(224, 224, 224, 150);

public:
	NodeSlot(ImVec2 positionRelative, bool isInput, bool isOutput);
	void Draw(ImDrawList* drawList, ImVec2 nodePos, ImVec2 nodeSize, bool isEnabled, bool clipDetails);

	inline std::string GetId() const { return _id; };
	inline ImVec2 GetRelativePosition() const { return _positionRelative; };
	inline ImVec2 GetPosition() const { return _position; };
	inline bool IsHovered() const { return _isHovered; };
	inline bool IsPressed() const { return _isPressed; };
	inline bool IsInput() const { return _isInput; };
	inline bool IsOutput() const { return _isOutput; };

	inline void AddConnectionFrom() { _connectionsFrom++; }
	inline void AddConnectionTo() { _connectionsTo++; }
	inline void RemoveConnectionFrom() { _connectionsFrom--; }
	inline void RemoveConnectionTo() { _connectionsTo--; }

	inline bool IsConnectedFrom() const { return _connectionsFrom > 0; }
	inline bool IsConnectedTo() const { return _connectionsTo > 0; }

	void ToJson(nlohmann::json& j);
	void FromJson(const nlohmann::json& j);
};