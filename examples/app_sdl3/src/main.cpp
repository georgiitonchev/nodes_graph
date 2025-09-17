#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_internal.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"

#include <SDL3/SDL.h>

// std
#include <string>
#include <vector>
#include <fstream>
#include <map>
#include <cstdio>
#include <cstdlib>

// graph
#include "nodes_graph.h"
#include "nodes_graph_settings.h"

// nodes
#include "nodes/entry_node.h"
#include "nodes/exit_node.h"
#include "nodes/speech_node.h"
#include "nodes/responses_node.h"
#include "nodes/action_node.h"
#include "nodes/connector_in_node.h"
#include "nodes/connector_out_node.h"

const char* SAVE_POPUP = "Save?";
const char* FILE_CONTEXT_MENU = "FILE_CONTEXT_MENU";

// fields
static SDL_Window* _window = nullptr;

static bool _windowHasBeenResized = true;
static int _windowWidth = 1280;
static int _windowHeight = 720;

static int _renamingFile = -1;
static bool _creatingNewFile = false;
static bool _newFileInputFocused = false;
static std::string _newFileName;

static bool _showStatsWindow = false;
static bool _showHistoryWindow = false;

static std::string _directory;
static bool _filesLoaded = false;

static std::vector<std::string> _files;
static std::string _currentFile; // focused graph
static std::string _hoveredFile; // hovered in files
static std::string _focusedFile; // right clicked
static std::string _renamedFile;

static std::map<std::string, NodesGraph*> _openedGraphs;
static NodesGraph* _focusedGraph = nullptr;
static std::string _closingGraph;

static bool _showSavePopup = false;

static bool _done = false;
static bool _closing = false;

static void* UserData_ReadOpen(ImGuiContext* ctx, ImGuiSettingsHandler* handler, const char* name)
{
	return (void*)1;
}

static void UserData_ReadLine(ImGuiContext* ctx, ImGuiSettingsHandler* handler, void* entry, const char* line)
{
	char value[201];
	int valueInt;

	if (sscanf(line, "Path=%200s", value) == 1)
		_directory = std::string(value);
	else if (sscanf(line, "DebugWindow=%10s", value) == 1)
		_showStatsWindow = (strcmp(value, "true") == 0);
	else if (sscanf(line, "HistoryWindow=%10s", value) == 1)
		_showHistoryWindow = (strcmp(value, "true") == 0);
	else if (sscanf(line, "ValidateNodes=%10s", value) == 1)
		NodesGraphSettings::ValidateNodesRef() = (strcmp(value, "true") == 0);
	else if (sscanf(line, "EnableSnapping=%10s", value) == 1)
		NodesGraphSettings::NodeSnappingEnabledRef() = (strcmp(value, "true") == 0);
	else if (sscanf(line, "NodeSnapping=%d", &valueInt) == 1)
		NodesGraphSettings::NodeSnappingValueRef() = valueInt;
}

static void UserData_WriteAll(ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buffer)
{
	buffer->appendf("[%s][%s]\n", "UserData", "Settings");
	buffer->appendf("Path=%s\n", _directory.c_str());
	buffer->appendf("DebugWindow=%s\n", _showStatsWindow ? "true" : "false");
	buffer->appendf("HistoryWindow=%s\n", _showHistoryWindow ? "true" : "false");
	buffer->appendf("ValidateNodes=%s\n", NodesGraphSettings::ValidateNodes() ? "true" : "false");
	buffer->appendf("EnableSnapping=%s\n", NodesGraphSettings::NodeSnappingEnabled() ? "true" : "false");
	buffer->appendf("NodeSnapping=%d\n", NodesGraphSettings::NodeSnappingValue());
}

static SDL_EnumerationResult EnumerateDirectoryCallback(void* userdata, const char* dirname, const char* fname)
{
	auto fullFileName = std::string(fname);
	auto pos = fullFileName.find(".sgraph");
	if (pos != std::string::npos) {
		auto fileName = fullFileName.substr(0, pos);
		_files.emplace_back(fileName);
	}

	return SDL_ENUM_CONTINUE;
}

