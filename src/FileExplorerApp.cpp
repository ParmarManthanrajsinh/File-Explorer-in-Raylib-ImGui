#include "FileExplorerApp.hpp"
#include "ImGuiCustomTheme.h"

FileExplorerApp::FileExplorerApp()
{
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(900, 500, "File Explorer");
	Image img = LoadImage("assets/file_explorer_icon.png");
	SetWindowIcon(img);
	UnloadImage(img);

	SetTargetFPS(120);

	rlImGuiSetup(true);
	ImCustomTheme();

	// Load Icon
	m_FileIcon = LoadTexture("assets/icons/file.png");
	m_FolderIcon = LoadTexture("assets/icons/folder.png");
	m_ImgIcon = LoadTexture("assets/icons/image.png");
	m_EditFileIcon = LoadTexture("assets/icons/edit_file.png");

	// Initialize File Browser
	m_FileBrowser = ImGui::FileBrowser
	(
		ImGuiFileBrowserFlags_SelectDirectory |
		ImGuiFileBrowserFlags_EnterNewFilename |
		ImGuiFileBrowserFlags_NoModal |
		ImGuiFileBrowserFlags_NoStatusBar
	);

	current_path = "";			// Start with the current working directory
	m_SelectedFile = fs::path(); // To store the selected file path
	m_FileContent.clear();		// Store file content for editing
	m_bFileLoaded = false;		// Track if file is loaded
	m_bFileModified = false;		// Track if file has been modified
	m_bExit = false;

	// Image handling variables
	m_ImgTexture = { 0 };			  // Initialize to empty texture
	m_bImgLoaded = false;			  // Track if image is loaded
	m_LoadedImgPath = fs::path(); // Track which image is currently loaded

	// UI state variables
	m_bShowSaveDialog = false;
	m_bShowErrorPopup = false;
	m_ErrorMessage.clear();
	m_SideMenuWidth = 300.0f; // Make resizable

	m_SupportedFileTypes =
	{
		".txt", ".cpp", ".h",
		".hpp", ".c", ".py",
		".js", ".html",".css",
		".json", ".md", ".xml",
		".yaml", ".ini", ".log",
		".bat", ".sh", ".php",
		".rb", ".go", ".swift",
		".ts", ".tsx", ".vue",
		".sql", ".pl", ".lua",
		".r", ".dart", ".scala",
		".rs", ".java", ".kt"
	};

	m_SupportedImgTypes = {
		".jpg", ".png", ".bmp" };
}

FileExplorerApp::~FileExplorerApp()
{
	// Clean up loaded texture before closing
	if (m_bImgLoaded && m_ImgTexture.id != 0)
	{
		UnloadTexture(m_ImgTexture);
	}
	rlImGuiShutdown();
	CloseWindow();
}

void FileExplorerApp::Run()
{
	while (!WindowShouldClose() && !m_bExit)
	{
		BeginDrawing();
		ClearBackground(BLACK);

		rlImGuiBegin();

		static bool sb_Open = false;
		static bool sb_Save = false;
		static bool sb_CreateNewFolder = false;
		static bool sb_CreateNewFile = false;
		static bool sb_RenameFile = false;
		static bool sb_Delete = false;

		ImGui::GetStyle().FramePadding.y = 6.0f;

		RenderMainMenuBar
		(
			sb_Open,
			sb_Save,
			sb_CreateNewFolder,
			sb_CreateNewFile,
			sb_RenameFile,
			sb_Delete
		);

		float menu_bar_height = ImGui::GetFrameHeight();

		ApplyShortcuts
		(
			sb_Open,
			sb_Save,
			sb_CreateNewFolder,
			sb_CreateNewFile,
			sb_RenameFile
		);

		ProcessFileBrowserDialog(sb_Open);

		ProcessSaveFile(sb_Save);

		HandleErrorPopup();

		HandleCreateFolderPopup(sb_CreateNewFolder);

		HandleCreateFilePopup(sb_CreateNewFile);

		HandleRenamePopup(sb_RenameFile);

		HandleDeletePopup(sb_Delete);

		RenderExplorerPanel(menu_bar_height, sb_Open);

		UpdateSideMenuWidth();

		RenderFileViewer(menu_bar_height);

		rlImGuiEnd();
		EndDrawing();
	}
}

