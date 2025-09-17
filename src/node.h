#pragma once

// external
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include <json.h>

// std
#include <string>
#include <vector>

// local
#include "node_slot.h"
#include "literals.h"

class Node {
public:
	Node();
	void Init();

	virtual ~Node();

	virtual void Draw(ImDrawList* drawList, bool clipDetails);
	virtual void PreDraw(ImDrawList* drawList);
	virtual Node* Clone();

	inline std::string GetId() const { return _id; };
	inline ImVec2 GetPosition() const { return _position; };
	inline ImVec2 GetRecordedPosition() const { return _recordedPosition; };
	inline ImVec2 GetSize() const { return _size; };
	inline std::vector<NodeSlot*>& GetSlots() { return _slots; };

	inline void SetType(std::string type) { _type = type; };
	inline void SetLabel(std::string label) { _label = label; };
	inline void SetPosition(const ImVec2& position) { _position = position; };
	inline void SetRecordedPosition(const ImVec2& position) { _recordedPosition = position; };
	inline std::string GetValidationMessage() const { return _validationMessage; };

	inline bool IsValid() const { return _isValid; };
	inline bool IsValidationCircleHovered() const { return _isErrorCircleHovered; };

	inline bool IsHovered() const { return _isHovered; };
	inline bool IsPressed() const { return _isPressed; };
	inline bool IsSelected() const { return _isSelected; };

	inline void SetIsSelected(bool value) { _isSelected = value; };

	virtual void ToJson(nlohmann::json& j);
	virtual void FromJson(const nlohmann::json& j);

private:
	ImVec2 _recordedPosition;
	ImVec2 _position;
	ImVec2 _size;

	std::string _type;
	std::string _id;

	std::string _validationMessage;
	bool _isValid = true;
	bool _isErrorCircleHovered = false;

	bool _isPressed;
	bool _isHovered;
	bool _isSelected;

	std::vector<NodeSlot*> _slots;

	bool Validate();

protected:
	ImColor _colorSelected = IM_COL32(48, 48, 48, 255);
	ImColor _colorHovered = IM_COL32(32, 32, 32, 255);
	ImColor _colorDefault = IM_COL32(24, 24, 24, 255);
	ImColor _colorOutline = IM_COL32(128, 128, 128, 255);
	ImVec2 _padding = ImVec2(8_dpi, 8_dpi);

	bool _outputRequired = true;
	bool _inputRequired = true;
	std::string _label;

	virtual void _Init() = 0;
	inline virtual void _DrawBefore(ImDrawList* drawList) {};
	inline virtual void _DrawAfter(ImDrawList* drawList) {};
	virtual void _Draw(ImDrawList* drawList) = 0;
	virtual void _ToJson(nlohmann::json& j) = 0;
	virtual void _FromJson(const nlohmann::json& j) = 0;
	virtual bool _Validate() { return true; };
	virtual Node* _Clone() = 0;

	inline void SetValidationMessage(const std::string message) { _validationMessage = message; }
	void AddSlot(ImVec2 relativePosition, bool isInput = true, bool isOutput = true);

	enum SlotPosition {
		Left,
		Right,
		Top,
		Bottom
	};

	void AddSlot(SlotPosition position, bool isInput = true, bool isOutput = true);
};

template <typename T>
concept DerivedFromNode = std::is_base_of<Node, T>::value && !std::is_same<Node, T>::value;

class _GroupNode : public Node {
protected:
	std::vector<Node*> _nodes;
	ImVec2 _dummySize;

	Node* _createdNode;

	virtual void _Init() override = 0;
	virtual void _Draw(ImDrawList* drawList) override = 0;
	virtual void _ToJson(nlohmann::json& j) override = 0;
	virtual void _FromJson(const nlohmann::json& j) override = 0;
	virtual Node* _Clone() override = 0;

public:
	_GroupNode() : _createdNode(nullptr) {}

	inline Node* GetCreatedNode() { return _createdNode; };
	inline std::vector<Node*>& GetNodes() { return _nodes; };
	inline ImVec2 GetDummySize() const { return _dummySize; }

	virtual Node* Clone() override;

	virtual ~_GroupNode() override {
		for (auto node : _nodes) {
			delete node;
		}
	}
};

template<DerivedFromNode T>
class GroupNode : public _GroupNode {
protected:
	virtual void _Draw(ImDrawList* drawList) override = 0;
	virtual void _DrawAfter(ImDrawList* drawList) override {
		_dummySize = ImVec2(0, 0);

		for (auto& child : _nodes) {
			if (_dummySize.x < child->GetSize().x)
				_dummySize.x = child->GetSize().x;

			_dummySize.y += child->GetSize().y + 6_dpi;
		}

		ImGui::Dummy(_dummySize);

		_createdNode = nullptr;

		auto buttonSize = ImGui::GetFrameHeight();
		if (ImGui::Button("+", ImVec2(buttonSize, buttonSize))) {
			_createdNode = new T();
			_createdNode->Init();
		}
	}

public:
	virtual void ToJson(nlohmann::json& j) override {
		Node::ToJson(j);
		nlohmann::json jArrayNodes = nlohmann::json::array();

		for (const auto node : _nodes) {
			nlohmann::json jObjectNode;
			node->ToJson(jObjectNode);

			jArrayNodes.push_back(jObjectNode);
		}

		j["nodes"] = jArrayNodes;
		j["dummy_size.x"] = _dummySize.x;
		j["dummy_size.y"] = _dummySize.y;
	}

	virtual void FromJson(const nlohmann::json& j) override {
		Node::FromJson(j);
		if (j.contains("nodes")) {
			nlohmann::json jArrayNodes = j["nodes"];
			for (const auto& jObjectNode : jArrayNodes) {
				auto node = new T();
				node->Init();
				node->FromJson(jObjectNode);

				_nodes.push_back(node);
			}
		}

		j.at("dummy_size.x").get_to(_dummySize.x);
		j.at("dummy_size.y").get_to(_dummySize.y);
	}
};