static void LoadFilesAtDirectory()
{
	_files.clear();
	SDL_EnumerateDirectory(_directory.c_str(), EnumerateDirectoryCallback, nullptr);
	_filesLoaded = true;
}

static bool SDLEventWatch(void* userData, SDL_Event* event)
{
	if (event->type == SDL_EVENT_WINDOW_RESIZED) {
		SDL_GetWindowSize(_window, &_windowWidth, &_windowHeight);
		_windowHasBeenResized = true;
	}

	return true;
}

static void CloseGraph(std::string filename)
{
	auto graph = _openedGraphs.at(filename);
	_openedGraphs.erase(filename);

	if (_focusedGraph == graph) {
		if (_openedGraphs.size() > 0) {
			_focusedGraph = _openedGraphs.begin()->second;
			_currentFile = _openedGraphs.begin()->first;
		}
		else {
			_focusedGraph = nullptr;
			_currentFile.clear();
		}
	}

	delete graph;
}

static void OpenGraph(std::string graphName)
{
	char filename[100];
	SDL_snprintf(filename, 100, "%s/%s.%s", _directory.c_str(), graphName.c_str(), "sgraph");

	_currentFile = graphName;

	if (!_openedGraphs.contains(graphName)) {
		auto graph = new NodesGraph();
		_openedGraphs.emplace(graphName, graph);

		auto stream = SDL_IOFromFile(filename, "r");
		auto dataSize = SDL_GetIOSize(stream);
		auto fileData = (char*)malloc(dataSize + 1);
		fileData[dataSize] = '\0';
		SDL_ReadIO(stream, fileData, dataSize);

		graph->Deserialize(fileData);

		free(fileData);
		SDL_CloseIO(stream);
	}

	_focusedGraph = _openedGraphs[graphName];
}

static void DrawMenuBar()
{
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::BeginMenu("New", !_directory.empty()))
			{
				if (ImGui::MenuItem("Graph"))
				{
					_creatingNewFile = true;
					_newFileInputFocused = false;
				}
				ImGui::EndMenu();
			}
			if (_directory.empty())
				ImGui::SetItemTooltip("No folder selected.");

			ImGui::Separator();
			if (ImGui::MenuItem("Save", "Ctrl+S", nullptr, _focusedGraph && _focusedGraph->HasUnsavedChanges()))
			{
				// TODO: Fix all of these
				auto data = _focusedGraph->Serialize();

				char filename[100];
				SDL_snprintf(filename, 100, "%s/%s.%s", _directory.c_str(), _currentFile.c_str(), "sgraph");

				auto stream = SDL_IOFromFile(filename, "w");
				SDL_WriteIO(stream, data.c_str(), data.size());
				SDL_CloseIO(stream);
			}

			/*if (ImGui::MenuItem("Save All", "Ctrl+Shift+S", nullptr, false)) {

			}*/

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit"))
		{
			if (ImGui::MenuItem("Undo", "Ctrl+Z", nullptr, _focusedGraph && !_focusedGraph->GetUndoStack().empty()))
				_focusedGraph->Undo();
			if (ImGui::MenuItem("Redo", "Ctrl+Y", nullptr, _focusedGraph && !_focusedGraph->GetRedoStack().empty()))
				_focusedGraph->Redo();
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
			ImGui::MenuItem("History", "", &_showHistoryWindow);
			ImGui::MenuItem("Stats", "", &_showStatsWindow);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Settings"))
		{
			if (ImGui::MenuItem("Validate Nodes", NULL, &NodesGraphSettings::ValidateNodesRef())) {}
			if (ImGui::BeginMenu("Snapping"))
			{
				ImGui::MenuItem("Enabled", "", &NodesGraphSettings::NodeSnappingEnabledRef());
				ImGui::SetNextItemWidth(90_dpi);
				ImGui::InputInt("Value", &NodesGraphSettings::NodeSnappingValueRef(), 5, 10);
				ImGui::EndMenu();
			}

			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}
}

