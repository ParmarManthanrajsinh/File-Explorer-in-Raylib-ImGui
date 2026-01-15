#include "FileExplorerApp.h"
#include "ImGuiCustomTheme.h"

FileExplorerApp::FileExplorerApp()
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(900, 500, "File Explorer");
    MaximizeWindow();
    Image img = LoadImage("assets/Icons/Logo.png");
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
        ImGuiFileBrowserFlags_SelectDirectory  |
        ImGuiFileBrowserFlags_EnterNewFilename |
        ImGuiFileBrowserFlags_NoModal          |
        ImGuiFileBrowserFlags_NoStatusBar
    );

    current_path = "";              // Start with the current working directory
    m_SelectedFile = fs::path();    // To store the selected file path
    m_bFileLoaded = false;          // Track if file is loaded
    m_bFileModified = false;        // Track if file has been modified
    m_bExit = false;
    m_bShowExitConfirm = false;
	m_bShowSaveBeforeOpenConfirm = false;

    // Image handling variables
    m_ImgTexture = { 0 };           	// Initialize to empty texture
    m_bImgLoaded = false;           	// Track if image is loaded
    m_LoadedImgPath = fs::path();   	// Track which image is currently loaded
	m_PendingFileToOpen = fs::path();   

    // UI state variables
    m_bShowSaveDialog = false;
    m_bShowErrorPopup = false;
    m_ErrorMessage.clear();
    m_SideMenuWidth = 300.0f; // Make resizable

    // Configure the text editor
    m_TextEditor.SetPalette(TextEditor::GetDarkPalette());
    m_TextEditor.SetShowWhitespaces(false);
    m_TextEditor.SetReadOnly(false);
    m_TextEditor.SetHandleKeyboardInputs(true);
    m_TextEditor.SetHandleMouseInputs(true);
    m_TextEditor.SetImGuiChildIgnored(false);

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

    m_SupportedImgTypes = 
    {
        ".jpg", ".png", ".bmp" 
    };
}

FileExplorerApp::~FileExplorerApp()
{
    // Clean up loaded texture before closing
    if (m_bImgLoaded && m_ImgTexture.id != 0)
    {
        UnloadTexture(m_ImgTexture);
    }
    
    // Clean up icons
    UnloadTexture(m_FileIcon);
    UnloadTexture(m_FolderIcon);
    UnloadTexture(m_ImgIcon);
    UnloadTexture(m_EditFileIcon);
    
    rlImGuiShutdown();
    CloseWindow();
}

