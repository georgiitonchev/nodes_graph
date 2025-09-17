#include "node_connection.h"

#include "guid.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

void NodeConnection::Draw(ImDrawList* drawList, bool clipDetails)
{
	_isHovered = false;

	auto getControlPoint = [](const ImVec2& relativePosition)
		{
			auto controlPoint = ImVec2(0, 0);
			if (relativePosition.x > 0.0f && relativePosition.x < 1)
				controlPoint.y = relativePosition.y > 0.5f ? 1 : -1;
			if (relativePosition.y > 0.0f && relativePosition.y < 1)
				controlPoint.x = relativePosition.x > 0.5f ? 1 : -1;

			return controlPoint;
		};

	auto distance = [](const ImVec2& p1, const ImVec2& p2) {
		return std::sqrt((p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y));
		};

	auto p1 = _from->GetPosition();
	auto p4 = _to->GetPosition();

	auto controlPointFactor = (distance(p1, p4) / 4 * NodesGraphSettings::GetDpiScale());

	auto p2 = p1 + getControlPoint(_from->GetRelativePosition()) * controlPointFactor;
	auto p3 = p4 + getControlPoint(_to->GetRelativePosition()) * controlPointFactor;
	auto mousePos = ImGui::GetMousePos();

	auto getBezierPoint = [p1, p2, p3, p4](float t)
		{
			auto u = 1.0 - t;
			auto tt = t * t;
			auto uu = u * u;
			auto ttt = tt * t;
			auto uuu = uu * u;

			ImVec2 B;
			B.x = uuu * p1.x + 3 * uu * t * p2.x + 3 * u * tt * p3.x + ttt * p4.x;
			B.y = uuu * p1.y + 3 * uu * t * p2.y + 3 * u * tt * p3.y + ttt * p4.y;

			return B;
		};

	auto isPointOnBezierCurve = [distance, getBezierPoint](const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, const ImVec2& point, float thickness, int samples = 100)
		{
			for (int i = 0; i <= samples; ++i)
			{
				auto t = static_cast<float>(i) / static_cast<float>(samples);
				auto bezierPoint = getBezierPoint(t);
				auto dist = distance(bezierPoint, point);
				if (dist <= thickness)
					return true;
			}

			return false;
		};

	if (ImGui::IsWindowHovered() && isPointOnBezierCurve(p1, p2, p3, p4, mousePos, _thicknessDefault * 2))
		_isHovered = true;

	auto thickness = _isHovered ? _thicknessHovered : _thicknessDefault;
	auto color = _colors[_type];
	drawList->AddBezierCubic(p1, p2, p3, p4, color, thickness);

	ImVec2 pt1 = p4;
	ImVec2 pt2, pt3;

	auto xRelativeToNode = (_to->GetRelativePosition().x * 2.0f) - 1.0f;
	auto yRelativeToNode = (_to->GetRelativePosition().y * 2.0f) - 1.0f;

	auto triangleSize = _isHovered ? _triangleSizeHovered : _triangleSizeDefault;

	if (xRelativeToNode != 0) {
		pt2 = p4 + ImVec2(0, triangleSize) + ImVec2(triangleSize, 0) * xRelativeToNode;
		pt3 = p4 + ImVec2(0, -triangleSize) + ImVec2(triangleSize, 0) * xRelativeToNode;
	}
	else if (yRelativeToNode != 0) {
		pt2 = p4 + ImVec2(0, triangleSize) * yRelativeToNode + ImVec2(triangleSize, 0);
		pt3 = p4 + ImVec2(0, triangleSize) * yRelativeToNode + ImVec2(-triangleSize, 0);
	}

	if (!clipDetails)
		drawList->AddTriangleFilled(pt1, pt2, pt3, color);

	if (!clipDetails && _value != 0.0f) {
		char label[32];
		snprintf(label, 32, "%.1f", _value);
		auto center = getBezierPoint(.5);
		auto labelSize = ImGui::CalcTextSize(label);
		auto labelPos = ImVec2((center.x) - labelSize.x / 2, center.y - labelSize.y / 2);
		auto labelPadding = ImVec2(4_dpi, 4_dpi);

		drawList->AddRectFilled(labelPos - labelPadding, labelPos + labelSize + labelPadding, IM_COL32(32, 32, 32, 128), 1);
		drawList->AddText(labelPos, IM_COL32(255, 255, 255, 255), label);
	}
}

void NodeConnection::ToJson(nlohmann::json& j)
{
	j["id"] = _id;
	j["type"] = _type;
	j["value"] = _value;
	j["from"] = _from->GetId();
	j["to"] = _to->GetId();
}

void NodeConnection::FromJson(const nlohmann::json& j)
{
	j.at("id").get_to(_id);
	j.at("type").get_to(_type);
	j.at("value").get_to(_value);
}

NodeConnection::NodeConnection(NodeSlot* from, NodeSlot* to) :
	_from(from),
	_to(to),
	_isHovered(false)
{
	_id = Guid::CreateGuid();
}
