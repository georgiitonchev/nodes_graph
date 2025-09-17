#pragma once
#include <json.h>

#include "node_slot.h"
#include "literals.h"

class NodeConnection {
private:
	std::string _id;

	NodeSlot* _from;
	NodeSlot* _to;

	int _type = 0;
	float _value = 0;

	bool _isHovered;

	float _triangleSizeDefault = 6.0_dpi;
	float _triangleSizeHovered = 8.0_dpi;

	float _thicknessDefault = 2.0_dpi;
	float _thicknessHovered = 4.0_dpi;

	ImColor _color = IM_COL32(155, 155, 155, 255);

	inline static constexpr ImColor _colors[5] = {
		ImColor(155, 155, 155, 255),
		ImColor(228, 54, 54, 255),
		ImColor(255, 215, 0, 255),
		ImColor(155, 215, 0, 255),
		ImColor(77, 168, 218, 255),
	};

public:
	NodeConnection(NodeSlot* from, NodeSlot* to);

	inline std::string GetId() const { return _id; };
	inline NodeSlot* GetFrom() const { return _from; };
	inline void SetFrom(NodeSlot* slot) { _from = slot; };
	inline NodeSlot* GetTo() const { return _to; };
	inline void SetTo(NodeSlot* slot) { _to = slot; };
	inline bool IsHovered() const { return _isHovered; };

	inline ImColor GetColor(int index) { return _colors[index]; };

	inline int GetType() const { return _type; };
	inline int* GetTypePtr() { return &_type; };
	inline int GetValue() const { return _value; };
	inline float* GetValuePtr() { return &_value; };

	void Draw(ImDrawList* drawList, bool clipDetails);

	void ToJson(nlohmann::json& j);
	void FromJson(const nlohmann::json& j);
};