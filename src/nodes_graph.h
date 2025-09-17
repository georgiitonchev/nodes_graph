#pragma once

// external
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include "imgui_stdlib.h"

// std
#include <map>
#include <string>
#include <unordered_set>
#include <typeindex>

// local
#include "node.h"
#include "node_connection.h"
#include "node_slot.h"
#include "commands.h"
#include "literals.h"

class NodesGraph {
public:
	~NodesGraph();

	void Draw();

	void Deserialize(std::string data);
	std::string Serialize();

	void Execute(_Command* command);

	void Undo();
	std::vector<_Command*>& GetUndoStack();

	void Redo();
	std::vector<_Command*>& GetRedoStack();

	bool HasUnsavedChanges() const;
	inline float GetScale() const { return _scale; };
	inline ImVec2 GetOffset() const { return _offset; }
	inline ImVec2 GetWindowPos() const { return _windowPos; }
	inline ImVec2 GetWindowSize() const { return _windowSize; }
	inline std::map<std::string, Node*>& GetNodes() { return _nodes; }
	inline std::map<std::string, NodeConnection*>& GetConnections() { return _connections; }

	void FocusPosition(const ImVec2& position);
	void FocusOnNode(Node* node);

	template<DerivedFromNode T>
	inline static void RegisterNode(std::string label) {
		_nodesRegistry[label] = [label](ImVec2 pos) -> Node* {
			auto node = new T();
			node->Init();
			node->SetType(label);
			node->SetLabel(label);
			node->SetPosition(pos);
			node->SetRecordedPosition(pos);
			return node;
			};
	}

	template<DerivedFromNode T>
	inline static void RegisterNodeContextMenu(std::function<void(T*)> callback) {
		_nodeContextMenus[std::type_index(typeid(T))] =
			[cb = std::move(callback)](Node* n) {
			cb(static_cast<T*>(n));
			};
	}

	static NodesGraph* GetCurrent() {
		return _current;
	}

private:
	inline static NodesGraph* _current;

	Commands _commands;
	int _savedCommandIndex = 0;

	ImGuiIO& _io = ImGui::GetIO();
	ImDrawList* _drawList;

	inline static const float _zoomLevels[] =
	{
		 0.1f, 0.15f, 0.20f, 0.25f, 0.33f, 0.5f, 0.75f, 1.0f, 1.25f, 1.50f, 2.0f, 2.5f, 3.0f
	};

	int _scaleIndexClipDetails = 3;
	int _scaleIndex = 7;
	float _scale = 1;
	ImVec2 _scalePosition;
	float _targetScale = 1;

	ImVec2 _windowPos;
	ImVec2 _windowSize;
	ImVec2 _offset;

	ImColor _colorBackground = IM_COL32(200, 200, 200, 40);
	float _backgroundGridSize = 48.0_dpi;
	float _backgroundDotSize = 2_dpi;
	void DrawBackground() const;

	std::map<std::string, Node*> _nodes;
	std::map<std::string, NodeConnection*> _connections;

	std::unordered_set<Node*> _selectedNodes;
	std::unordered_set<Node*> _copiedNodes;
	CommandCluster* _copyNodesCommand;

	ImVec2 _draggedNodePos;
	Node* _draggedNode;
	Node* _hoveredNode;
	Node* _focusedNode;
	Node* _copiedNode;
	NodeSlot* _hoveredSlot;

	std::string _validationMessage;

	Node* _hoveredChildNode;
	_GroupNode* _hoveredChildNodeParent;

	Node* _focusedChildNode;
	_GroupNode* _focusedChildNodeParent;

	Node* _draggedChildNode;
	_GroupNode* _draggedChildNodeParent;
	int _draggedChildNodeIndex;

	NodeConnection* _hoveredConnection;
	NodeConnection* _focusedConnection;
	NodeConnection* _clickedConnection;

	bool _isEditingConnection = false;
	bool _isEditingConnectionFrom = false;

	bool _hasOffscreenNodesLeft = false;
	bool _hasOffscreenNodesRight = false;
	bool _hasOffscreenNodesTop = false;
	bool _hasOffscreenNodesBottom = false;

	bool _isDrawingSelection = false;
	bool _isDraggingNodes = false;

	bool _isScrollingCanvas = false;

	ImColor _selectionFillColor = IM_COL32(60, 60, 60, 60);
	ImColor _selectionOutlineColor = IM_COL32(120, 120, 120, 60);
	ImVec2 _drawingSelectionFrom;

	bool _isDrawingConnection = false;
	NodeSlot* _drawingConnectionFrom = nullptr;

	ImVec2 _mousePosBackup;
	ImVec2 _mousePosPrevBackup;
	ImVec2 _mouseDeltaBackup;
	ImVec2 _mouseClickedPosBackup[IM_ARRAYSIZE(ImGuiIO::MouseClickedPos)];
	ImVec2 _windowCursorMaxBackup;

	int _drawListFirstCommandIndex;
	int _drawListCommadBufferSize;
	int _drawListStartVertexIndex;

	float _fringeScaleBackup;

	void BeginCanvas();
	void EndCanvas();

	inline ImVec2 ScreenToCanvas(const ImVec2& screenPos) const { return  (screenPos - _offset - _windowPos) / _scale; }
	inline ImVec2 CanvasToScreen(const ImVec2& canvasPos) const { return canvasPos * _scale + _offset + _windowPos; }

	void DrawNodes();

	void DrawConnections();
	void DrawContextMenus();
	void DrawSelection();
	void DrawConnectionAttempt();
	void DrawOverlay();

	void HandleNodesDragging();
	void HandleCanvasZooming();

	void HandleInput();

	inline static Node* CreateNode(const std::string& type) {
		auto it = _nodesRegistry.find(type);
		if (it != _nodesRegistry.end()) {
			return it->second(ImVec2(0, 0));
		}

		return nullptr;
	}

	inline static std::function<void(Node*)> GetNodeContextMenu(Node* n) {
		auto it = _nodeContextMenus.find(std::type_index(typeid(*n)));
		if (it != _nodeContextMenus.end())
			return it->second;

		return nullptr;
	}

	inline static std::unordered_map<std::type_index, std::function<void(Node*)>> _nodeContextMenus;
	inline static std::unordered_map<std::string, std::function<Node* (ImVec2 pos)>> _nodesRegistry;

	struct Input {
	private:
		inline static float _previousFloatValue;
	public:
		static void Float(const char* label, float* v, float step = 0.0f, float step_fast = 0.0f, const char* format = "%.3f", ImGuiInputTextFlags flags = 0);
	};
};