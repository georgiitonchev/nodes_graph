#include "nodes_graph.h"

// std
#include <fstream>
#include <cmath>

// external
#include <imgui.h>
#include <imgui_internal.h>
#include <json.h>

// commands
#include "commands/create_node_command.h"
#include "commands/create_child_node_command.h"
#include "commands/delete_node_command.h"
#include "commands/delete_child_node_command.h"
#include "commands/move_node_command.h"
#include "commands/move_child_node_command.h"
#include "commands/create_connection_command.h"
#include "commands/delete_connection_command.h"
#include "commands/edit_connection_command.h"
#include "commands/edit_value_command.h"

#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(x, y) (((x) > (y)) ? (x) : (y))
#define clamp(v, min, max) ((v < min) ? min : (v > max) ? max : v)
#define map(v, in_min, in_max, out_min, out_max) \
	((v - in_min) * (out_max - out_min) / (in_max - in_min) + out_min)

const char* NODE_CONTEXT_MENU = "NODE_CONTEXT_MENU";
const char* CHILD_NODE_CONTEXT_MENU = "CHILD_NODE_CONTEXT_MENU";
const char* CANVAS_CONTEXT_MENU = "CANVAS_CONTEXT_MENU";
const char* CONNECTION_CONTEXT_MENU = "CONNECTION_CONTEXT_MENU";

NodesGraph::~NodesGraph()
{
	for (auto& [_, node] : _nodes)
		delete node;
	_nodes.clear();

	for (auto& [_, connection] : _connections)
		delete connection;
	_connections.clear();

	_commands.Clear();
}

void NodesGraph::Draw()
{
	_current = this;

	auto window = ImGui::GetCurrentWindow();
	_windowPos = window->Pos;
	_windowSize = window->Size;

	_drawList = ImGui::GetWindowDrawList();
	_drawList->ChannelsSplit(3);

	BeginCanvas();
	DrawBackground();

	DrawNodes();
	DrawConnections();
	DrawSelection();
	DrawConnectionAttempt();
	HandleNodesDragging();

	_drawList->ChannelsMerge();
	EndCanvas();

	DrawContextMenus();
	DrawOverlay();

	HandleInput();
	HandleCanvasZooming();
}

void NodesGraph::Deserialize(std::string data)
{
	using json = nlohmann::json;

	try {
		std::map<std::string, NodeSlot*> slots;

		json jsonGraph = json::parse(data);
		json jsonArrayNodes = jsonGraph["nodes"];

		for (const auto& jsonNode : jsonArrayNodes)
		{
			std::string id = jsonNode["id"];
			std::string type = jsonNode["type"];

			auto node = CreateNode(type);
			if (!node)
				throw std::runtime_error("Unknown/Unregistered node type: " + type);

			node->FromJson(jsonNode);
			_nodes[id] = node;

			for (auto slot : node->GetSlots())
				slots[slot->GetId()] = slot;

			auto groupNode = dynamic_cast<_GroupNode*>(node);
			if (groupNode != nullptr)
			{
				for (const auto& child : groupNode->GetNodes())
				{
					for (auto slot : child->GetSlots())
						slots[slot->GetId()] = slot;
				}
			}
		}

		json jsonArrayConnections = jsonGraph["connections"];
		for (const auto& jsonConnection : jsonArrayConnections)
		{
			std::string idFrom = jsonConnection["from"];
			std::string idTo = jsonConnection["to"];

			if (slots.find(idFrom) != slots.end() && slots.find(idTo) != slots.end())
			{
				auto slotFrom = static_cast<NodeSlot*>(slots.at(idFrom));
				auto slotTo = static_cast<NodeSlot*>(slots.at(idTo));

				auto connection = new NodeConnection(slotFrom, slotTo);
				connection->FromJson(jsonConnection);

				connection->GetFrom()->AddConnectionFrom();
				connection->GetTo()->AddConnectionTo();

				_connections[connection->GetId()] = connection;
			}
		}

		jsonGraph.at("scale").get_to(_scaleIndex);
		jsonGraph.at("offset_x").get_to(_offset.x);
		jsonGraph.at("offset_y").get_to(_offset.y);

		_offset *= NodesGraphSettings::GetDpiScale();

		_targetScale = _zoomLevels[_scaleIndex];
		_scale = _targetScale;
	}
	catch (const json::parse_error& e) {
	}
	catch (const std::exception& e) {
	}
}

std::string NodesGraph::Serialize()
{
	using json = nlohmann::json;
	json jsonGraph;
	json jsonArrayNodes = json::array();

	for (const auto& [_, node] : _nodes)
	{
		json jsonNode;
		node->ToJson(jsonNode);
		jsonArrayNodes.push_back(jsonNode);
	}

	jsonGraph["nodes"] = jsonArrayNodes;

	json jsonArrayConnections = json::array();

	for (const auto& [_, value] : _connections)
	{
		json jsonConnection;
		value->ToJson(jsonConnection);
		jsonArrayConnections.push_back(jsonConnection);
	}

	jsonGraph["connections"] = jsonArrayConnections;
	jsonGraph["scale"] = _scaleIndex;
	jsonGraph["offset_x"] = _offset.x / NodesGraphSettings::GetDpiScale();
	jsonGraph["offset_y"] = _offset.y / NodesGraphSettings::GetDpiScale();

	// TODO: This shouldn't be here.
	_savedCommandIndex = _commands.CommandIndex();

	return jsonGraph.dump();
}