static void DrawDirectoryFiles()
{
	_hoveredFile.clear();

	ImGui::SeparatorText("Graphs");
	ImGui::BeginChild("Files", ImVec2(192_dpi, ImGui::GetContentRegionAvail().y - 48_dpi));
	if (_creatingNewFile)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		ImGui::PushItemWidth(192_dpi);

		if (!_newFileInputFocused) {
			ImGui::SetKeyboardFocusHere();
			_newFileInputFocused = true;
		}

		ImGui::InputText("#new_file_name", &_newFileName);
		ImGui::PopItemWidth();
		ImGui::PopStyleVar();

		if (ImGui::IsItemDeactivatedAfterEdit()) {
			std::string fullPath = _directory + "/" + _newFileName + ".sgraph";
			std::ofstream file(fullPath);
			if (file) file.close();

			_creatingNewFile = false;
			_filesLoaded = false;
			_newFileName.clear();
		}
	}

	for (int i = 0; i < _files.size(); i++)
	{
		if (_renamedFile.compare(_files[i]) == 0)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
			ImGui::PushItemWidth(192_dpi);
			ImGui::InputText("#renamed", &_files[i]);
			ImGui::PopItemWidth();
			ImGui::PopStyleVar();

			if (ImGui::IsItemDeactivatedAfterEdit()) {
				SDL_Log("renamed!");
			}
		}
		else
		{
			if (ImGui::Selectable(_files[i].c_str(), !_currentFile.compare(_files[i]), 0)) {
				OpenGraph(_files[i]);
			}

			if (ImGui::IsItemHovered())
				_hoveredFile = _files[i];
		}
	}

	ImGui::EndChild();
}

static void DialogFileCallback(void* userdata, const char* const* filelist, int filter)
{
	if (*filelist != nullptr)
	{
		_directory = filelist[0];
		_filesLoaded = false;
	}
}

static void DrawDirectorySelect()
{
	ImGui::BeginChild("Folder", ImVec2(192_dpi, 0));
	ImGui::SeparatorText("Folder");

	if (_directory.empty()) {
		if (ImGui::Selectable("Select a folder", false))
			SDL_ShowOpenFolderDialog(DialogFileCallback, nullptr, _window, _directory.c_str(), false);
	}
	else {
		auto lastSlashIndex = _directory.find_last_of('/');
		if (lastSlashIndex == std::string::npos)
			lastSlashIndex = _directory.find_last_of('\\');

		auto folderName = _directory.substr(lastSlashIndex);

		if (ImGui::Selectable(folderName.c_str(), false))
			SDL_ShowOpenFolderDialog(DialogFileCallback, nullptr, _window, _directory.c_str(), false);

		ImGui::SetItemTooltip("%s", _directory.c_str());
	}
	ImGui::EndChild();
}

static void DrawTabs()
{
	ImGui::PushStyleVar(ImGuiStyleVar_TabBarBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, 1.0f);

	auto tabBarFlags = ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_DrawSelectedOverline;
	if (ImGui::BeginTabBar("Tabs", tabBarFlags))
	{
		const std::string* closeGraph = nullptr;

		for (const auto& [key, graph] : _openedGraphs) {

			ImGuiTabItemFlags tabItemFlags = ImGuiTabItemFlags_None;
			bool open = true;

			if (_focusedGraph == graph)
				tabItemFlags |= ImGuiTabItemFlags_SetSelected;

			if (graph->HasUnsavedChanges())
				tabItemFlags |= ImGuiTabItemFlags_UnsavedDocument;

			auto isTabSelected = ImGui::BeginTabItem(key.c_str(), &open, tabItemFlags);

			if (ImGui::IsItemClicked()) {
				if (_focusedGraph != graph) {
					_focusedGraph = graph;
					_currentFile = key;
				}
			}

			if (isTabSelected)
				ImGui::EndTabItem();

			if (!open)
				closeGraph = &key;
		}

		if (closeGraph)
		{
			auto graph = _openedGraphs.at(*closeGraph);

			if (graph->HasUnsavedChanges()) {
				_showSavePopup = true;
				_closingGraph = *closeGraph;
			}
			else {
				CloseGraph(*closeGraph);
			}
		}

		ImGui::EndTabBar();
	}

	ImGui::PopStyleVar();
	ImGui::PopStyleVar();
}