// Function to render the main menu bar
void FileExplorerApp::RenderMainMenuBar
(
	bool& b_Open,
	bool& b_Save,
	bool& b_CreateNewFolder,
	bool& b_CreateNewFile,
	bool& b_RenameFile,
	bool& b_Delete
)
{
	// Main Menu Bar
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Open Directory", "Ctrl+O", false))
			{
				b_Open = true;
				m_FileBrowser.Open();
			}

			if
			(
				ImGui::MenuItem
				(
					"Save",
					"Ctrl+S",
					false,
					m_SelectedFile != fs::path() 
					&& m_bFileLoaded
				)
			)
			{
				b_Save = true;
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Exit", "Escape"))
			{
				m_bExit = true;
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Edit"))
		{
			if (ImGui::MenuItem("New Folder", "Ctrl+Shift+N"))
			{
				b_CreateNewFolder = true;
			}
			if (ImGui::MenuItem("New File", "Ctrl+N"))
			{
				b_CreateNewFile = true;
			}
			if (ImGui::MenuItem("Rename", "F2", false))
			{
				b_RenameFile = true;
			}
			if (ImGui::MenuItem("Delete"))
			{
				b_Delete = true;
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Help"))
		{
			if (ImGui::MenuItem("About"))
			{
				m_ErrorMessage = "Simple File Explorer\nBuilt with Raylib and ImGui";
				m_bShowErrorPopup = true;
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
}

// Function to apply keyboard shortcuts
void FileExplorerApp::ApplyShortcuts
(
	bool& b_Open,
	bool& b_Save,
	bool& b_CreateNewFolder,
	bool& b_CreateNewFile,
	bool& b_RenameFile
)
{
	// Handle keyboard shortcuts
	if (ImGui::IsKeyPressed(ImGuiKey_O)
		&& ImGui::GetIO().KeyCtrl)
	{
		b_Open = true;
		m_FileBrowser.Open();
	}

	if (ImGui::IsKeyPressed(ImGuiKey_S)
		&& ImGui::GetIO().KeyCtrl
		&& m_SelectedFile != fs::path()
		&& m_bFileLoaded)
	{
		b_Save = true;
	}

	if (ImGui::IsKeyPressed(ImGuiKey_N)
		&& ImGui::GetIO().KeyCtrl
		&& !ImGui::GetIO().KeyShift)
	{
		b_CreateNewFile = true;
	}
	if (ImGui::IsKeyPressed(ImGuiKey_N)
		&& ImGui::GetIO().KeyCtrl
		&& ImGui::GetIO().KeyShift)
	{
		b_CreateNewFolder = true;
	}
	if (ImGui::IsKeyPressed(ImGuiKey_F2))
	{
		b_RenameFile = true;
	}
}

// Function to process the file browser dialog
void FileExplorerApp::ProcessFileBrowserDialog(bool& b_Open)
{
	// File Browser Dialog
	if (b_Open)
	{
		m_FileBrowser.Display();

		if (m_FileBrowser.HasSelected())
		{
			b_Open = false;
			fs::path new_path = m_FileBrowser.GetDirectory();
			if (fs::exists(new_path) && fs::is_directory(new_path))
			{
				current_path = new_path;

				// Clean up any loaded resources when changing directory
				if (m_bImgLoaded && m_ImgTexture.id != 0)
				{
					UnloadTexture(m_ImgTexture);
					m_bImgLoaded = false;
					m_LoadedImgPath = fs::path();
				}
				m_SelectedFile = fs::path();
				m_bFileLoaded = false;
				m_bFileModified = false;
				m_FileContent.clear();
			}
			m_FileBrowser.ClearSelected();
			m_FileBrowser.Close();
		}
	}
}

// Function to process saving a file
void FileExplorerApp::ProcessSaveFile(bool& b_Save)
{
	// Save File
	if (b_Save
		&& m_SelectedFile != fs::path()
		&& m_bFileLoaded)
	{
		ofstream out_file(m_SelectedFile, ios::out | ios::binary);
		if (out_file.is_open())
		{
			out_file.write(m_FileContent.data(), m_FileContent.size());
			out_file.close();
			m_bFileModified = false;
			b_Save = false;
		}
		else
		{
			m_ErrorMessage = "Could not save file: " + m_SelectedFile.string();
			m_bShowErrorPopup = true;
			b_Save = false;
		}
	}
}

// Function to handle error popups
void FileExplorerApp::HandleErrorPopup()
{
	// Error Popup
	if (m_bShowErrorPopup)
	{
		ImGui::OpenPopup("Error");
		m_bShowErrorPopup = false;
		m_SelectedFile = fs::path();
	}

	if 
	(
		ImGui::BeginPopupModal
		(
			"Error", 
			nullptr, 
			ImGuiWindowFlags_AlwaysAutoResize
		)
	)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255)); // Red color
		ImGui::TextWrapped("%s", m_ErrorMessage.c_str());
		ImGui::PopStyleColor();

		ImGui::Separator();
		if (ImGui::Button("OK", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

// Function to handle the "Create Folder" popup
void FileExplorerApp::HandleCreateFolderPopup(bool& b_CreateNewFolder)
{
	// New Folder Popup
	if (b_CreateNewFolder)
	{
		ImGui::OpenPopup("Create Folder");
		b_CreateNewFolder = false;
	}

	if (ImGui::BeginPopupModal("Create Folder", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		static char s_FolderName[128] = "";
		ImGui::InputText("Folder Name", s_FolderName, sizeof(s_FolderName));
		if (ImGui::Button("Create"))
		{
			fs::path new_folder_path = current_path / s_FolderName;
			if (!fs::exists(new_folder_path))
			{
				fs::create_directory(new_folder_path);
				current_path = new_folder_path; // Change to the new folder
				m_SelectedFile = fs::path();		// Reset selected file
				m_bFileLoaded = false;
				m_bFileModified = false;
				m_FileContent.clear();
			}
			else
			{
				m_ErrorMessage = "Folder already exists!";
				m_bShowErrorPopup = true;
			}
			s_FolderName[0] = '\0';
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

// Function to handle the "Create File" popup
void FileExplorerApp::HandleCreateFilePopup(bool& b_CreateNewFile)
{
	// New File Popup
	if (b_CreateNewFile)
	{
		ImGui::OpenPopup("Create File");
		b_CreateNewFile = false;
	}

	if 
	(
		ImGui::BeginPopupModal
		(
			"Create File", 
			nullptr, 
			ImGuiWindowFlags_AlwaysAutoResize
		)
	)
	{
		static char s_NewFileName[128] = "";
		ImGui::InputText("File Name", s_NewFileName, sizeof(s_NewFileName));

		if (ImGui::Button("Create"))
		{
			fs::path new_file_path = current_path / s_NewFileName;
			if (!fs::exists(new_file_path))
			{
				ofstream file(new_file_path);
				if (file.is_open())
				{
					file.close();
					m_SelectedFile = new_file_path;
					m_bFileLoaded = false;
					m_bFileModified = false;
					m_FileContent.clear();
				}
				else
				{
					m_ErrorMessage = "Could not create file: " + new_file_path.string();
					m_bShowErrorPopup = true;
				}
			}
			else
			{
				m_ErrorMessage = "File already exists!";
				m_bShowErrorPopup = true;
			}
			s_NewFileName[0] = '\0';
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

// Function to handle the "Rename" popup
void FileExplorerApp::HandleRenamePopup(bool& b_RenameFile)
{
	// Rename File Popup
	if (b_RenameFile)
	{
		ImGui::OpenPopup("Rename");
		b_RenameFile = false;
	}

	if 
	(
		ImGui::BeginPopupModal
		(
			"Rename", nullptr, ImGuiWindowFlags_AlwaysAutoResize
		)
	)
	{
		static char s_NewName[128] = "";
		static bool sb_FirstFrame = true;

		bool b_RenamingSelectedFile = !m_SelectedFile.empty()
			&& fs::exists(m_SelectedFile);

		bool b_IsDir = b_RenamingSelectedFile ?
			fs::is_directory(m_SelectedFile) : fs::is_directory(current_path);

		// Initialize the input field with current filename when popup first opens
		if (sb_FirstFrame)
		{
			if (b_RenamingSelectedFile)
			{
				// Renaming the selected file/folder
				strncpy
				(
					s_NewName,
					m_SelectedFile.filename().string().c_str(),
					sizeof(s_NewName) - 1
				);
			}
			else
			{
				// Renaming the current directory
				strncpy
				(
					s_NewName,
					current_path.filename().string().c_str(),
					sizeof(s_NewName) - 1
				);
			}
			s_NewName[sizeof(s_NewName) - 1] = '\0';
			sb_FirstFrame = false;
		}

		ImGui::InputText
		(
			b_IsDir ? "New Folder Name" : "New File Name",
			s_NewName,
			sizeof(s_NewName)
		);

		if (ImGui::Button("Rename"))
		{
			// Validate input
			if (strlen(s_NewName) == 0)
			{
				m_ErrorMessage = "Name cannot be empty!";
				m_bShowErrorPopup = true;
			}
			else
			{
				fs::path source_path, new_path;

				if (b_RenamingSelectedFile)
				{
					source_path = m_SelectedFile;
					new_path = m_SelectedFile.parent_path() / s_NewName;
				}
				else
				{
					source_path = current_path;
					new_path = current_path.parent_path() / s_NewName;
				}

				if (!fs::exists(new_path))
				{
					try
					{
						fs::rename(source_path, new_path);

						// Update paths after successful rename
						if (b_RenamingSelectedFile)
						{
							m_SelectedFile = new_path;
						}
						else
						{
							current_path = new_path;
							m_SelectedFile = fs::path(); // Reset selected file
						}

						// Reset file state
						m_bFileLoaded = false;
						m_bFileModified = false;
						m_FileContent.clear();
					}
					catch (const fs::filesystem_error& ex)
					{
						m_ErrorMessage = string("Error renaming ")
							+ (b_IsDir ? "folder" : "file")
							+ ": "
							+ ex.what();

						m_bShowErrorPopup = true;
					}
				}
				else
				{
					m_ErrorMessage =
						string(b_IsDir ? "Folder" : "File")
						+ " already exists!";

					m_bShowErrorPopup = true;
				}
			}

			// Reset for next use and close popup
			memset(s_NewName, 0, sizeof(s_NewName));
			sb_FirstFrame = true;
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
		{
			// Reset for next use and close popup
			memset
			(
				s_NewName, 
				0, 
				sizeof(s_NewName)
			);
			sb_FirstFrame = true;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

// Function to handle the "Delete" popup
void FileExplorerApp::HandleDeletePopup(bool& b_Delete)
{
	// Delete File and Folder Popup
	if (b_Delete)
	{
		ImGui::OpenPopup("Delete");
		b_Delete = false;
	}

	if (ImGui::BeginPopupModal("Delete", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		bool b_RenamingSelectedFile = !m_SelectedFile.empty()
			&& fs::exists(m_SelectedFile);

		bool b_IsDir = b_RenamingSelectedFile ?
			fs::is_directory(m_SelectedFile) : fs::is_directory(current_path);

		if (b_RenamingSelectedFile)
		{
			ImGui::Text
			(
				"Are you sure you want to delete '%s'?",
				m_SelectedFile.filename().string().c_str()
			);
		}
		else
		{
			ImGui::Text
			(
				"Are you sure you want to delete the current directory '%s'?", current_path.filename().string().c_str()
			);
		}

		// Show confirmation buttons
		if (ImGui::Button("Delete", ImVec2(120, 0)))
		{
			try
			{
				if (b_RenamingSelectedFile)
				{
					fs::remove_all(m_SelectedFile);
					m_SelectedFile = fs::path();
				}
				else
				{
					fs::remove_all(current_path);
					current_path = fs::current_path();
					m_SelectedFile = fs::path();
				}
				m_bFileLoaded = false;
				m_bFileModified = false;
				m_FileContent.clear();
			}
			catch (const fs::filesystem_error& ex)
			{
				m_ErrorMessage = "Error deleting file/folder: " 
								 + string(ex.what());
				m_bShowErrorPopup = true;
			}
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

// Function to render the explorer side panel
void FileExplorerApp::RenderExplorerPanel(float menu_bar_height, bool& b_Open)
{
	// Explorer Side Panel (Resizable)
	ImGui::SetNextWindowPos
	(
		ImVec2(0, menu_bar_height),
		ImGuiCond_Always
	);

	ImGui::SetNextWindowSize
	(
		ImVec2
		(
			m_SideMenuWidth, 
			static_cast<float>(GetScreenHeight() - menu_bar_height)
		),
		ImGuiCond_Always
	);

	ImGui::Begin
	(
		"Explorer",
		nullptr,
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize
	);

	if (current_path == "")
	{
		ImGui::Text("No folder opened\n");
		if (ImGui::Button("Open Folder", ImVec2(-1, 0)))
		{
			b_Open = true;
			m_FileBrowser.Open();
		}
		ImGui::End();
		return;
	}

	// Current path display with better formatting
	ImGui::Text("Current Directory:");
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));

	ImGui::BeginChild("PathDisplay", ImVec2(0, 40), true);
	ImGui::Text("%s", current_path.filename().string().c_str());
	ImGui::EndChild();

	ImGui::PopStyleColor();

	// Navigation section
	ImGui::BeginChild("Navigation", ImVec2(0, -1), false);

	// Back button with better styling
	if (current_path.has_parent_path())
	{
		if (ImGui::Selectable("Back", false, ImGuiSelectableFlags_None))
		{
			current_path = current_path.parent_path();
			// Clean up loaded resources when navigating back
			if (m_bImgLoaded && m_ImgTexture.id != 0)
			{
				UnloadTexture(m_ImgTexture);
				m_bImgLoaded = false;
				m_LoadedImgPath = fs::path();
			}
			m_SelectedFile = fs::path();
			m_bFileLoaded = false;
			m_bFileModified = false;
			m_FileContent.clear();
		}
		ImGui::Separator();
	}

	// Get and display files with better organization
	auto files = GetFilesInDirectory(current_path);

	// Separate directories and files
	vector<pair<string, string>> directories;
	vector<pair<string, string>> regular_files;

	for (const auto& file : files)
	{
		if (file.second == "[D]")
			directories.emplace_back(file);
		else
			regular_files.emplace_back(file);
	}

	// Display directories first
	if (!directories.empty())
	{
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 5.0f, 1.0f), "Directories:");
		for (const auto& dir : directories)
		{
			string label = dir.first;
			bool b_IsSelected = (m_SelectedFile == current_path / dir.first);

			// Start a group to keep icon and text together
			ImGui::BeginGroup();

			// Draw the icon first
			rlImGuiImage(&m_FolderIcon);
			ImGui::SameLine();

			ImVec2 cursor_pos = ImGui::GetCursorPos();
			ImGui::SetCursorPos(ImVec2(cursor_pos.x - 6.0f, cursor_pos.y + 6.0f));

			// Then draw the selectable
			if (ImGui::Selectable(label.c_str(), b_IsSelected))
			{
				current_path /= dir.first;
				// Clean up any loaded resources
				if (m_bImgLoaded && m_ImgTexture.id != 0)
				{
					UnloadTexture(m_ImgTexture);
					m_bImgLoaded = false;
					m_LoadedImgPath = fs::path();
				}
				m_SelectedFile = fs::path();
				m_bFileLoaded = false;
				m_bFileModified = false;
				m_FileContent.clear();
			}

			ImGui::EndGroup();
		}
	}

	// Display files
	if (!regular_files.empty())
	{
		ImGui::TextColored(ImVec4(0.7f, 1.0f, 0.7f, 1.0f), "Files:");
		for (const auto& file : regular_files)
		{
			fs::path file_path = current_path / file.first;
			bool b_IsSelected = (m_SelectedFile == file_path);
			Texture2D icon = m_FileIcon;
			string ext = fs::path(file.first).extension().string();

			if
			(
				find
				(
					m_SupportedImgTypes.begin(),
					m_SupportedImgTypes.end(),
					ext
				) != m_SupportedImgTypes.end()
			)
			{
				icon = m_ImgIcon;
			}

			else if
			(
				find
				(
					m_SupportedFileTypes.begin(),
					m_SupportedFileTypes.end(),
					ext
				) != m_SupportedFileTypes.end()
			)
			{
				icon = m_EditFileIcon;
			}

			string label = file.first + " (" + file.second + ")";

			// Start a group to keep icon and text together
			ImGui::BeginGroup();

			// Draw the icon first
			rlImGuiImage(&icon);
			ImGui::SameLine();

			// Add padding if needed
			ImVec2 cursor_pos = ImGui::GetCursorPos();
			ImGui::SetCursorPos(ImVec2(cursor_pos.x - 6.0f, cursor_pos.y + 6.0f));

			// Then draw the selectable
			if (ImGui::Selectable(label.c_str(), b_IsSelected))
			{
				// Only process if it's a different file
				if (m_SelectedFile != file_path)
				{
					// Clean up previous resources
					if (m_bImgLoaded && m_ImgTexture.id != 0)
					{
						UnloadTexture(m_ImgTexture);
						m_bImgLoaded = false;
						m_LoadedImgPath = fs::path();
					}
					m_SelectedFile = file_path;
					m_bFileLoaded = false;
					m_bFileModified = false;
					m_FileContent.clear();
				}
			}

			ImGui::EndGroup();
		}
	}

	ImGui::Separator();

	if (files.empty())
	{
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Directory is empty");
	}

	ImGui::EndChild(); // End Navigation
	ImGui::End();	   // End Explorer window
}

// Function to update side menu width for resizing
void FileExplorerApp::UpdateSideMenuWidth()
{
	if (ImGui::IsWindowHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
	{
		m_SideMenuWidth = ImGui::GetMousePos().x;
		m_SideMenuWidth = 
		max
		(
			200.0f, min(m_SideMenuWidth, GetScreenWidth() * 0.6f)
		);
	}
}

// Function to render the file viewer/editor
void FileExplorerApp::RenderFileViewer(float menu_bar_height)
{
	// File Editor/Viewer Window
	if (m_SelectedFile != fs::path())
	{
		string file_name = m_SelectedFile.filename().string();
		string window_title = file_name;
		if (m_bFileModified)
		{
			window_title += " *";
		}

		ImGui::SetNextWindowPos
		(
			ImVec2
			(
				m_SideMenuWidth + 5, 
				menu_bar_height
			), 
			ImGuiCond_Always
		);

		ImGui::SetNextWindowSize
		(
			ImVec2
			(
				static_cast<float>(GetScreenWidth() - m_SideMenuWidth - 5),
				static_cast<float>(GetScreenHeight() - menu_bar_height)
			),
			ImGuiCond_Always
		);

		ImGui::Begin
		(
			window_title.c_str(), 
			nullptr, 
			ImGuiWindowFlags_NoCollapse | 
			ImGuiWindowFlags_NoMove | 
			ImGuiWindowFlags_NoResize
		);

		// File info header
		ImGui::Text("File: %s", m_SelectedFile.filename().string().c_str());
		ImGui::Text("Path: %s", m_SelectedFile.parent_path().string().c_str());
		try
		{
			ImGui::Text("Size: %s", FormatSize(fs::file_size(m_SelectedFile)).c_str());
		}
		catch (...)
		{
			ImGui::Text("Size: Unknown");
		}
		ImGui::Separator();

		string file_ext = m_SelectedFile.extension().string();

		// Handle text files
		if 
		(
			find
			(
				m_SupportedFileTypes.begin(), 
				m_SupportedFileTypes.end(), 
				file_ext
			) != m_SupportedFileTypes.end()
		)
		{
			if (!m_bFileLoaded)
			{
				ifstream file(m_SelectedFile, ios::in | ios::binary);
				if (file.is_open())
				{
					file.seekg(0, ios::end);
					size_t fileSize = file.tellg();
					file.seekg(0, ios::beg);

					if (fileSize > ce_MAXBUFFERSIZE)
					{
						m_ErrorMessage = "File too large! Maximum size: " 
							+ to_string(ce_MAXBUFFERSIZE / (1024 * 1024)) 
							+ " MB";

						m_bShowErrorPopup = true;
					}
					else
					{
						m_FileContent.resize(fileSize + 1024); // Extra buffer for editing
						file.read(&m_FileContent[0], fileSize);
						m_FileContent[fileSize] = '\0'; // Null terminate
						m_FileContent.resize(fileSize);
						m_bFileLoaded = true;
						m_bFileModified = false;
					}
					file.close();
				}
				else
				{
					m_ErrorMessage = "Could not open file: "
									 + m_SelectedFile.string();
				}
			}

			if (m_bFileLoaded)
			{
				// Create a larger buffer for editing
				static string s_EditBuffer;
				if (s_EditBuffer.size() != m_FileContent.size() + 1024)
				{
					s_EditBuffer = m_FileContent;
					s_EditBuffer.resize(m_FileContent.size() + 1024);
				}

				if 
				(
					ImGui::InputTextMultiline
					(
						"##editor", 
						&s_EditBuffer[0], 
						s_EditBuffer.capacity(),
						ImVec2(-1, -1), 
						ImGuiInputTextFlags_AllowTabInput
					)
				)
				{
					m_FileContent = s_EditBuffer.c_str(); // Update content
					m_bFileModified = true;
				}
			}
		}

		// Handle image files
		else if 
		(
			find
			(
				m_SupportedImgTypes.begin(), 
				m_SupportedImgTypes.end(), 
				file_ext
			) != m_SupportedImgTypes.end()
		)
		{
			// Only load texture if it's a different file or not loaded yet
			if (!m_bImgLoaded || m_LoadedImgPath != m_SelectedFile)
			{
				// Unload previous texture if one was loaded
				if (m_bImgLoaded && m_ImgTexture.id != 0)
				{
					UnloadTexture(m_ImgTexture);
				}

				// Try to load the new image
				Image img = LoadImage(m_SelectedFile.string().c_str());
				if (img.data != nullptr)
				{
					m_ImgTexture = LoadTextureFromImage(img);
					UnloadImage(img); // Free the image data, keep only the texture
					m_bImgLoaded = true;
					m_LoadedImgPath = m_SelectedFile;
				}
				else
				{
					m_ErrorMessage = "Failed to load image: " + m_SelectedFile.filename().string();
					m_bShowErrorPopup = true;
					m_bImgLoaded = false;
				}
			}

			// Display the image if loaded successfully
			if (m_bImgLoaded && m_ImgTexture.id != 0)
			{
				// Calculate display size while maintaining aspect ratio
				float img_width = static_cast<float>(m_ImgTexture.width);
				float img_height = static_cast<float>(m_ImgTexture.height);

				// Leave some margins
				float available_width = 
					ImGui::GetContentRegionAvail().x - 20; 

				float available_height = 
					ImGui::GetContentRegionAvail().y - 20; 

				float scale_x = available_width / img_width;
				float scale_y = available_height / img_height;
				float scale = min(scale_x, scale_y);

				// Don't scale up small images too much
				if (scale > 2.0f)
					scale = 2.0f;

				float display_width = img_width * scale;
				float display_height = img_height * scale;

				ImGui::Text
				(
					"Dimensions: %dx%d pixels", 
					m_ImgTexture.width, 
					m_ImgTexture.height
				);
				ImGui::Text("Display Scale: %.2f", scale);
				ImGui::Separator();

				// Center the image horizontally
				float cursor_x = (available_width - display_width) * 0.5f;

				if (cursor_x > 0)
				{
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + cursor_x);
				}

				// Use scrollable child window for large images
				ImGui::BeginChild
				(
					"ImageView", 
					ImVec2(0, 0), 
					false, 
					ImGuiWindowFlags_HorizontalScrollbar
				);

				rlImGuiImageSize
				(
					&m_ImgTexture, static_cast<int>(display_width), 
					static_cast<int>(display_height)
				);
				ImGui::EndChild();
			}
		}
		else
		{
			ImGui::TextColored
			(
				ImVec4
				(
					1.0f, 0.6f, 0.0f, 1.0f
				), 
				"File format not supported for preview"
			);

			ImGui::Text("Extension: %s", file_ext.c_str());
			ImGui::Separator();
			ImGui::Text("Supported text formats:");
			ImGui::BulletText
			(
				"Code files: .cpp, .h, .py, .js, .html, .css, etc."
			);

			ImGui::BulletText
			(
				"Documents: .txt, .md, .json, .xml, .yaml, etc."
			);

			ImGui::Text("Supported image formats:");
			ImGui::BulletText("Images: .jpg, .png, .bmp");
		}

		ImGui::End(); // End file editor/viewer window
	}
}

// Function to format file sizes
string FileExplorerApp::FormatSize(double size_in_bytes)
{
	constexpr std::array<const char*, 5> ce_UNITS =
	{
		"B", "KB", "MB", "GB", "TB"
	};

	uint8_t unit_index = 0;
	while (size_in_bytes >= 1024.0f && unit_index < 4)
	{
		size_in_bytes /= 1024.0f;
		unit_index++;
	}

	std::ostringstream out;
	out << std::fixed
		<< std::setprecision(2)
		<< size_in_bytes
		<< " " 
		<< ce_UNITS.at(unit_index);
	return out.str();
}

// Function to get files in a directory
map<string, string> FileExplorerApp::GetFilesInDirectory(const fs::path& path)
{
	map<string, string> files;
	if (fs::exists(path) && fs::is_directory(path))
	{
		for (const auto& ENTRY : fs::directory_iterator(path))
		{
			if (ENTRY.is_regular_file())
			{
				string filename = ENTRY.path().filename().string();
				string size_str = FormatSize(ENTRY.file_size());
				files.emplace(filename, size_str);
			}
			else if (ENTRY.is_directory())
			{
				string dirname = ENTRY.path().filename().string();
				files.emplace(dirname, "[D]");
			}
		}
	}

	return files;
}