void NodesGraph::Execute(_Command* command)
{
	_commands.Execute(command);
}

void NodesGraph::Undo()
{
	_commands.Undo();
}

std::vector<_Command*>& NodesGraph::GetUndoStack()
{
	return _commands.GetUndoStack();
}

void NodesGraph::Redo()
{
	_commands.Redo();
}

std::vector<_Command*>& NodesGraph::GetRedoStack()
{
	return _commands.GetRedoStack();
}

bool NodesGraph::HasUnsavedChanges() const
{
	return _savedCommandIndex != _commands.CommandIndex();
}

void NodesGraph::FocusPosition(const ImVec2& position)
{
	_offset = (_windowSize / 2) - position * _scale;
}

void NodesGraph::FocusOnNode(Node* node)
{
	FocusPosition(node->GetPosition() + node->GetSize() / 2);
}

void NodesGraph::DrawBackground() const
{
	if (_scale < .4) return;
	auto color = _colorBackground;
	auto scrollOffset = _offset / _scale;
	auto windowSize = _windowSize / _scale;

	color.Value.w = map(clamp(_scale, .4, 1), 0.4, 1, 0, _colorBackground.Value.w);
	for (float x = -1 - scrollOffset.x + fmodf(scrollOffset.x, _backgroundGridSize); x < windowSize.x - scrollOffset.x + 1; x += _backgroundGridSize)
	{
		for (float y = -1 - scrollOffset.y + fmodf(scrollOffset.y, _backgroundGridSize); y < windowSize.y - scrollOffset.y + 1; y += _backgroundGridSize)
		{
			_drawList->AddCircleFilled(ImVec2(x, y), _backgroundDotSize, color);
		}
	}
}

void NodesGraph::BeginCanvas()
{
	_mousePosBackup = _io.MousePos;
	_mousePosPrevBackup = _io.MousePosPrev;
	_mouseDeltaBackup = _io.MouseDelta;
	_windowCursorMaxBackup = ImGui::GetCurrentWindow()->DC.CursorMaxPos;

	for (auto i = 0; i < IM_ARRAYSIZE(_mouseClickedPosBackup); ++i)
		_mouseClickedPosBackup[i] = _io.MouseClickedPos[i];

	ImVec4 clipRect = _drawList->_ClipRectStack.back();
	auto offset = _windowPos + _offset;

	_drawListCommadBufferSize = ImMax(_drawList->CmdBuffer.Size, 0);
	_drawListStartVertexIndex = _drawList->_VtxCurrentIdx + _drawList->_CmdHeader.VtxOffset;
	_drawListFirstCommandIndex = ImMax(_drawList->CmdBuffer.Size - 1, 0);

	clipRect.x = (clipRect.x - offset.x) / _scale;
	clipRect.y = (clipRect.y - offset.y) / _scale;
	clipRect.z = (clipRect.z - offset.x) / _scale;
	clipRect.w = (clipRect.w - offset.y) / _scale;
	ImGui::PushClipRect(ImVec2(clipRect.x, clipRect.y), ImVec2(clipRect.z, clipRect.w), false);

	_io.MousePos = (_mousePosBackup - offset) / _scale;
	_io.MousePosPrev = (_mousePosPrevBackup - offset) / _scale;
	_io.MouseDelta = _mouseDeltaBackup / _scale;

	for (auto i = 0; i < IM_ARRAYSIZE(_mouseClickedPosBackup); ++i)
		_io.MouseClickedPos[i] = (_mouseClickedPosBackup[i] - offset) / _scale;

	auto& fringeScale = _drawList->_FringeScale;
	_fringeScaleBackup = fringeScale;
	fringeScale /= _scale;
}

void NodesGraph::EndCanvas()
{
	auto vertex = _drawList->VtxBuffer.Data + _drawListStartVertexIndex;
	auto vertexEnd = _drawList->VtxBuffer.Data + _drawList->_VtxCurrentIdx + _drawList->_CmdHeader.VtxOffset;

	auto offset = _windowPos + _offset;

	while (vertex < vertexEnd)
	{
		vertex->pos.x = vertex->pos.x * _scale + offset.x;
		vertex->pos.y = vertex->pos.y * _scale + offset.y;
		++vertex;
	}

	for (int i = _drawListFirstCommandIndex; i < _drawList->CmdBuffer.size(); ++i)
	{
		auto& command = _drawList->CmdBuffer[i];
		command.ClipRect.x = command.ClipRect.x * _scale + offset.x;
		command.ClipRect.y = command.ClipRect.y * _scale + offset.y;
		command.ClipRect.z = command.ClipRect.z * _scale + offset.x;
		command.ClipRect.w = command.ClipRect.w * _scale + offset.y;
	}

	auto& fringeScale = _drawList->_FringeScale;
	fringeScale = _fringeScaleBackup;

	ImGui::PopClipRect();

	_io.MousePos = _mousePosBackup;
	_io.MousePosPrev = _mousePosPrevBackup;
	_io.MouseDelta = _mouseDeltaBackup;
	for (auto i = 0; i < IM_ARRAYSIZE(_mouseClickedPosBackup); ++i)
		_io.MouseClickedPos[i] = _mouseClickedPosBackup[i];

	ImGui::GetCurrentWindow()->DC.CursorMaxPos = _windowCursorMaxBackup;
}