static void DrawStatsWindow()
{
	if (!_showStatsWindow) return;

	ImGuiWindowFlags windowFlags =
		ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoFocusOnAppearing |
		ImGuiWindowFlags_NoNav;

	ImGui::SetNextWindowPos(ImVec2(212_dpi, 54_dpi), ImGuiCond_Appearing, ImVec2(0, 0));
	ImGui::SetNextWindowBgAlpha(0.35f);
	ImGui::SetNextWindowSizeConstraints(ImVec2(196_dpi, 0), ImVec2(FLT_MAX, FLT_MAX));

	if (ImGui::Begin("Stats", &_showStatsWindow, windowFlags))
	{
		ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

		if (_focusedGraph) {
			ImGui::Text("Scale: %.2f", _focusedGraph->GetScale());
			ImGui::Text("Scroll: (%.1f, %.1f)", _focusedGraph->GetOffset().x, _focusedGraph->GetOffset().y);
			ImGui::Text("Nodes: %d", (int)_focusedGraph->GetNodes().size());
			ImGui::Text("Connections: %d", (int)_focusedGraph->GetConnections().size());
		}
	}

	ImGui::End();
}

static void DrawHistoryWindow()
{
	if (!_showHistoryWindow) return;

	ImGuiWindowFlags windowFlags =
		ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoFocusOnAppearing |
		ImGuiWindowFlags_NoNav;

	auto windowSize = ImGui::GetMainViewport()->WorkSize;

	ImGui::SetNextWindowPos(ImVec2(windowSize.x - 12_dpi, 54_dpi), ImGuiCond_Appearing, ImVec2(1, 0));
	ImGui::SetNextWindowBgAlpha(0.35f);
	ImGui::SetNextWindowSizeConstraints(ImVec2(196_dpi, 0), ImVec2(196_dpi, 320_dpi));

	if (ImGui::Begin("History", &_showHistoryWindow, windowFlags))
	{
		if (_focusedGraph != nullptr) {
			_Command* commandUndoClicked = nullptr;
			_Command* commandRedoClicked = nullptr;

			auto& redoStack = _focusedGraph->GetRedoStack();
			auto& undoStack = _focusedGraph->GetUndoStack();

			int cmdId = 0;
			for (const auto& it : undoStack)
			{
				ImGui::PushID(cmdId);
				if (ImGui::Selectable(it->GetLabel(), false)) { commandUndoClicked = it; }
				ImGui::PopID();
				cmdId++;
			}

			for (auto it = redoStack.rbegin(); it != redoStack.rend(); ++it)
			{
				ImGui::PushID(cmdId);
				if (ImGui::Selectable((*it)->GetLabel(), false, ImGuiSelectableFlags_Highlight)) { commandRedoClicked = *it; }
				ImGui::PopID();
				cmdId++;
			}

			if (commandUndoClicked) {
				while (undoStack.back() != commandUndoClicked) {
					_focusedGraph->Undo();
				}
			}

			if (commandRedoClicked) {
				while (redoStack.back() != commandRedoClicked) {
					_focusedGraph->Redo();
				}

				_focusedGraph->Redo();
			}
		}
	}

	ImGui::End();
}

static void DrawGraphWindow()
{
	auto childFlags = 0;
	childFlags |= ImGuiChildFlags_Borders;

	auto windowFlags = 0;
	windowFlags |= ImGuiWindowFlags_NoScrollbar;
	windowFlags |= ImGuiWindowFlags_NoMove;
	windowFlags |= ImGuiWindowFlags_NoScrollWithMouse;

	ImGui::BeginChild("Graph", ImVec2(0, 0), childFlags, windowFlags);
	ImGui::PushItemWidth(32_dpi * 6_dpi);

	if (!_focusedGraph)
	{
		auto windowSize = ImGui::GetWindowSize();
		auto textSize = ImGui::CalcTextSize("No graph loaded.");

		float textX = (windowSize.x - textSize.x) * 0.5f;
		float textY = (windowSize.y - textSize.y) * 0.5f;

		ImGui::SetCursorPos(ImVec2(textX, textY));
		ImGui::Text("No graph loaded.");
	}
	else
	{
		_focusedGraph->Draw();
	}

	ImGui::PopItemWidth();
	ImGui::EndChild();
}

