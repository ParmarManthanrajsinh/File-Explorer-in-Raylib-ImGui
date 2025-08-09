#pragma once

#include <rlImGui.h>
#include <imgui.h>
#include <imfilebrowser.h>
#include <filesystem>
#include <map>
#include <fstream>
#include <array>
#include <string>
#include <vector>
using namespace std;

namespace fs = std::filesystem;
constexpr int ce_MAXBUFFERSIZE = 5 * 1024 * 1024; // 5MB buffer

class FileExplorerApp
{
public:
	FileExplorerApp();
	~FileExplorerApp();
	void Run();

private:
	// Function to render the main menu bar
	void RenderMainMenuBar
	(
		bool& b_Open, 
		bool& b_Save, 
		bool& b_CreateNewFolder, 
		bool& b_CreateNewFile, 
		bool& b_RenameFile, 
		bool& _delete
	);

	// Function to apply keyboard shortcuts
	void ApplyShortcuts
	(
		bool& b_Open, 
		bool& b_Save, 
		bool& b_CreateNewFolder, 
		bool& b_CreateNewFile, 
		bool& b_RenameFile
	);

	// Function to process the file browser dialog
	void ProcessFileBrowserDialog(bool& b_Open);

	// Function to process saving a file
	void ProcessSaveFile(bool& b_Save);

	// Function to handle error popups
	void HandleErrorPopup();

	// Function to handle the "Create Folder" popup
	void HandleCreateFolderPopup(bool& b_CreateNewFolder);

	// Function to handle the "Create File" popup
	void HandleCreateFilePopup(bool& b_CreateNewFile);

	// Function to handle the "Rename" popup
	void HandleRenamePopup(bool& b_RenameFile);

	// Function to handle the "Delete" popup
	void HandleDeletePopup(bool& _delete);

	// Function to render the explorer side panel
	void RenderExplorerPanel(float menu_bar_height, bool& b_Open);

	// Function to update side menu width for resizing
	void UpdateSideMenuWidth();

	// Function to render the file viewer/editor
	void RenderFileViewer(float menu_bar_height);

	// Function to format file sizes
	string FormatSize(uintmax_t size_in_bytes);

	// Function to get files in a directory
	map<string, string> GetFilesInDirectory(const fs::path& path);

	// Member variables
	ImGui::FileBrowser file_browser;
	fs::path current_path;
	fs::path selected_file;
	string file_content;
	bool b_FileLoaded;
	bool b_FileModified;
	bool b_Exit;

	Texture2D file_icon;
	Texture2D folder_icon;
	Texture2D img_icon;
	Texture2D edit_file_icon;

	Texture2D img_texture;
	bool b_ImgLoaded;
	fs::path loaded_img_path;

	bool b_ShowSaveDialog;
	bool b_ShowErrorPopup;
	string error_message;
	float side_menu_width;

	array<string, 33> supported_file_types;
	array<string, 3> supported_img_types;
};