void NodesGraph::DrawNodes()
{
	_validationMessage.clear();

	_hoveredNode = nullptr;
	_hoveredSlot = nullptr;

	_hoveredChildNode = nullptr;
	_hoveredChildNodeParent = nullptr;

	_hasOffscreenNodesLeft = false;
	_hasOffscreenNodesRight = false;
	_hasOffscreenNodesTop = false;
	_hasOffscreenNodesBottom = false;

	for (const auto& [_, node] : _nodes)
	{
		auto nodePosition = node->GetPosition();
		auto nodeSize = node->GetSize();
		auto shouldClip = false;

		auto nodeMin = CanvasToScreen(ImVec2(nodePosition.x, nodePosition.y));
		auto nodeMax = CanvasToScreen(ImVec2(nodePosition.x + nodeSize.x, nodePosition.y + nodeSize.y));

		if (nodeMax.x < _windowPos.x) {
			_hasOffscreenNodesLeft = true;
			shouldClip = true;
		}
		if (nodeMin.x > _windowPos.x + _windowSize.x) {
			_hasOffscreenNodesRight = true;
			shouldClip = true;
		}
		if (nodeMax.y < _windowPos.y) {
			_hasOffscreenNodesTop = true;
			shouldClip = true;
		}
		if (nodeMin.y > _windowPos.y + _windowSize.y) {
			_hasOffscreenNodesBottom = true;
			shouldClip = true;
		}

		auto clipDetails = (nodeSize.x != 0 && shouldClip) || (_scaleIndex <= _scaleIndexClipDetails);

		node->Draw(_drawList, clipDetails);

		if (!node->IsValid() && node->IsValidationCircleHovered())
			_validationMessage = node->GetValidationMessage();

		for (auto& slot : node->GetSlots())
		{
			auto isEnabled = true;
			if (_isEditingConnection && _isEditingConnectionFrom && !slot->IsOutput()) isEnabled = false;
			if (_isEditingConnection && !_isEditingConnectionFrom && !slot->IsInput()) isEnabled = false;
			if (_isDrawingConnection && !slot->IsInput()) isEnabled = false;
			if (!_isDrawingConnection && !slot->IsOutput()) isEnabled = false;

			slot->Draw(_drawList, nodePosition, node->GetSize(), isEnabled, clipDetails);

			if (slot->IsHovered())
				_hoveredSlot = slot;
		}

		auto groupNode = dynamic_cast<_GroupNode*>(node);
		if (groupNode != nullptr)
		{
			int childYPos = 0;
			int childIndex = 0;

			for (auto childNode : groupNode->GetNodes())
			{
				float offsetY = (groupNode->GetSize() - groupNode->GetDummySize()).y - 26_dpi;
				childNode->SetPosition(groupNode->GetPosition() + ImVec2(8_dpi, offsetY + childYPos));
				childNode->Draw(_drawList, clipDetails);

				if (!childNode->IsValid() && childNode->IsValidationCircleHovered())
					_validationMessage = childNode->GetValidationMessage();

				if (childNode->IsPressed() && groupNode->GetNodes().size() > 1)
				{
					_draggedChildNode = childNode;
					_draggedChildNodeParent = groupNode;
					_draggedChildNodeIndex = childIndex;
				}

				if (childNode->IsHovered())
				{
					_hoveredChildNode = childNode;
					_hoveredChildNodeParent = groupNode;
				}

				for (auto& slot : childNode->GetSlots())
				{
					auto isEnabled = true;
					if (_isEditingConnection && _isEditingConnectionFrom && !slot->IsOutput()) isEnabled = false;
					if (_isEditingConnection && !_isEditingConnectionFrom && !slot->IsInput()) isEnabled = false;
					if (_isDrawingConnection && !slot->IsInput()) isEnabled = false;
					if (!_isDrawingConnection && !slot->IsOutput()) isEnabled = false;

					slot->Draw(_drawList, childNode->GetPosition(), childNode->GetSize(), isEnabled, clipDetails);

					if (slot->IsHovered())
						_hoveredSlot = slot;
				}

				childYPos += childNode->GetSize().y + 6_dpi;
				childIndex++;
			}

			auto createdNode = groupNode->GetCreatedNode();
			if (createdNode) {
				createdNode->PreDraw(_drawList);
				Execute(new CreateChildNodeCommand(createdNode, &groupNode->GetNodes()));
			}
		}

		if (node->IsHovered())
			_hoveredNode = node;

		if (_isDrawingSelection)
		{
			// TODO: Extract as a function?
			ImVec2 selectionRectMin;
			ImVec2 selectionRectMax;

			auto mousePosition = ImGui::GetMousePos();

			selectionRectMin.x = min(_drawingSelectionFrom.x, mousePosition.x);
			selectionRectMin.y = min(_drawingSelectionFrom.y, mousePosition.y);
			selectionRectMax.x = max(_drawingSelectionFrom.x, mousePosition.x);
			selectionRectMax.y = max(_drawingSelectionFrom.y, mousePosition.y);

			auto nodePosition = node->GetPosition();
			auto nodeSize = node->GetSize();
			auto nodeRect = ImRect{ nodePosition.x, nodePosition.y, nodePosition.x + nodeSize.x, nodePosition.y + nodeSize.y };
			auto selectionRect = ImRect{ selectionRectMin.x, selectionRectMin.y, selectionRectMax.x, selectionRectMax.y };
			auto isOverlappedBySelection = selectionRect.Overlaps(nodeRect);

			if (isOverlappedBySelection)
			{
				if (_selectedNodes.find(node) == _selectedNodes.end())
					_selectedNodes.insert(node);
			}
			else if (!ImGui::IsKeyDown(ImGuiKey_LeftCtrl))
			{
				if (_selectedNodes.find(node) != _selectedNodes.end())
					_selectedNodes.erase(node);
			}

			if (isOverlappedBySelection)
				node->SetIsSelected(true);
			else if (!ImGui::IsKeyDown(ImGuiKey_LeftCtrl))
				node->SetIsSelected(false);
		}
		if (!_isDraggingNodes && node->IsPressed() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
		{
			if (!node->IsSelected())
			{
				//for (auto selectedNode : _selectedNodes)
				//	selectedNode->SetIsSelected(false);

				//_selectedNodes.clear();
				_draggedNode = node;
				_draggedNodePos = node->GetPosition();
			}

			_isDraggingNodes = true;
		}

		if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && node->IsHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {

			if (node->IsSelected()) {
				if (_selectedNodes.find(node) != _selectedNodes.end())
					_selectedNodes.erase(node);

				node->SetIsSelected(false);
			}
			else {
				if (_selectedNodes.find(node) == _selectedNodes.end())
					_selectedNodes.insert(node);

				node->SetIsSelected(true);
			}
		}
	}

	// TODO: Move out
	if (_hoveredSlot) {
		if (_hoveredSlot->IsPressed()) {
			_isDrawingConnection = true;
			_drawingConnectionFrom = _hoveredSlot;

			for (const auto& selectedNode : _selectedNodes)
				selectedNode->SetIsSelected(false);

			_selectedNodes.clear();
			ImGui::ClearActiveID();
		}
	}

	// TODO: Move out
	if (_draggedChildNode)
	{
		auto rectSize = _draggedChildNode->GetSize();
		auto rectPos = _draggedChildNode->GetPosition() - ImVec2(0, 3_dpi);
		auto newPlaceIndex = _draggedChildNodeIndex;

		int childNodeIndex = 0;
		for (auto childNode : _draggedChildNodeParent->GetNodes())
		{
			auto mousePos = ImGui::GetMousePos();
			auto childNodePos = childNode->GetPosition();

			if (mousePos.y > childNodePos.y)
			{
				rectPos = childNodePos - ImVec2(0, 3_dpi);
				newPlaceIndex = childNodeIndex;
			}

			childNodeIndex++;
		}

		_drawList->ChannelsSetCurrent(2);
		_drawList->AddLine(rectPos, rectPos + ImVec2(rectSize.x, 0), IM_COL32(127, 24, 247, 255), 3_dpi);

		if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
		{
			if (_draggedChildNodeIndex != newPlaceIndex)
				Execute(new MoveChildNodeCommand(_draggedChildNode, _draggedChildNodeParent->GetNodes(), _draggedChildNodeIndex, newPlaceIndex));

			_draggedChildNode = nullptr;
		}
	}
}

void NodesGraph::HandleNodesDragging()
{
	if (_isDraggingNodes)
	{
		if (_draggedNode != nullptr) {
			_draggedNodePos += _io.MouseDelta;
			auto pos = _draggedNodePos;

			if (NodesGraphSettings::NodeSnappingEnabled() && !ImGui::IsKeyDown(ImGuiKey_LeftAlt)) {
				auto snap = NodesGraphSettings::NodeSnappingValue();
				pos = ImVec2(((int)(_draggedNodePos.x + 2) / snap) * snap, ((int)(_draggedNodePos.y + 2) / snap) * snap);
			}

			_draggedNode->SetPosition(pos);
		}
		else {
			for (auto& node : _selectedNodes)
				node->SetPosition(node->GetPosition() + _io.MouseDelta);
		}

		if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
		{
			_isDraggingNodes = false;

			if (_draggedNode != nullptr)
			{
				_commands.Execute(new MoveNodeCommand(_draggedNode, _draggedNode->GetPosition()));
				_draggedNode = nullptr;
			}
			else
			{
				auto command = new CommandCluster("Move Nodes");
				for (const auto& n : _selectedNodes)
					command->Add(new MoveNodeCommand(n, n->GetPosition()));

				_commands.Execute(command);
			}
		}
	}
}

void NodesGraph::HandleCanvasZooming()
{
	if (ImGui::IsWindowHovered() && ImGui::IsKeyDown(ImGuiKey_MouseWheelY)) {
		_scaleIndex += (_io.MouseWheel > 0 ? 1 : -1);
		_scaleIndex = clamp(_scaleIndex, 0, 12);

		_targetScale = _zoomLevels[_scaleIndex];
		_scalePosition = ScreenToCanvas(_io.MousePos);
	}

	auto lerp = [](float a, float b, float t) -> float {
		return a + (b - a) * t;
		};

	auto scaleBefore = _scale;
	_scale = lerp(_scale, _targetScale, .33f);

	auto scaleChange = scaleBefore - _scale;
	_offset += _scalePosition * scaleChange;
}

void NodesGraph::DrawConnections()
{
	_hoveredConnection = nullptr;

	for (const auto& [_, connection] : _connections) {
		if (_isEditingConnection && _clickedConnection == connection) continue;
		auto clipDetails = (_scaleIndex <= _scaleIndexClipDetails);
		connection->Draw(_drawList, clipDetails);

		if (connection->IsHovered()) {
			_hoveredConnection = connection;

			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
				_clickedConnection = connection;
			}
		}
	}

	// TODO: Move out.
	if (!_isEditingConnection && !_isDrawingConnection && _clickedConnection && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
	{
		_isEditingConnection = true;

		auto pointsDistanceSquared = [](ImVec2 p1, ImVec2 p2) -> float {
			return pow(p2.x - p1.x, 2) + pow(p2.y - p1.y, 2);
			};

		auto distanceToSquared = pointsDistanceSquared(ImGui::GetMousePos(), _clickedConnection->GetTo()->GetPosition());
		auto distanceFromSquared = pointsDistanceSquared(ImGui::GetMousePos(), _clickedConnection->GetFrom()->GetPosition());

		_isEditingConnectionFrom = distanceFromSquared < distanceToSquared;
	}

	if (_isEditingConnection)
	{
		if (_isEditingConnectionFrom)
		{
			_drawList->AddCircleFilled(ImGui::GetMousePos(), 3.0f, IM_COL32(164, 164, 164, 255));
			_drawList->AddLine(ImGui::GetMousePos(), _clickedConnection->GetTo()->GetPosition(), IM_COL32(164, 164, 164, 255), 2.0f);
		}
		else
		{
			_drawList->AddCircleFilled(_clickedConnection->GetFrom()->GetPosition(), 3.0f, IM_COL32(164, 164, 164, 255));
			_drawList->AddLine(_clickedConnection->GetFrom()->GetPosition(), ImGui::GetMousePos(), IM_COL32(164, 164, 164, 255), 2.0f);
		}

		if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
		{
			if (_hoveredSlot)
			{
				auto to = _isEditingConnectionFrom ? _clickedConnection->GetTo() : _hoveredSlot;
				auto from = _isEditingConnectionFrom ? _hoveredSlot : _clickedConnection->GetFrom();

				if (_clickedConnection->GetTo() != to || _clickedConnection->GetFrom() != from)
				{
					_commands.Execute(new EditConnectionCommand(_clickedConnection, from, to));
				}
			}
			else
				_commands.Execute(new DeleteConnectionCommand(_clickedConnection, &_connections));

			_clickedConnection = nullptr;
			_isEditingConnection = false;
		}
	}

	if (_clickedConnection && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
		_clickedConnection = nullptr;
}

void NodesGraph::DrawContextMenus()
{
	if (ImGui::IsWindowHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
	{
		if (_hoveredNode) {
			ImGui::OpenPopup(NODE_CONTEXT_MENU);
			_focusedNode = _hoveredNode;
		}
		else if (_hoveredChildNode) {
			ImGui::OpenPopup(CHILD_NODE_CONTEXT_MENU);
			_focusedChildNode = _hoveredChildNode;
			_focusedChildNodeParent = _hoveredChildNodeParent;
		}
		else if (_hoveredConnection) {
			ImGui::OpenPopup(CONNECTION_CONTEXT_MENU);
			_focusedConnection = _hoveredConnection;
		}
		else {
			ImGui::OpenPopup(CANVAS_CONTEXT_MENU);
		}
	}

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));

	if (ImGui::BeginPopup(CANVAS_CONTEXT_MENU))
	{
		auto canvasPos = ScreenToCanvas(ImGui::GetMousePosOnOpeningCurrentPopup());

		if (ImGui::BeginMenu("Add"))
		{
			for (const auto& [key, fn] : _nodesRegistry)
			{
				if (ImGui::MenuItem(key.c_str()))
				{
					auto node = fn(canvasPos);
					node->PreDraw(_drawList);
					Execute(new CreateNodeCommand(node, &_nodes));
				}
			}
			ImGui::EndMenu();
		}

		ImGui::BeginDisabled(_copiedNodes.size() == 0);
		if (ImGui::MenuItem("Paste"))
		{
			for (const auto& node : _copiedNodes)
			{
				auto offset = node->GetPosition() - _copiedNode->GetPosition();
				node->SetPosition(canvasPos + offset);
				node->SetRecordedPosition(node->GetPosition());
			}

			Execute(_copyNodesCommand);
			_copiedNodes.clear();
		}
		ImGui::EndDisabled();
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopup(CONNECTION_CONTEXT_MENU))
	{
		ImGui::SeparatorText("Connection");

		auto typeValue = _focusedConnection->GetType();
		for (int i = 0; i < 5; i++) {
			if (i > 0) ImGui::SameLine();

			auto rgb = _focusedConnection->GetColor(i);
			float h, s, v;
			ImGui::ColorConvertRGBtoHSV(rgb.Value.x, rgb.Value.y, rgb.Value.z, h, s, v);

			ImGui::PushID(i);
			ImGui::PushStyleColor(ImGuiCol_CheckMark, IM_COL32(24, 24, 24, 255));
			ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor::HSV(h, s, v));
			ImGui::PushStyleColor(ImGuiCol_FrameBgActive, (ImVec4)ImColor::HSV(h, s - .1, v - .1));
			ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, (ImVec4)ImColor::HSV(h, s - .2, v - .2));
			ImGui::RadioButton("##", _focusedConnection->GetTypePtr(), i);
			ImGui::PopStyleColor(4);
			ImGui::PopID();
		}

		if (typeValue != _focusedConnection->GetType())
			Execute(new EditValueCommand<int>(_focusedConnection->GetTypePtr(), typeValue));

		ImGui::Spacing();
		ImGui::Separator();

		auto avail = ImGui::GetContentRegionAvail();
		ImGui::SetNextItemWidth(avail.x);
		NodesGraph::Input::Float("##V", _focusedConnection->GetValuePtr(), .1f);
		if (ImGui::MenuItem("Delete"))
			_commands.Execute(new DeleteConnectionCommand(_focusedConnection, &_connections));

		ImGui::EndPopup();
	}

	if (ImGui::BeginPopup(NODE_CONTEXT_MENU))
	{
		auto deleteNode = [this](Node* node, CommandCluster* command) {
			command->Add(new DeleteNodeCommand(node, &_nodes));

			auto groupNode = dynamic_cast<_GroupNode*>(node);
			for (const auto& [_, connection] : _connections)
			{
				for (const auto& slot : node->GetSlots())
				{
					if (connection->GetFrom() == slot || connection->GetTo() == slot)
						command->Add(new DeleteConnectionCommand(connection, &_connections));
				}

				if (groupNode)
				{
					for (const auto& childNode : groupNode->GetNodes())
					{
						for (const auto& slot : childNode->GetSlots())
						{
							if (connection->GetFrom() == slot || connection->GetTo() == slot)
								command->Add(new DeleteConnectionCommand(connection, &_connections));
						}
					}
				}
			}
			};

		auto isSelected = _selectedNodes.size() > 1 && _selectedNodes.find(_focusedNode) != _selectedNodes.end();
		auto text = isSelected ? "Nodes" : "Node";
		ImGui::SeparatorText(text);
		if (ImGui::MenuItem("Delete"))
		{
			if (!isSelected) {
				auto command = new CommandCluster("Delete Node");
				deleteNode(_focusedNode, command);
				_commands.Execute(command);
			}
			else {
				auto command = new CommandCluster("Delete Nodes");
				for (const auto& node : _selectedNodes)
					deleteNode(node, command);
				_commands.Execute(command);
			}
		}
		if (ImGui::MenuItem("Copy"))
		{
			if (_copyNodesCommand != nullptr && _copyNodesCommand->GetState() == Created)
				delete _copyNodesCommand;

			_copiedNodes.clear();

			_copyNodesCommand = new CommandCluster("Paste Nodes");

			std::map<NodeConnection*, std::tuple<Node*, int>> connectionsFrom;
			std::map<NodeConnection*, std::tuple<Node*, int>> connectionsTo;
			std::map<Node*, Node*> nodesCreated;

			auto copyNodeConnections = [this, &connectionsFrom, &connectionsTo, &nodesCreated](Node* node) {
				for (const auto& [key, connection] : _connections)
				{
					int slotIndex = 0;
					for (const auto& slot : node->GetSlots())
					{
						if (connection->GetFrom() == slot)
						{
							auto iter = connectionsTo.find(connection);
							if (iter != connectionsTo.end())
							{
								auto slotFrom = nodesCreated.at(node)->GetSlots()[slotIndex];
								auto slotTo = nodesCreated.at(std::get<0>(iter->second))->GetSlots()[std::get<1>(iter->second)];

								auto newConnection = new NodeConnection(slotFrom, slotTo);
								_copyNodesCommand->Add(new CreateConnectionCommand(newConnection, &_connections));
								connectionsTo.erase(connection);
							}
							else
							{
								connectionsFrom.emplace(connection, std::make_tuple(node, slotIndex));
							}
						}
						else if (connection->GetTo() == slot)
						{
							auto iter = connectionsFrom.find(connection);
							if (iter != connectionsFrom.end())
							{
								auto slotTo = nodesCreated.at(node)->GetSlots()[slotIndex];
								auto slotFrom = nodesCreated.at(std::get<0>(iter->second))->GetSlots()[std::get<1>(iter->second)];

								auto newConnection = new NodeConnection(slotFrom, slotTo);
								_copyNodesCommand->Add(new CreateConnectionCommand(newConnection, &_connections));
								connectionsFrom.erase(connection);
							}
							else
							{
								connectionsTo.emplace(connection, std::make_tuple(node, slotIndex));
							}
						}

						slotIndex++;
					}
				}
				};

			if (_selectedNodes.size() > 0)
			{
				for (const auto& node : _selectedNodes)
				{
					auto copy = node->Clone();
					_copyNodesCommand->Add(new CreateNodeCommand(copy, &_nodes));

					nodesCreated.emplace(node, copy);
					_copiedNodes.emplace(copy);
					copyNodeConnections(node);

					auto groupNode = dynamic_cast<_GroupNode*>(node);
					auto groupNodeCopy = dynamic_cast<_GroupNode*>(copy);
					if (groupNode)
					{
						auto& childNodes = groupNode->GetNodes();
						auto& childNodesCopies = groupNodeCopy->GetNodes();
						for (int i = 0; i < childNodes.size(); i++)
						{
							auto childNode = childNodes[i];
							auto childNodeCopy = childNodesCopies[i];
							nodesCreated.emplace(childNode, childNodeCopy);
							copyNodeConnections(childNode);
						}
					}
				}
			}
			else {
				auto copy = _focusedNode->Clone();
				_copyNodesCommand->Add(new CreateNodeCommand(copy, &_nodes));

				nodesCreated.emplace(_focusedNode, copy);
				_copiedNodes.emplace(copy);
				copyNodeConnections(_focusedNode);

				auto groupNode = dynamic_cast<_GroupNode*>(_focusedNode);
				auto groupNodeCopy = dynamic_cast<_GroupNode*>(copy);
				if (groupNode)
				{
					auto& childNodes = groupNode->GetNodes();
					auto& childNodesCopies = groupNodeCopy->GetNodes();
					for (int i = 0; i < childNodes.size(); i++)
					{
						auto childNode = childNodes[i];
						auto childNodeCopy = childNodesCopies[i];
						nodesCreated.emplace(childNode, childNodeCopy);
						copyNodeConnections(childNode);
					}
				}
			}

			_copiedNode = _focusedNode;
		}

		auto contextMenu = GetNodeContextMenu(_focusedNode);
		if (contextMenu) {
			ImGui::Spacing();
			ImGui::Separator();
			contextMenu(_focusedNode);
		}

		ImGui::EndPopup();
	}

	if (ImGui::BeginPopup(CHILD_NODE_CONTEXT_MENU))
	{
		ImGui::SeparatorText("Child Node");
		if (ImGui::MenuItem("Delete"))
		{
			auto command = new CommandCluster("Delete Child Node");
			command->Add(new DeleteChildNodeCommand(_focusedChildNode, &_focusedChildNodeParent->GetNodes()));

			for (const auto& [_, connection] : _connections)
			{
				for (const auto& slot : _focusedChildNode->GetSlots())
				{
					if (connection->GetFrom() == slot || connection->GetTo() == slot)
						command->Add(new DeleteConnectionCommand(connection, &_connections));
				}
			}

			_commands.Execute(command);
		}

		auto contextMenu = GetNodeContextMenu(_focusedChildNode);
		if (contextMenu) {
			ImGui::Spacing();
			ImGui::Separator();
			contextMenu(_focusedChildNode);
		}

		ImGui::EndPopup();
	}

	ImGui::PopStyleVar();
}