static void CheckIfAppIsClosing()
{
	if (_closing) {
		for (auto it = _openedGraphs.begin(); it != _openedGraphs.end(); ) {
			auto& key = it->first;
			auto graph = it->second;

			if (graph->HasUnsavedChanges()) {
				_showSavePopup = true;
				_closingGraph = key;
				break;
			}

			it = _openedGraphs.erase(it);

			if (_focusedGraph == graph) {
				if (_openedGraphs.size() > 0) {
					_focusedGraph = _openedGraphs.begin()->second;
					_currentFile = _openedGraphs.begin()->first;
				}
				else {
					_focusedGraph = nullptr;
					_currentFile.clear();
				}
			}

			delete graph;
		}

		if (!_showSavePopup)
			_done = true;
	}
}

static void DrawPopups()
{
	if (!_hoveredFile.empty() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
		ImGui::OpenPopup(FILE_CONTEXT_MENU);
		_focusedFile = _hoveredFile;
	}

	if (_showSavePopup)
		ImGui::OpenPopup(SAVE_POPUP);

	auto windowFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove;
	if (ImGui::BeginPopupModal(SAVE_POPUP, nullptr, windowFlags))
	{
		ImGui::Text("[%s] has unsaved changes.", _closingGraph.c_str());
		ImGui::Separator();

		ImGui::SetItemDefaultFocus();
		if (ImGui::Button("Save")) {

			auto graph = _openedGraphs[_closingGraph];

			char filename[100];
			SDL_snprintf(filename, 100, "%s/%s.%s", _directory.c_str(), _closingGraph.c_str(), "sgraph");

			auto data = graph->Serialize();
			auto stream = SDL_IOFromFile(filename, "w");
			SDL_WriteIO(stream, data.c_str(), data.size());
			SDL_CloseIO(stream);

			CloseGraph(_closingGraph);
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();
		if (ImGui::Button("Don't Save"))
		{
			CloseGraph(_closingGraph);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine(0, 64_dpi);
		if (ImGui::Button("Cancel")) {
			_closing = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopup(FILE_CONTEXT_MENU)) {

		ImGui::SeparatorText("File");

		if (ImGui::MenuItem("Delete"))
		{
			std::string fullPath = _directory + "/" + _focusedFile + ".sgraph";
			std::remove(fullPath.c_str());
			_filesLoaded = false;
		}

		ImGui::BeginDisabled(true);
		if (ImGui::MenuItem("Rename"))
		{
			_renamedFile = _focusedFile;
		}
		ImGui::EndDisabled();

		ImGui::EndPopup();
	}
}

static void Draw()
{
	_showSavePopup = false;
	if (!_filesLoaded)
		LoadFilesAtDirectory();

	ImGuiWindowFlags windowFlags = 0;
	windowFlags |= ImGuiWindowFlags_MenuBar;
	windowFlags |= ImGuiWindowFlags_NoTitleBar;
	windowFlags |= ImGuiWindowFlags_NoScrollbar;
	windowFlags |= ImGuiWindowFlags_NoMove;
	windowFlags |= ImGuiWindowFlags_NoResize;
	windowFlags |= ImGuiWindowFlags_NoCollapse;
	windowFlags |= ImGuiWindowFlags_NoNav;
	windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus;

	if (_windowHasBeenResized) {
		ImGui::SetNextWindowSize(ImVec2((float)_windowWidth, (float)_windowHeight));
		_windowHasBeenResized = false;
	}

	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
	ImGui::Begin("Nodes Graph", nullptr, windowFlags);

	DrawMenuBar();

	ImGui::BeginChild("Left", ImVec2(192_dpi, 0));
	DrawDirectoryFiles();
	DrawDirectorySelect();
	ImGui::EndChild();

	ImGui::SameLine();
	ImGui::BeginGroup();

	DrawTabs();
	DrawGraphWindow();

	ImGui::EndGroup();
	ImGui::End();

	CheckIfAppIsClosing();

	DrawPopups();

	DrawStatsWindow();
	DrawHistoryWindow();
}

static void RegisterNodes()
{
	NodesGraph::RegisterNode<EntryNode>("Entry");
	NodesGraph::RegisterNode<ExitNode>("Exit");
	NodesGraph::RegisterNode<SpeechNode>("Speech");
	NodesGraph::RegisterNode<ResponsesNode>("Responses");
	NodesGraph::RegisterNode<ActionNode>("Action");
	NodesGraph::RegisterNode<ConnectorInNode>("Connector In");
	NodesGraph::RegisterNode<ConnectorOutNode>("Connector Out");

	NodesGraph::RegisterNodeContextMenu<ConnectorInNode>([](ConnectorInNode* node) {
		if (ImGui::MenuItem("Output")) {
			auto& nodes = NodesGraph::GetCurrent()->GetNodes();
			for (const auto& [_, n] : nodes) {
				auto connectorOutNode = dynamic_cast<ConnectorOutNode*>(n);
				if (connectorOutNode && connectorOutNode->GetValue() == node->GetValue()) {
					NodesGraph::GetCurrent()->FocusOnNode(connectorOutNode);
				}
			}
		}
		});
}

static void HandleInput()
{
	if (!_focusedGraph) return;

	if ((ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_Y)) ||
		(ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyDown(ImGuiKey_LeftShift) && ImGui::IsKeyPressed(ImGuiKey_Z)))
		_focusedGraph->Redo();

	else if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_Z))
		_focusedGraph->Undo();

	if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_S)) {
		auto data = _focusedGraph->Serialize();

		char filename[100];
		SDL_snprintf(filename, 100, "%s/%s.%s", _directory.c_str(), _currentFile.c_str(), "sgraph");

		auto stream = SDL_IOFromFile(filename, "w");
		SDL_WriteIO(stream, data.c_str(), data.size());
		SDL_CloseIO(stream);
	}
}

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

