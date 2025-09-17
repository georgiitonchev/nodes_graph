#include "node_slot.h"
#include "guid.h"

#include "imgui_internal.h"

NodeSlot::NodeSlot(ImVec2 positionRelative, bool isInput, bool isOutput) :
	_positionRelative(positionRelative),
	_isInput(isInput),
	_isOutput(isOutput)
{
	_id = Guid::CreateGuid();
}

void NodeSlot::ToJson(nlohmann::json& j)
{
	j["id"] = _id;
}

void NodeSlot::FromJson(const nlohmann::json& j)
{
	j.at("id").get_to(_id);
}

void NodeSlot::Draw(ImDrawList* drawList, ImVec2 nodePos, ImVec2 nodeSize, bool isEnabled, bool clipDetails)
{
	_position = ImFloor(nodePos) + nodeSize * _positionRelative;

	if (clipDetails) return;

	drawList->ChannelsSetCurrent(1);

	ImGui::SetCursorScreenPos(_position - ImVec2(_radius * 2, _radius * 2));
	ImGui::InvisibleButton(_id.c_str(), ImVec2(_radius * 4, _radius * 4));

	_isHovered = isEnabled ? ImGui::IsItemHovered() : false;
	_isPressed = isEnabled ? ImGui::IsItemActive() : false;

	auto color = _isPressed ? _colorPressed : _colorDefault;
	auto radius = _radius * (_isHovered ? 1.5f : 1.0f);

	drawList->AddCircleFilled(_position, radius, color);
}