void FileExplorerApp::Run()
{
    while (!m_bExit)
    {
        // Check for window close button or Escape key
        if (WindowShouldClose() || IsKeyPressed(KEY_ESCAPE))
        {
            if (m_bFileModified)
            {
                m_bShowExitConfirm = true;
            }
            else
            {
                m_bExit = true;
            }
        }
        
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

        HandleExitConfirmPopup();  

		HandleSaveBeforeOpenPopup();

        RenderExplorerPanel(menu_bar_height, sb_Open);

        UpdateSideMenuWidth();

        RenderFileViewer(menu_bar_height);

        rlImGuiEnd();
        EndDrawing();
    }
}

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
                if (m_bFileModified)
                {
                    m_bShowExitConfirm = true;
                }
                else
                {
                    m_bExit = true;
                }
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
            if (m_bFileModified)
            {
                // Show error message for directory changes
                m_ErrorMessage = "You have unsaved changes. Please save before changing directory.";
                m_bShowErrorPopup = true;
                m_FileBrowser.ClearSelected();
                m_FileBrowser.Close();
                return;
            }
            
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
                m_TextEditor.SetText("");
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
        // Get text from the editor
        string content = m_TextEditor.GetText();
        
        ofstream out_file(m_SelectedFile, ios::out | ios::binary);
        if (out_file.is_open())
        {
            out_file.write(content.data(), content.size());
            out_file.close();
            m_bFileModified = false;
            b_Save = false;
        }
        else
        {
            m_ErrorMessage = 
            "Could not save file: " + m_SelectedFile.string();
            
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
        // Red color
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255)); 
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

    if  
    (
        ImGui::BeginPopupModal
        (
            "Create Folder", 
            nullptr, 
            ImGuiWindowFlags_AlwaysAutoResize
        )
    )
    {
        static string s_FolderName{};
        ImGui::InputText("Folder Name", &s_FolderName);
        if (ImGui::Button("Create"))
        {
            fs::path new_folder_path = current_path / s_FolderName;
            if (!fs::exists(new_folder_path))
            {
                fs::create_directory(new_folder_path);
                current_path = new_folder_path; // Change to the new folder
                m_SelectedFile = fs::path();    // Reset selected file
                m_bFileLoaded = false;
                m_bFileModified = false;
                m_TextEditor.SetText("");
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
        static string s_NewFileName{};
        ImGui::InputText
        (
            "File Name", 
            &s_NewFileName
        );

        if (ImGui::Button("Create"))
        {
            fs::path new_file_path = current_path / s_NewFileName;
            if (!fs::exists(new_file_path))
            {
                ofstream FILE(new_file_path);
                if (FILE.is_open())
                {
                    FILE.close();
                    m_SelectedFile = new_file_path;
                    m_bFileLoaded = false;
                    m_bFileModified = false;
                    m_TextEditor.SetText("");
                }
                else
                {
                    m_ErrorMessage = "Could not create file: " 
                    + new_file_path.string();
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
        static string s_NewName{};
        static bool sb_FirstFrame = true;   

        bool b_RenamingSelectedFile = !m_SelectedFile.empty()
            && fs::exists(m_SelectedFile);

        bool b_IsDir = b_RenamingSelectedFile ?
             fs::is_directory(m_SelectedFile) :  
             fs::is_directory(current_path);

        // Initialize the input field with current filename when popup first opens
        if (sb_FirstFrame)
        {
            if (b_RenamingSelectedFile)
            {
                s_NewName = m_SelectedFile.filename().string();
            }
            else
            {
                s_NewName = current_path.filename().string();
            }
            sb_FirstFrame = false;
        }

        ImGui::InputText
        (
            b_IsDir ? "New Folder Name" : "New File Name",
            &s_NewName
        );

        if (ImGui::Button("Rename"))
        {
            if (s_NewName.empty())
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
                        m_TextEditor.SetText("");
                    }
                    catch (const fs::filesystem_error& EX)
                    {
                        m_ErrorMessage = string("Error renaming ")
                        + (b_IsDir ? "folder" : "file")
                        + ": "
                        + EX.what();

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
            s_NewName.clear();
            sb_FirstFrame = true;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            s_NewName.clear();
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

    if 
    (
        ImGui::BeginPopupModal
        (
            "Delete", 
            nullptr, 
            ImGuiWindowFlags_AlwaysAutoResize
        )
    )
    {
        bool b_RenamingSelectedFile = !m_SelectedFile.empty() && 
									  fs::exists(m_SelectedFile);

        bool b_IsDir = b_RenamingSelectedFile ?
             fs::is_directory(m_SelectedFile) : 
             fs::is_directory(current_path);

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
                "Are you sure you want to delete the current directory '%s'?",
                current_path.filename().string().c_str()
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
                m_TextEditor.SetText("");
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

void FileExplorerApp::HandleExitConfirmPopup()
{
    // Exit Confirmation Popup
    if (m_bShowExitConfirm)
    {
        ImGui::OpenPopup("Unsaved Changes");
        m_bShowExitConfirm = false;
    }

    if 
	(
		ImGui::BeginPopupModal
		(
			"Unsaved Changes", 
			nullptr, 
			ImGuiWindowFlags_AlwaysAutoResize
		)
	)
    {
        // Simple message
        ImGui::Text("Do you want to save changes to:");
        ImGui::TextColored
		(
			ImVec4(1.0f, 1.0f, 0.5f, 1.0f), "%s", 
            m_SelectedFile.filename().string().c_str()
		);
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        float button_width = 100.0f;
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        if (ImGui::Button("Save", ImVec2(button_width + 20, 0)))
        {
            // Save the file first
            if (m_SelectedFile != fs::path() && m_bFileLoaded)
            {
                string content = m_TextEditor.GetText();
                ofstream out_file(m_SelectedFile, ios::out | ios::binary);
                if (out_file.is_open())
                {
                    out_file.write(content.data(), content.size());
                    out_file.close();
                    m_bFileModified = false;
                }
            }
            m_bExit = true;
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Don't Save", ImVec2(button_width, 0)))
        {
            m_bExit = true;
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(button_width, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
}

void FileExplorerApp::HandleSaveBeforeOpenPopup()
{
	if (m_bShowSaveBeforeOpenConfirm)
    {
        ImGui::OpenPopup("Save Changes");
        m_bShowSaveBeforeOpenConfirm = false;
    }
	
	if 
	(
		ImGui::BeginPopupModal
		(
			"Save Changes", 
			nullptr, 
			ImGuiWindowFlags_AlwaysAutoResize
		)
	)
	{
		ImGui::Text("Do you want to save changes to:");
		ImGui::TextColored
        (
            ImVec4(1.0f, 1.0f, 0.5f, 1.0f), "%s", 
            m_SelectedFile.filename().string().c_str()
        );
		ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

		float button_width = 100.0f;
        float spacing = ImGui::GetStyle().ItemSpacing.x;
		if (ImGui::Button("Save", ImVec2(button_width + 20, 0)))
		{
			if (m_SelectedFile != fs::path() && m_bFileLoaded)
            {
                string content = m_TextEditor.GetText();
                ofstream out_file(m_SelectedFile, ios::out | ios::binary);
                if (out_file.is_open())
                {
                    out_file.write(content.data(), content.size());
                    out_file.close();
                    m_bFileModified = false;
                }
            }

			FileExplorerApp::OpenFile(m_PendingFileToOpen);
            ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();
		if (ImGui::Button("Don't Save", ImVec2(button_width, 0)))
        {
            // Don't save, just open the new file
            OpenFile(m_PendingFileToOpen);
            ImGui::CloseCurrentPopup();
        }
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(button_width, 0)))
		{
			// Clear the pending file and don't open anything
			m_PendingFileToOpen = fs::path();
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

// Function to render the explorer side panel
void FileExplorerApp::RenderExplorerPanel(float menu_bar_height, bool& b_Open)
{
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
        ImGuiWindowFlags_NoMove     |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize
    );

    if (current_path.empty())
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
    ImGui::PushStyleColor
    (
        ImGuiCol_ChildBg, 
        ImVec4
        (
            0.1f, 0.1f, 0.1f, 1.0f
        )
    );

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
		    if (m_bFileModified)
		    {
		        m_ErrorMessage = "You have unsaved changes. Please save before changing directory.";
		        m_bShowErrorPopup = true;
		    }
		    else
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
		        m_TextEditor.SetText("");
		    }
		}
        ImGui::Separator();
    }

    // Get and display files with better organization
    auto directory_contents = GetFilesInDirectory(current_path);

    enum class e_FileType { DIR, FILE };
    struct FileEntry
    {
        std::string name;
        e_FileType type;
        string size_or_marker;
    };

    // Separate directories and files
    vector<FileEntry> dir_entries;
    vector<FileEntry> file_entries;

    for (const auto& [name, info] : directory_contents)
    {
        if(info == "[D]")
        {
            dir_entries.emplace_back(name, e_FileType::DIR, "[D]");
        }
        else
        {
            file_entries.emplace_back(name, e_FileType::FILE, info);
        }
    }

    // Display directories first
    if (!dir_entries.empty())
    {
        ImGui::TextColored
        (
            ImVec4
            (
                0.7f, 0.7f, 5.0f, 1.0f
            ), 
            "Directories:"
        );
        for (const auto& ENTRY : dir_entries)
        {
            string label = ENTRY.name;
            fs::path dir_path = current_path / ENTRY.name;
            bool b_IsSelected = (m_SelectedFile == dir_path);

            // Start a group to keep icon and text together
            ImGui::BeginGroup();

            // Draw the icon first
            rlImGuiImage(&m_FolderIcon);
            ImGui::SameLine();

            ImVec2 cursor_pos = ImGui::GetCursorPos();
            ImGui::SetCursorPos
            (
                ImVec2
                (
                    cursor_pos.x - 6.0f, 
                    cursor_pos.y + 6.0f
                )
            );

            // Then draw the selectable
			if (ImGui::Selectable(label.c_str(), b_IsSelected))
			{
			    if (m_bFileModified)
			    {
			        m_ErrorMessage = "You have unsaved changes. Please save before changing directory.";
			        m_bShowErrorPopup = true;
			    }
			    else
			    {
			        current_path = dir_path;  // Navigate to the directory
				
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
			        m_TextEditor.SetText("");
			    }
			}

            ImGui::EndGroup();
        }
    }

    // Display files
    if (!file_entries.empty())
    {
        ImGui::TextColored(ImVec4(0.7f, 1.0f, 0.7f, 1.0f), "Files:");
        for (const auto& ENTRY : file_entries)
        {
            fs::path file_path = current_path / ENTRY.name;
            bool b_IsSelected = (m_SelectedFile == file_path);
            Texture2D icon = m_FileIcon;
            string ext = fs::path(ENTRY.name).extension().string();

            if (ranges::contains(m_SupportedImgTypes, ext))
            {
                icon = m_ImgIcon;
            }
            else if (ranges::contains(m_SupportedFileTypes, ext))
            {
                icon = m_EditFileIcon;
            }

            string label = ENTRY.name + " (" + ENTRY.size_or_marker + ")";

            // Start a group to keep icon and text together
            ImGui::BeginGroup();

            // Draw the icon first
            rlImGuiImage(&icon);
            ImGui::SameLine();

            ImVec2 cursor_pos = ImGui::GetCursorPos();
            ImGui::SetCursorPos
            (
                ImVec2
                (
                    cursor_pos.x - 6.0f, 
                    cursor_pos.y + 6.0f
                )
            );

            // Then draw the selectable
			if (ImGui::Selectable(label.c_str(), b_IsSelected))
			{
			    if (m_bFileModified)
			    {
			        // Store the file the user wants to open and show confirmation
			        m_PendingFileToOpen = file_path;
			        m_bShowSaveBeforeOpenConfirm = true;
			    }
			    else
			    {
			        // Only process if it's a different file
			        if (m_SelectedFile != file_path)
			        {
			            OpenFile(file_path);
			        }
			    }
			}

            ImGui::EndGroup();
        }
    }

    ImGui::Separator();

    if (directory_contents.empty())
    {
        ImGui::TextColored
        (
            ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
            "Directory is empty"
        );
    }

    ImGui::EndChild(); 
    ImGui::End();      
}

// Function to update side menu width for resizing
void FileExplorerApp::UpdateSideMenuWidth()
{
    if 
    (
        ImGui::IsWindowHovered() && 
        ImGui::IsMouseDragging
        (
            ImGuiMouseButton_Left
        )
    )
    {
        m_SideMenuWidth = ImGui::GetMousePos().x;
        m_SideMenuWidth = 
        max
        (
            200.0f, 
            min
            (
                m_SideMenuWidth, 
                GetScreenWidth() * 0.6f
            )
        );
    }
}

// Function to render the file viewer/editor with syntax highlighting
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
            ImGuiWindowFlags_NoMove     | 
            ImGuiWindowFlags_NoResize
        );

        // File info header
        ImGui::Text
        (
            "File: %s", 
            m_SelectedFile.filename().string().c_str()
        );
        ImGui::Text
        (
            "Path: %s", 
            m_SelectedFile.parent_path().string().c_str()
        );
        try
        {
            ImGui::Text
            (
                "Size: %s", 
                FormatSize(fs::file_size(m_SelectedFile)).c_str()
            );
        }
        catch (...)
        {
            ImGui::Text("Size: Unknown");
        }
        
        // Show if file has been modified
        if (m_bFileModified)
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "(Modified)");
        }
        
        ImGui::Separator();
        string file_ext = m_SelectedFile.extension().string();

        // Upper to Lower case conversion ( Ascii for maximum speed )
        ranges::for_each(file_ext, [](char &c) { if (c >= 'A' && c <= 'Z') c += 32; });

        // Handle text files with syntax highlighting
        if (ranges::contains(m_SupportedFileTypes, file_ext))
        {
            if (!m_bFileLoaded)
            {
                ifstream FILE(m_SelectedFile, ios::in | ios::binary);
                if (FILE.is_open())
                {
                    FILE.seekg(0, ios::end);
                    size_t fileSize = FILE.tellg();
                    FILE.seekg(0, ios::beg);

                    if (fileSize > ce_MAX_BUFFER_SIZE)
                    {
                        m_ErrorMessage = "File too large! Maximum size: " 
                        + to_string(ce_MAX_BUFFER_SIZE / (1024 * 1024)) 
                        + " MB";

                        m_bShowErrorPopup = true;
                        FILE.close();
                        return;
                    }
                    else
                    {
                        // Read file content
                        vector<char> buffer(fileSize + 1);
                        FILE.read(buffer.data(), fileSize);
                        buffer[fileSize] = '\0';
                        
                        // Set text in the editor
                        m_TextEditor.SetText(string(buffer.data()));
                        SetEditorLanguage(m_SelectedFile);
                        
                        m_bFileLoaded = true;
                        m_bFileModified = false;
                    }
                    FILE.close();
                }
                else
                {
                    m_ErrorMessage = "Could not open file: "
                                   + m_SelectedFile.string();
                    m_bShowErrorPopup = true;
                }
            }

            if (m_bFileLoaded)
            {
                // Render the text editor with syntax highlighting
                // Get available space
                ImVec2 availableSize = ImGui::GetContentRegionAvail();
                
                // Render the TextEditor
                m_TextEditor.Render("##TextEditor", availableSize);
                
                // Check if text was modified
                if (m_TextEditor.IsTextChanged())
                {
                    m_bFileModified = true;
                }
            }
        }
		
        // Handle image files (unchanged)
        else if (ranges::contains(m_SupportedImgTypes, file_ext))
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

                    // Free the image data, keep only the texture
                    UnloadImage(img); 
                    m_bImgLoaded = true;
                    m_LoadedImgPath = m_SelectedFile;
                }
                else
                {
                    m_ErrorMessage = "Failed to load image: " 
                                   + m_SelectedFile.filename().string();
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
                if (scale > 2.0f) scale = 2.0f;
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
                    &m_ImgTexture, 
                    static_cast<int>(display_width), 
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
    constexpr std::array<string_view, 5> ce_UNITS =
    {
        "B", "KB", "MB", "GB", "TB"
    };

    std::size_t unit_idx = 0;
    while (size_in_bytes >= 1024.0 && (unit_idx + 1) < ce_UNITS.size())
    {
        size_in_bytes /= 1024.0;
        ++unit_idx;
    }

    return std::format("{:.2f} {}", size_in_bytes, ce_UNITS[unit_idx]);
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

// Helper function to determine language from file extension
const TextEditor::LanguageDefinition& FileExplorerApp::GetLanguageDefinition
(
    const string& extension
)
{
    static const TextEditor::LanguageDefinition& default_lang = TextEditor::LanguageDefinition::CPlusPlus();
    
    if 
    (
        extension == ".cpp" || 
        extension == ".h"   || 
        extension == ".hpp" || 
        extension == ".cxx" || 
        extension == ".hxx"
    )
    {
        return TextEditor::LanguageDefinition::CPlusPlus();
    }
    else if (extension == ".c")
    {
        return TextEditor::LanguageDefinition::C();
    }
    else if (extension == ".hlsl" || extension == ".fx")
    {
        return TextEditor::LanguageDefinition::HLSL();
    }
    else if 
    (
        extension == ".glsl" || 
        extension == ".vert" || 
        extension == ".frag" || 
        extension == ".geom"
    )
    {
        return TextEditor::LanguageDefinition::GLSL();
    }
    else if (extension == ".sql")
    {
        return TextEditor::LanguageDefinition::SQL();
    }
    else if (extension == ".as")
    {
        return TextEditor::LanguageDefinition::AngelScript();
    }
    else if (extension == ".lua")
    {
        return TextEditor::LanguageDefinition::Lua();
    }
    else if (extension == ".py")
    {
        // Python-like highlighting (we'll use C++ as base for now)
        static TextEditor::LanguageDefinition python_lang = TextEditor::LanguageDefinition::CPlusPlus();
        python_lang.mName = "Python";
        python_lang.mCommentStart = "'''";
        python_lang.mCommentEnd = "'''";
        python_lang.mSingleLineComment = "#";
        return python_lang;
    }
    else if (extension == ".js" || extension == ".ts" || extension == ".tsx")
    {
        // JavaScript/TypeScript (use C++ as base)
        static TextEditor::LanguageDefinition js_lang = TextEditor::LanguageDefinition::CPlusPlus();
        js_lang.mName = "JavaScript";
        return js_lang;
    }
    else if (extension == ".html" || extension == ".htm")
    {
        // HTML (use C++ as base)
        static TextEditor::LanguageDefinition html_lang = TextEditor::LanguageDefinition::CPlusPlus();
        html_lang.mName = "HTML";
        html_lang.mSingleLineComment = "<!--";
        return html_lang;
    }
    else if (extension == ".css")
    {
        // CSS (use C++ as base)
        static TextEditor::LanguageDefinition css_lang = TextEditor::LanguageDefinition::CPlusPlus();
        css_lang.mName = "CSS";
        css_lang.mSingleLineComment = "/*";
        return css_lang;
    }
    else if (extension == ".java")
    {
        static TextEditor::LanguageDefinition java_lang = TextEditor::LanguageDefinition::CPlusPlus();
        java_lang.mName = "Java";
        return java_lang;
    }
    else if (extension == ".rs")
    {
        static TextEditor::LanguageDefinition rust_lang = TextEditor::LanguageDefinition::CPlusPlus();
        rust_lang.mName = "Rust";
        rust_lang.mSingleLineComment = "//";
        return rust_lang;
    }
    else if (extension == ".go")
    {
        static TextEditor::LanguageDefinition go_lang = TextEditor::LanguageDefinition::CPlusPlus();
        go_lang.mName = "Go";
        go_lang.mSingleLineComment = "//";
        return go_lang;
    }
    
    return default_lang;
}

void FileExplorerApp::OpenFile(const fs::path &file_path)
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
    m_TextEditor.SetText("");
    
    // Clear the pending file
    m_PendingFileToOpen = fs::path();
}

// Helper function to set editor language based on file extension
void FileExplorerApp::SetEditorLanguage(const fs::path& filePath)
{
    string ext = filePath.extension().string();
    
    // Convert to lowercase for comparison ( Used direct ascii for ultimate speed )
    std::ranges::for_each(ext, [](char& c) { if (c >= 'A' && c <= 'Z') c += 32; });

    m_TextEditor.SetLanguageDefinition(GetLanguageDefinition(ext));
}