void NodesGraph::DrawSelection()
{
	if (!_isDrawingSelection &&
		ImGui::IsWindowHovered() &&
		!ImGui::IsAnyItemHovered() && !_hoveredConnection &&
		ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		_isDrawingSelection = true;
		_drawingSelectionFrom = ImGui::GetMousePos();
	}

	if (_isDrawingSelection)
	{
		_drawList->AddRectFilled(_drawingSelectionFrom, ImGui::GetMousePos(), _selectionFillColor);
		_drawList->AddRect(_drawingSelectionFrom, ImGui::GetMousePos(), _selectionOutlineColor);

		if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
			_isDrawingSelection = false;
	}
}

void NodesGraph::DrawConnectionAttempt()
{
	if (_isDrawingConnection)
	{
		_drawList->AddCircleFilled(_drawingConnectionFrom->GetPosition(), 3.0_dpi, IM_COL32(164, 164, 164, 255));
		_drawList->AddLine(_drawingConnectionFrom->GetPosition(), ImGui::GetMousePos(), IM_COL32(164, 164, 164, 255), 2.0_dpi);
		_drawList->AddCircleFilled(ImGui::GetMousePos(), 3.0_dpi, IM_COL32(164, 164, 164, 255));

		if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			_isDrawingConnection = false;

			if (_hoveredSlot != NULL && _hoveredSlot != _drawingConnectionFrom)
			{
				auto connection = new NodeConnection(_drawingConnectionFrom, _hoveredSlot);
				_commands.Execute(new CreateConnectionCommand(connection, &_connections));
			}
		}
	}
}

