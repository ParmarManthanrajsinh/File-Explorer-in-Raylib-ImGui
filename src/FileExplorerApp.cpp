#include "FileExplorerApp.hpp"
#include "ImGuiCustomTheme.h"

FileExplorerApp::FileExplorerApp()
{
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(900, 500, "File Explorer");
	SetTargetFPS(60);

	rlImGuiSetup(true);
	ImCustomTheme();

	// Load Icon
	file_icon = LoadTexture("assets/icons/file.png");
	folder_icon = LoadTexture("assets/icons/folder.png");
	img_icon = LoadTexture("assets/icons/image.png");
	edit_file_icon = LoadTexture("assets/icons/edit_file.png");

	// Initialize File Browser
	file_browser = ImGui::FileBrowser(ImGuiFileBrowserFlags_SelectDirectory | ImGuiFileBrowserFlags_EnterNewFilename |
									  ImGuiFileBrowserFlags_NoModal | ImGuiFileBrowserFlags_NoStatusBar);

	current_path = ""; // Start with the current working directory
	selected_file = fs::path();		   // To store the selected file path
	file_content.clear();			   // Store file content for editing
	file_loaded = false;			   // Track if file is loaded
	file_modified = false;			   // Track if file has been modified
	exit = false;

	// Image handling variables
	img_texture = {0};			  // Initialize to empty texture
	img_loaded = false;			  // Track if image is loaded
	loaded_img_path = fs::path(); // Track which image is currently loaded

	// UI state variables
	show_save_dialog = false;
	show_error_popup = false;
	error_message.clear();
	side_menu_width = 300.0f; // Make resizable

	supported_file_types = {
		".txt", ".cpp", ".h", ".hpp", ".c", ".py", ".js", ".html", ".css",
		".json", ".md", ".xml", ".yaml", ".ini", ".log", ".bat", ".sh", ".php",
		".rb", ".go", ".swift", ".ts", ".tsx", ".vue", ".sql", ".pl", ".lua",
		".r", ".dart", ".scala", ".rs", ".java", ".kt"};

	supported_img_types = {
		".jpg", ".png", ".bmp"};
}

FileExplorerApp::~FileExplorerApp()
{
	// Clean up loaded texture before closing
	if (img_loaded && img_texture.id != 0)
	{
		UnloadTexture(img_texture);
	}
	rlImGuiShutdown();
	CloseWindow();
}

void FileExplorerApp::Run()
{
	while (!WindowShouldClose() && !exit)
	{
		BeginDrawing();
		ClearBackground(BLACK);

		rlImGuiBegin();

		static bool open = false;
		static bool save = false;
		static bool create_new_folder = false;
		static bool create_new_file = false;
		static bool rename_file = false;
		static bool _delete = false;

		ImGui::GetStyle().FramePadding.y = 6.0f;

		RenderMainMenuBar(open, save, create_new_folder, create_new_file, rename_file, _delete);

		float menu_bar_height = ImGui::GetFrameHeight();

		ApplyShortcuts(open, save, create_new_folder, create_new_file, rename_file);

		ProcessFileBrowserDialog(open);

		ProcessSaveFile(save);

		HandleErrorPopup();

		HandleCreateFolderPopup(create_new_folder);

		HandleCreateFilePopup(create_new_file);

		HandleRenamePopup(rename_file);

		HandleDeletePopup(_delete);

		RenderExplorerPanel(menu_bar_height, open);

		UpdateSideMenuWidth();

		RenderFileViewer(menu_bar_height);

		rlImGuiEnd();
		EndDrawing();
	}
}