int main(int, char**)
{
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
#endif

	// Setup SDL
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		SDL_Log("Error: SDL_Init(): %s\n", SDL_GetError());
		return -1;
	}

	auto contentScale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
	_windowWidth *= contentScale;
	_windowHeight *= contentScale;
	SDL_WindowFlags windowFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
	_window = SDL_CreateWindow("nodes-graph", (int)(_windowWidth), (int)(_windowHeight), windowFlags);
	if (_window == nullptr)
	{
		SDL_Log("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
		return -1;
	}

	SDL_Renderer* renderer = SDL_CreateRenderer(_window, nullptr);
	SDL_SetRenderVSync(renderer, 1);
	if (renderer == nullptr)
	{
		SDL_Log("Error: SDL_CreateRenderer(): %s\n", SDL_GetError());
		return -1;
	}

	SDL_SetWindowPosition(_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	SDL_ShowWindow(_window);

	SDL_AddEventWatch(SDLEventWatch, nullptr);

	// Setup ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImGuiSettingsHandler settingsHandler;
	settingsHandler.TypeName = "UserData";
	settingsHandler.TypeHash = ImHashStr("UserData");
	settingsHandler.ReadOpenFn = UserData_ReadOpen;
	settingsHandler.ReadLineFn = UserData_ReadLine;
	settingsHandler.WriteAllFn = UserData_WriteAll;
	ImGui::AddSettingsHandler(&settingsHandler);

	ImGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();
	style.ScaleAllSizes(contentScale);
	style.FontScaleDpi = contentScale;

	ImGui_ImplSDL3_InitForSDLRenderer(_window, renderer);
	ImGui_ImplSDLRenderer3_Init(renderer);

	NodesGraphSettings::SetDpiScale(contentScale);
	RegisterNodes();

	while (!_done)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSDL3_ProcessEvent(&event);
			if (event.type == SDL_EVENT_QUIT ||
				(event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(_window))) {
				_closing = true;
			}
		}

		if (SDL_GetWindowFlags(_window) & SDL_WINDOW_MINIMIZED)
		{
			SDL_Delay(10);
			continue;
		}

		ImGui_ImplSDLRenderer3_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();

		HandleInput();
		Draw();

		ImGui::Render();
		SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);

		auto& clearColor = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
		SDL_SetRenderDrawColorFloat(renderer, clearColor.x, clearColor.y, clearColor.z, clearColor.w);
		SDL_RenderClear(renderer);
		ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
		SDL_RenderPresent(renderer);
	}

	ImGui_ImplSDLRenderer3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(_window);
	SDL_Quit();

	return 0;
}