void NodesGraph::DrawOverlay()
{
	const float arrowsPadding = 16_dpi;
	const float arrowSize = 12_dpi;

	if (_hasOffscreenNodesLeft)
	{
		ImVec2 pl1 = ImVec2(_windowPos.x + arrowsPadding, _windowPos.y + _windowSize.y / 2);
		ImVec2 pl2 = ImVec2(pl1.x + arrowSize / 2, pl1.y - arrowSize / 2);
		ImVec2 pl3 = ImVec2(pl1.x + arrowSize / 2, pl1.y + arrowSize / 2);
		_drawList->AddTriangleFilled(pl1, pl2, pl3, IM_COL32_WHITE);
	}

	if (_hasOffscreenNodesTop)
	{
		ImVec2 pt1 = ImVec2(_windowPos.x + _windowSize.x / 2, _windowPos.y + arrowsPadding);
		ImVec2 pt2 = ImVec2(pt1.x + arrowSize / 2, pt1.y + arrowSize / 2);
		ImVec2 pt3 = ImVec2(pt1.x - arrowSize / 2, pt1.y + arrowSize / 2);
		_drawList->AddTriangleFilled(pt1, pt2, pt3, IM_COL32_WHITE);
	}

	if (_hasOffscreenNodesRight)
	{
		ImVec2 pr1 = ImVec2(_windowPos.x + _windowSize.x - arrowsPadding, _windowPos.y + _windowSize.y / 2);
		ImVec2 pr2 = ImVec2(pr1.x - arrowSize / 2, pr1.y + arrowSize / 2);
		ImVec2 pr3 = ImVec2(pr1.x - arrowSize / 2, pr1.y - arrowSize / 2);
		_drawList->AddTriangleFilled(pr1, pr2, pr3, IM_COL32_WHITE);
	}

	if (_hasOffscreenNodesBottom)
	{
		ImVec2 pb1 = ImVec2(_windowPos.x + _windowSize.x / 2, _windowPos.y + _windowSize.y - arrowsPadding);
		ImVec2 pb2 = ImVec2(pb1.x - arrowSize / 2, pb1.y - arrowSize / 2);
		ImVec2 pb3 = ImVec2(pb1.x + arrowSize / 2, pb1.y - arrowSize / 2);
		_drawList->AddTriangleFilled(pb1, pb2, pb3, IM_COL32_WHITE);
	}

	if (!_validationMessage.empty()) {
		ImGui::BeginTooltip();
		ImGui::TextUnformatted(_validationMessage.c_str());
		ImGui::EndTooltip();
	}
}