// Function to render the main menu bar
void FileExplorerApp::RenderMainMenuBar(bool &open, bool &save, bool &create_new_folder, bool &create_new_file, bool &rename_file, bool &_delete)
{
	// Main Menu Bar
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Open Directory", "Ctrl+O", false))
			{
				open = true;
				file_browser.Open();
			}
			if (ImGui::MenuItem("Save", "Ctrl+S", false, selected_file != fs::path() && file_loaded))
			{
				save = true;
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Exit", "Escape"))
			{
				exit = true;
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Edit"))
		{
			if (ImGui::MenuItem("New Folder", "Ctrl+Shift+N"))
			{
				create_new_folder = true;
			}
			if (ImGui::MenuItem("New File", "Ctrl+N"))
			{
				create_new_file = true;
			}
			if (ImGui::MenuItem("Rename", "F2", false))
			{
				rename_file = true;
			}
			if (ImGui::MenuItem("Delete"))
			{
				_delete = true;
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Help"))
		{
			if (ImGui::MenuItem("About"))
			{
				error_message = "Simple File Explorer\nBuilt with Raylib and ImGui";
				show_error_popup = true;
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
}

// Function to apply keyboard shortcuts
void FileExplorerApp::ApplyShortcuts(bool &open, bool &save, bool &create_new_folder, bool &create_new_file, bool &rename_file)
{
	// Handle keyboard shortcuts
	if (ImGui::IsKeyPressed(ImGuiKey_O) && ImGui::GetIO().KeyCtrl)
	{
		open = true;
		file_browser.Open();
	}
	if (ImGui::IsKeyPressed(ImGuiKey_S) && ImGui::GetIO().KeyCtrl && selected_file != fs::path() && file_loaded)
	{
		save = true;
	}
	if (ImGui::IsKeyPressed(ImGuiKey_N) && ImGui::GetIO().KeyCtrl && !ImGui::GetIO().KeyShift)
	{
		create_new_file = true;
	}
	if (ImGui::IsKeyPressed(ImGuiKey_N) && ImGui::GetIO().KeyCtrl && ImGui::GetIO().KeyShift)
	{
		create_new_folder = true;
	}
	if (ImGui::IsKeyPressed(ImGuiKey_F2))
	{
		rename_file = true;
	}
}

// Function to process the file browser dialog
void FileExplorerApp::ProcessFileBrowserDialog(bool &open)
{
	// File Browser Dialog
	if (open)
	{
		file_browser.Display();

		if (file_browser.HasSelected())
		{
			open = false;
			fs::path new_path = file_browser.GetDirectory();
			if (fs::exists(new_path) && fs::is_directory(new_path))
			{
				current_path = new_path;

				// Clean up any loaded resources when changing directory
				if (img_loaded && img_texture.id != 0)
				{
					UnloadTexture(img_texture);
					img_loaded = false;
					loaded_img_path = fs::path();
				}
				selected_file = fs::path();
				file_loaded = false;
				file_modified = false;
				file_content.clear();
			}
			file_browser.ClearSelected();
			file_browser.Close();
		}
	}
}

// Function to process saving a file
void FileExplorerApp::ProcessSaveFile(bool &save)
{
	// Save File
	if (save && selected_file != fs::path() && file_loaded)
	{
		ofstream out_file(selected_file, ios::out | ios::binary);
		if (out_file.is_open())
		{
			out_file.write(file_content.data(), file_content.size());
			out_file.close();
			file_modified = false;
			save = false;
		}
		else
		{
			error_message = "Could not save file: " + selected_file.string();
			show_error_popup = true;
			save = false;
		}
	}
}

// Function to handle error popups
void FileExplorerApp::HandleErrorPopup()
{
	// Error Popup
	if (show_error_popup)
	{
		ImGui::OpenPopup("Error");
		show_error_popup = false;
		selected_file = fs::path();
	}

	if (ImGui::BeginPopupModal("Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextWrapped("%s", error_message.c_str());
		ImGui::Separator();
		if (ImGui::Button("OK", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

// Function to handle the "Create Folder" popup
void FileExplorerApp::HandleCreateFolderPopup(bool &create_new_folder)
{
	// New Folder Popup
	if (create_new_folder)
	{
		ImGui::OpenPopup("Create Folder");
		create_new_folder = false;
	}

	if (ImGui::BeginPopupModal("Create Folder", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		static char folder_name[128] = "";
		ImGui::InputText("Folder Name", folder_name, sizeof(folder_name));
		if (ImGui::Button("Create"))
		{
			fs::path new_folder_path = current_path / folder_name;
			if (!fs::exists(new_folder_path))
			{
				fs::create_directory(new_folder_path);
				current_path = new_folder_path; // Change to the new folder
				selected_file = fs::path();		// Reset selected file
				file_loaded = false;
				file_modified = false;
				file_content.clear();
			}
			else
			{
				error_message = "Folder already exists!";
				show_error_popup = true;
			}
			folder_name[0] = '\0';
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
void FileExplorerApp::HandleCreateFilePopup(bool &create_new_file)
{
	// New File Popup
	if (create_new_file)
	{
		ImGui::OpenPopup("Create File");
		create_new_file = false;
	}

	if (ImGui::BeginPopupModal("Create File", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		static char new_file_name[128] = "";
		ImGui::InputText("File Name", new_file_name, sizeof(new_file_name));

		if (ImGui::Button("Create"))
		{
			fs::path new_file_path = current_path / new_file_name;
			if (!fs::exists(new_file_path))
			{
				ofstream file(new_file_path);
				if (file.is_open())
				{
					file.close();
					selected_file = new_file_path;
					file_loaded = false;
					file_modified = false;
					file_content.clear();
				}
				else
				{
					error_message = "Could not create file: " + new_file_path.string();
					show_error_popup = true;
				}
			}
			else
			{
				error_message = "File already exists!";
				show_error_popup = true;
			}
			new_file_name[0] = '\0';
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
void FileExplorerApp::HandleRenamePopup(bool &rename_file)
{
	// Rename File Popup
	if (rename_file)
	{
		ImGui::OpenPopup("Rename");
		rename_file = false;
	}

	if (ImGui::BeginPopupModal("Rename", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		static char new_name[128] = "";
		static bool first_frame = true;

		bool renaming_selected_file = !selected_file.empty() && fs::exists(selected_file);
		bool is_dir = renaming_selected_file ? fs::is_directory(selected_file) : fs::is_directory(current_path);

		// Initialize the input field with current filename when popup first opens
		if (first_frame)
		{
			if (renaming_selected_file)
			{
				// Renaming the selected file/folder
				strncpy(new_name, selected_file.filename().string().c_str(), sizeof(new_name) - 1);
			}
			else
			{
				// Renaming the current directory
				strncpy(new_name, current_path.filename().string().c_str(), sizeof(new_name) - 1);
			}
			new_name[sizeof(new_name) - 1] = '\0';
			first_frame = false;
		}

		ImGui::InputText(is_dir ? "New Folder Name" : "New File Name", new_name, sizeof(new_name));

		if (ImGui::Button("Rename"))
		{
			// Validate input
			if (strlen(new_name) == 0)
			{
				error_message = "Name cannot be empty!";
				show_error_popup = true;
			}
			else
			{
				fs::path source_path, new_path;

				if (renaming_selected_file)
				{
					source_path = selected_file;
					new_path = selected_file.parent_path() / new_name;
				}
				else
				{
					source_path = current_path;
					new_path = current_path.parent_path() / new_name;
				}

				if (!fs::exists(new_path))
				{
					try
					{
						fs::rename(source_path, new_path);

						// Update paths after successful rename
						if (renaming_selected_file)
						{
							selected_file = new_path;
						}
						else
						{
							current_path = new_path;
							selected_file = fs::path(); // Reset selected file
						}

						// Reset file state
						file_loaded = false;
						file_modified = false;
						file_content.clear();
					}
					catch (const fs::filesystem_error &ex)
					{
						error_message = string("Error renaming ") + (is_dir ? "folder" : "file") + ": " + ex.what();
						show_error_popup = true;
					}
				}
				else
				{
					error_message = string(is_dir ? "Folder" : "File") + " already exists!";
					show_error_popup = true;
				}
			}

			// Reset for next use and close popup
			memset(new_name, 0, sizeof(new_name));
			first_frame = true; // FIXED: Reset first_frame for next popup opening
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
		{
			// Reset for next use and close popup
			memset(new_name, 0, sizeof(new_name));
			first_frame = true; // FIXED: Reset first_frame for next popup opening
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

// Function to handle the "Delete" popup
void FileExplorerApp::HandleDeletePopup(bool &_delete)
{
	// Delete File and Folder Popup
	if (_delete)
	{
		ImGui::OpenPopup("Delete");
		_delete = false;
	}

	if (ImGui::BeginPopupModal("Delete", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		bool renaming_selected_file = !selected_file.empty() && fs::exists(selected_file);
		bool is_dir = renaming_selected_file ? fs::is_directory(selected_file) : fs::is_directory(current_path);

		if (renaming_selected_file)
		{
			ImGui::Text("Are you sure you want to delete '%s'?", selected_file.filename().string().c_str());
		}
		else
		{
			ImGui::Text("Are you sure you want to delete the current directory '%s'?", current_path.filename().string().c_str());
		}

		// Show confirmation buttons
		if (ImGui::Button("Delete", ImVec2(120, 0)))
		{
			try
			{
				if (renaming_selected_file)
				{
					fs::remove_all(selected_file);
					selected_file = fs::path();
				}
				else
				{
					fs::remove_all(current_path);
					current_path = fs::current_path();
					selected_file = fs::path();
				}
				file_loaded = false;
				file_modified = false;
				file_content.clear();
			}
			catch (const fs::filesystem_error &ex)
			{
				error_message = "Error deleting file/folder: " + string(ex.what());
				show_error_popup = true;
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
void FileExplorerApp::RenderExplorerPanel(float menu_bar_height, bool &open)
{
	// Explorer Side Panel (Resizable)
	ImGui::SetNextWindowPos(ImVec2(0, menu_bar_height), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(side_menu_width, static_cast<float>(GetScreenHeight() - menu_bar_height)), ImGuiCond_Always);

	ImGui::Begin("Explorer", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

	if (current_path == "")
	{
		ImGui::Text("No folder opened\n");
		if (ImGui::Button("Open Folder", ImVec2(-1, 0)))
		{
			open = true;
			file_browser.Open();
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
			if (img_loaded && img_texture.id != 0)
			{
				UnloadTexture(img_texture);
				img_loaded = false;
				loaded_img_path = fs::path();
			}
			selected_file = fs::path();
			file_loaded = false;
			file_modified = false;
			file_content.clear();
		}
		ImGui::Separator();
	}

	// Get and display files with better organization
	auto files = get_files_in_directory(current_path);

	// Separate directories and files
	vector<pair<string, string>> directories;
	vector<pair<string, string>> regular_files;

	for (const auto &file : files)
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
		for (const auto &dir : directories)
		{
			string label = dir.first;
			bool is_selected = (selected_file == current_path / dir.first);

			// Start a group to keep icon and text together
			ImGui::BeginGroup();

			// Draw the icon first
			rlImGuiImage(&folder_icon);
			ImGui::SameLine();

			ImVec2 cursor_pos = ImGui::GetCursorPos();
			ImGui::SetCursorPos(ImVec2(cursor_pos.x - 6.0f, cursor_pos.y + 6.0f));

			// Then draw the selectable
			if (ImGui::Selectable(label.c_str(), is_selected))
			{
				current_path /= dir.first;
				// Clean up any loaded resources
				if (img_loaded && img_texture.id != 0)
				{
					UnloadTexture(img_texture);
					img_loaded = false;
					loaded_img_path = fs::path();
				}
				selected_file = fs::path();
				file_loaded = false;
				file_modified = false;
				file_content.clear();
			}

			ImGui::EndGroup();
		}
	}

	// Display files
	if (!regular_files.empty())
	{
		ImGui::TextColored(ImVec4(0.7f, 1.0f, 0.7f, 1.0f), "Files:");
		for (const auto &file : regular_files)
		{
			fs::path file_path = current_path / file.first;
			bool is_selected = (selected_file == file_path);
			Texture2D icon = file_icon;
			string ext = fs::path(file.first).extension().string();
			if (find(supported_img_types.begin(), supported_img_types.end(), ext) != supported_img_types.end())
				icon = img_icon;
			else if (find(supported_file_types.begin(), supported_file_types.end(), ext) != supported_file_types.end())
				icon = edit_file_icon;
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
			if (ImGui::Selectable(label.c_str(), is_selected))
			{
				// Only process if it's a different file
				if (selected_file != file_path)
				{
					// Clean up previous resources
					if (img_loaded && img_texture.id != 0)
					{
						UnloadTexture(img_texture);
						img_loaded = false;
						loaded_img_path = fs::path();
					}
					selected_file = file_path;
					file_loaded = false;
					file_modified = false;
					file_content.clear();
				}
			}

			ImGui::EndGroup();
		}
	}

	ImGui::Separator();

	if (files.empty())
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Directory is empty");

	ImGui::EndChild(); // End Navigation
	ImGui::End();	   // End Explorer window
}

// Function to update side menu width for resizing
void FileExplorerApp::UpdateSideMenuWidth()
{
	if (ImGui::IsWindowHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
	{
		side_menu_width = ImGui::GetMousePos().x;
		side_menu_width = max(200.0f, min(side_menu_width, GetScreenWidth() * 0.6f));
	}
}

// Function to render the file viewer/editor
void FileExplorerApp::RenderFileViewer(float menu_bar_height)
{
	// File Editor/Viewer Window
	if (selected_file != fs::path())
	{
		string file_name = selected_file.filename().string();
		string window_title = file_name;
		if (file_modified)
			window_title += " *";

		ImGui::SetNextWindowPos(ImVec2(side_menu_width + 5, menu_bar_height), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(static_cast<float>(GetScreenWidth() - side_menu_width - 5),
										static_cast<float>(GetScreenHeight() - menu_bar_height)),
								 ImGuiCond_Always);
		ImGui::Begin(window_title.c_str(), nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

		// File info header
		ImGui::Text("File: %s", selected_file.filename().string().c_str());
		ImGui::Text("Path: %s", selected_file.parent_path().string().c_str());
		try
		{
			ImGui::Text("Size: %s", format_size(fs::file_size(selected_file)).c_str());
		}
		catch (...)
		{
			ImGui::Text("Size: Unknown");
		}
		ImGui::Separator();

		string file_ext = selected_file.extension().string();

		// Handle text files
		if (find(supported_file_types.begin(), supported_file_types.end(), file_ext) != supported_file_types.end())
		{
			if (!file_loaded)
			{
				ifstream file(selected_file, ios::in | ios::binary);
				if (file.is_open())
				{
					file.seekg(0, ios::end);
					size_t fileSize = file.tellg();
					file.seekg(0, ios::beg);

					if (fileSize > MAX_BUFFER_SIZE)
					{
						error_message = "File too large! Maximum size: " + to_string(MAX_BUFFER_SIZE / (1024 * 1024)) + " MB";
						show_error_popup = true;
					}
					else
					{
						file_content.resize(fileSize + 1024); // Extra buffer for editing
						file.read(&file_content[0], fileSize);
						file_content[fileSize] = '\0'; // Null terminate
						file_content.resize(fileSize);
						file_loaded = true;
						file_modified = false;
					}
					file.close();
				}
				else
				{
					error_message = "Could not open file: " + selected_file.string();
				}
			}

			if (file_loaded)
			{
				// Create a larger buffer for editing
				static string edit_buffer;
				if (edit_buffer.size() != file_content.size() + 1024)
				{
					edit_buffer = file_content;
					edit_buffer.resize(file_content.size() + 1024);
				}

				if (ImGui::InputTextMultiline("##editor", &edit_buffer[0], edit_buffer.capacity(),
											  ImVec2(-1, -1), ImGuiInputTextFlags_AllowTabInput))
				{
					file_content = edit_buffer.c_str(); // Update content
					file_modified = true;
				}
			}
		}
		// Handle image files
		else if (find(supported_img_types.begin(), supported_img_types.end(), file_ext) != supported_img_types.end())
		{
			// Only load texture if it's a different file or not loaded yet
			if (!img_loaded || loaded_img_path != selected_file)
			{
				// Unload previous texture if one was loaded
				if (img_loaded && img_texture.id != 0)
				{
					UnloadTexture(img_texture);
				}

				// Try to load the new image
				Image img = LoadImage(selected_file.string().c_str());
				if (img.data != nullptr)
				{
					img_texture = LoadTextureFromImage(img);
					UnloadImage(img); // Free the image data, keep only the texture
					img_loaded = true;
					loaded_img_path = selected_file;
				}
				else
				{
					error_message = "Failed to load image: " + selected_file.filename().string();
					show_error_popup = true;
					img_loaded = false;
				}
			}

			// Display the image if loaded successfully
			if (img_loaded && img_texture.id != 0)
			{
				// Calculate display size while maintaining aspect ratio
				float img_width = static_cast<float>(img_texture.width);
				float img_height = static_cast<float>(img_texture.height);
				float available_width = ImGui::GetContentRegionAvail().x - 20;	// Leave some margin
				float available_height = ImGui::GetContentRegionAvail().y - 20; // Leave some margin

				float scale_x = available_width / img_width;
				float scale_y = available_height / img_height;
				float scale = min(scale_x, scale_y);

				// Don't scale up small images too much
				if (scale > 2.0f)
					scale = 2.0f;

				float display_width = img_width * scale;
				float display_height = img_height * scale;

				ImGui::Text("Dimensions: %dx%d pixels", img_texture.width, img_texture.height);
				ImGui::Text("Display Scale: %.2f", scale);
				ImGui::Separator();

				// Center the image horizontally
				float cursor_x = (available_width - display_width) * 0.5f;
				if (cursor_x > 0)
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + cursor_x);

				// Use scrollable child window for large images
				ImGui::BeginChild("ImageView", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
				rlImGuiImageSize(&img_texture, static_cast<int>(display_width), static_cast<int>(display_height));
				ImGui::EndChild();
			}
		}
		else
		{
			ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "File format not supported for preview");
			ImGui::Text("Extension: %s", file_ext.c_str());

			ImGui::Separator();
			ImGui::Text("Supported text formats:");
			ImGui::BulletText("Code files: .cpp, .h, .py, .js, .html, .css, etc.");
			ImGui::BulletText("Documents: .txt, .md, .json, .xml, .yaml, etc.");

			ImGui::Text("Supported image formats:");
			ImGui::BulletText("Images: .jpg, .png, .bmp");
		}

		ImGui::End(); // End file editor/viewer window
	}
}

// Function to format file sizes
string FileExplorerApp::format_size(uintmax_t size_in_bytes)
{
	const char *units[] = {"B", "KB", "MB", "GB", "TB"};
	uint8_t unit_index = 0;
	double size = static_cast<double>(size_in_bytes);

	while (size >= 1024 && unit_index < 4)
	{
		size /= 1024;
		unit_index++;
	}

	ostringstream out;
	out << fixed << setprecision(2) << size << ' ' << units[unit_index];
	return out.str();
}

// Function to get files in a directory
map<string, string> FileExplorerApp::get_files_in_directory(const fs::path &path)
{
	map<string, string> files;
	if (fs::exists(path) && fs::is_directory(path))
	{
		for (const auto &entry : fs::directory_iterator(path))
		{
			if (entry.is_regular_file())
			{
				string filename = entry.path().filename().string();
				string size_str = format_size(entry.file_size());
				files.emplace(filename, size_str);
			}
			else if (entry.is_directory())
			{
				string dirname = entry.path().filename().string();
				files.emplace(dirname, "[D]");
			}
		}
	}
	return files;
}