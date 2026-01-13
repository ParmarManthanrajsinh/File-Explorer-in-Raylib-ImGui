#pragma once

#include <format>
#include <array>
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <rlImGui.h>
#include <imfilebrowser.h>
#include <map>
#include <string>
#include <vector>
#include <misc/cpp/imgui_stdlib.h>
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
		bool& b_Delete
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
	void HandleDeletePopup(bool& b_Delete);

	// Function to render the explorer side panel
	void RenderExplorerPanel(float menu_bar_height, bool& b_Open);

	// Function to update side menu width for resizing
	void UpdateSideMenuWidth();

	// Function to render the file viewer/editor
	void RenderFileViewer(float menu_bar_height);

	// Function to format file sizes
	string FormatSize(double size_in_bytes);

	// Function to get files in a directory
	map<string, string> GetFilesInDirectory(const fs::path& path);

	// Member variables
	ImGui::FileBrowser m_FileBrowser;
	fs::path current_path;
	fs::path m_SelectedFile;
	string m_FileContent;
	bool m_bFileLoaded;
	bool m_bFileModified;
	bool m_bExit;

	Texture2D m_FileIcon;
	Texture2D m_FolderIcon;
	Texture2D m_ImgIcon;
	Texture2D m_EditFileIcon;

	Texture2D m_ImgTexture;
	bool m_bImgLoaded;
	fs::path m_LoadedImgPath;

	bool m_bShowSaveDialog;
	bool m_bShowErrorPopup;
	string m_ErrorMessage;
	float m_SideMenuWidth;

	array<string, 33> m_SupportedFileTypes;
	array<string, 3> m_SupportedImgTypes;
};