void NodesGraph::HandleInput()
{
	if (!_isScrollingCanvas && ImGui::IsWindowHovered() && (ImGui::IsMouseClicked(ImGuiMouseButton_Middle) || ImGui::IsKeyPressed(ImGuiKey_Space))) {
		_isScrollingCanvas = true;
	}

	if (_isScrollingCanvas) {
		_offset += _io.MouseDelta;
		if (ImGui::IsMouseReleased(ImGuiMouseButton_Middle) || ImGui::IsKeyReleased(ImGuiKey_Space))
			_isScrollingCanvas = false;
	}
}

void NodesGraph::Input::Float(const char* label, float* v, float step, float step_fast, const char* format, ImGuiInputTextFlags flags)
{
	auto previousValue = *v;
	ImGui::SetNextItemWidth(192_dpi);
	ImGui::InputFloat(label, v, step, step_fast, format, flags);

	if (ImGui::IsItemActivated()) {
		_previousFloatValue = *v;
	}

	if (ImGui::IsItemDeactivatedAfterEdit()) {
		if (_previousFloatValue != *v)
			_current->Execute(new EditValueCommand<float>(v, _previousFloatValue));
	}
	else if (ImGui::IsItemEdited() && ImGui::IsItemActivated() && previousValue != *v)
	{
		if (previousValue != *v)
			_current->Execute(new EditValueCommand<float>(v, previousValue));
	}
}