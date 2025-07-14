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
#define MAX_BUFFER_SIZE ((int)5 * (int)1024 * (int)1024) // 5MB buffer

class FileExplorerApp
{
public:
	FileExplorerApp();
	~FileExplorerApp();
	void Run();

private:
	// Function to render the main menu bar
	void RenderMainMenuBar(bool& open, bool& save, bool& create_new_folder, bool& create_new_file, bool& rename_file, bool& _delete);

	// Function to apply keyboard shortcuts
	void ApplyShortcuts(bool& open, bool& save, bool& create_new_folder, bool& create_new_file, bool& rename_file);

	// Function to process the file browser dialog
	void ProcessFileBrowserDialog(bool& open);

	// Function to process saving a file
	void ProcessSaveFile(bool& save);

	// Function to handle error popups
	void HandleErrorPopup();

	// Function to handle the "Create Folder" popup
	void HandleCreateFolderPopup(bool& create_new_folder);

	// Function to handle the "Create File" popup
	void HandleCreateFilePopup(bool& create_new_file);

	// Function to handle the "Rename" popup
	void HandleRenamePopup(bool& rename_file);

	// Function to handle the "Delete" popup
	void HandleDeletePopup(bool& _delete);

	// Function to render the explorer side panel
	void RenderExplorerPanel(float menu_bar_height, bool& open);

	// Function to update side menu width for resizing
	void UpdateSideMenuWidth();

	// Function to render the file viewer/editor
	void RenderFileViewer(float menu_bar_height);

	// Function to format file sizes
	string format_size(uintmax_t size_in_bytes);

	// Function to get files in a directory
	map<string, string> get_files_in_directory(const fs::path& path);

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
