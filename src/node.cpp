#include "node.h"
#include "guid.h"

void Node::ToJson(nlohmann::json& j)
{
	j["id"] = _id;
	j["type"] = _type;
	j["label"] = _label;
	j["x"] = _position.x / NodesGraphSettings::GetDpiScale();
	j["y"] = _position.y / NodesGraphSettings::GetDpiScale();
	j["size_x"] = _size.x / NodesGraphSettings::GetDpiScale();
	j["size_y"] = _size.y / NodesGraphSettings::GetDpiScale();

	nlohmann::json jArraySlots = nlohmann::json::array();
	for (const auto& slot : _slots) {
		nlohmann::json jsonSlot;
		slot->ToJson(jsonSlot);
		jArraySlots.push_back(jsonSlot);
	}

	j["slots"] = jArraySlots;

	_ToJson(j);
}

void Node::FromJson(const nlohmann::json& j)
{
	j.at("id").get_to(_id);
	j.at("type").get_to(_type);
	j.at("label").get_to(_label);
	j.at("x").get_to(_position.x);
	j.at("y").get_to(_position.y);
	j.at("size_x").get_to(_size.x);
	j.at("size_y").get_to(_size.y);

	_position *= NodesGraphSettings::GetDpiScale();
	_size *= NodesGraphSettings::GetDpiScale();

	_recordedPosition = _position;

	nlohmann::json jArraySlots = j["slots"];
	int slotIndex = 0;
	for (const auto& jObjectSlot : jArraySlots) {
		auto slot = _slots[slotIndex];
		slot->FromJson(jObjectSlot);
		slotIndex++;
	}

	_FromJson(j);
}

Node::Node() :
	_isPressed(false),
	_isHovered(false),
	_isSelected(false)
{
	_id = Guid::CreateGuid();
}

void Node::Init()
{
	_Init();
}

Node::~Node()
{
	for (auto& slot : _slots)
		delete slot;

	_slots.clear();
}

void Node::AddSlot(ImVec2 relativePosition, bool isInput, bool isOutput)
{
	auto slot = new NodeSlot(relativePosition, isInput, isOutput);
	_slots.emplace_back(slot);
}

void Node::AddSlot(SlotPosition position, bool isInput, bool isOutput)
{
	ImVec2 relativePosition;
	switch (position) {
	case SlotPosition::Left:
		relativePosition = ImVec2(0, .5);
		break;
	case SlotPosition::Right:
		relativePosition = ImVec2(1, .5);
		break;
	case SlotPosition::Top:
		relativePosition = ImVec2(.5, 0);
		break;
	case SlotPosition::Bottom:
		relativePosition = ImVec2(.5, 1);
		break;
	default:
		return;
	}

	AddSlot(relativePosition, isInput, isOutput);
}

bool Node::Validate()
{
	if (!_outputRequired && !_inputRequired)
		return _Validate();

	if (!_Validate())
		return false;

	auto isConnectedTo = false;
	auto isConnectedFrom = false;

	bool hasInputNodes = false;
	bool hasOutputNodes = false;

	for (const auto& slot : _slots) {
		if (slot->IsInput()) {
			hasInputNodes = true;

			if (slot->IsConnectedTo())
				isConnectedTo = true;
		}
		if (slot->IsOutput()) {
			hasOutputNodes = true;

			if (slot->IsConnectedFrom())
				isConnectedFrom = true;
		}
	}

	if (_outputRequired && hasOutputNodes && !isConnectedFrom)
	{
		_validationMessage = "Node must have at least one output connection.";
		return false;
	}

	if (_inputRequired && hasInputNodes && !isConnectedTo)
	{
		_validationMessage = "Node must have at least one input connection.";
		return false;
	}

	return true;
}

void Node::PreDraw(ImDrawList* drawList)
{
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0);
	ImGui::PushID(_id.c_str());

	ImGui::BeginGroup();
	if (!_label.empty())
		ImGui::Text("%s", _label.c_str());

	_DrawBefore(drawList);
	_Draw(drawList);
	_DrawAfter(drawList);

	ImGui::EndGroup();
	_size = ImGui::GetItemRectSize() + _padding * 2;
	ImGui::PopID();
	ImGui::PopStyleVar();
}

void Node::Draw(ImDrawList* drawList, bool clipDetails)
{
	ImGui::PushID(_id.c_str());

	auto min = ImFloor(_position);
	ImGui::SetCursorScreenPos(min + _padding);

	// Foreground channel
	drawList->ChannelsSetCurrent(1);
	if (!clipDetails)
	{
		ImGui::BeginGroup();
		if (!_label.empty())
			ImGui::Text("%s", _label.c_str());

		_DrawBefore(drawList);
		_Draw(drawList);
		_DrawAfter(drawList);

		ImGui::EndGroup();

		_size = ImGui::GetItemRectSize() + _padding * 2;
	}

	// Background channel
	drawList->ChannelsSetCurrent(0);
	ImGui::SetCursorScreenPos(min);

	auto max = min + _size;

	ImGui::SetNextItemAllowOverlap();
	ImGui::InvisibleButton(_id.c_str(), _size);

	_isHovered = ImGui::IsItemHovered();
	_isPressed = ImGui::IsItemActive();

	auto colorBackground = (_isPressed || _isSelected) ? _colorSelected : _isHovered ? _colorHovered : _colorDefault;
	drawList->AddRectFilled(min, max, colorBackground, 1.0f);
	drawList->AddRect(min, max, _colorOutline, 1.0f, 0, 1_dpi);

	if (NodesGraphSettings::ValidateNodes()) {
		_isValid = Validate();
		if (!_isValid) {

			drawList->AddRect(min, max, ImColor(255, 0, 0, 255), 0, 0, 1_dpi);
			auto labelCenter = min + ImVec2(_size.x, 0);
			drawList->AddCircleFilled(labelCenter, 6.0_dpi, ImColor(255, 0, 0, 255));
			drawList->AddCircle(labelCenter, 6.0_dpi, ImColor(0.06f, 0.06f, 0.06f, 0.94f), 24, 1.5_dpi);
			ImGui::SetCursorScreenPos(labelCenter - ImVec2(6.0_dpi, 6.0_dpi));
			ImGui::InvisibleButton("##error", ImVec2(12.0_dpi, 12.0_dpi));
			_isErrorCircleHovered = ImGui::IsItemHovered();
		}
	}

	ImGui::PopID();
}

Node* Node::Clone()
{
	auto clone = _Clone();
	clone->Init();
	clone->_id = Guid::CreateGuid();
	clone->_type = _type;
	clone->_label = _label;
	clone->_size = _size;
	clone->_position = _position;
	clone->_recordedPosition = _recordedPosition;
	return clone;
}

Node* _GroupNode::Clone()
{
	auto clone = (_GroupNode*)Node::Clone();
	clone->_dummySize = _dummySize;

	for (const auto& node : _nodes) {
		auto childNodeClone = node->Clone();
		clone->_nodes.emplace_back(childNodeClone);
	}
	return clone;
}