// Raylib and ImGui headers for graphics and GUI
#include <raylib.h>
#include <rlImGui.h>
#include <imgui.h>

// File dialog header
#include <imfilebrowser.h>

// Standard C++ headers
#include <filesystem>
#include <map>
#include <fstream>
#include <array>

#define MAX_BUFFER_SIZE (5 * 1024 * 1024) // 5MB buffer

namespace fs = std::filesystem;
using namespace std;

string format_size(uintmax_t size_in_bytes)
{
	const char *units[] = {"B", "KB", "MB", "GB", "TB"};
	size_t unit_index = 0;
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

map<string, string> get_files_in_directory(const fs::path &path)
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

int main()
{
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(900, 500, "File Explorer");
	SetTargetFPS(60);

	rlImGuiSetup(true);

	// Load a font
	ImGuiIO &io = ImGui::GetIO();
	io.Fonts->Clear();
	io.Fonts->AddFontFromFileTTF("assets/Roboto-Regular.ttf", 20.0f);
	rlImGuiReloadFonts();

	// Custom theme
	ImGuiStyle &style = ImGui::GetStyle();
	style.Colors[ImGuiCol_FrameBg] = ImColor(0.1f, 0.1f, 0.1f, 1.0f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImColor(0.2f, 0.2f, 0.2f, 1.0f);
	style.Colors[ImGuiCol_FrameBgActive] = ImColor(0.3f, 0.3f, 0.3f, 1.0f);

	// Initialize File Browser
	ImGui::FileBrowser file_browser(ImGuiFileBrowserFlags_SelectDirectory | ImGuiFileBrowserFlags_EnterNewFilename |
									ImGuiFileBrowserFlags_NoModal | ImGuiFileBrowserFlags_NoStatusBar);

	static fs::path current_path = fs::current_path(); // Start with the current working directory
	static fs::path selected_file = fs::path();		   // To store the selected file path
	static string file_content;						   // Store file content for editing
	static bool file_loaded = false;				   // Track if file is loaded
	static bool file_modified = false;				   // Track if file has been modified

	// Image handling variables
	static Texture2D img_texture = {0};			  // Initialize to empty texture
	static bool img_loaded = false;				  // Track if image is loaded
	static fs::path loaded_img_path = fs::path(); // Track which image is currently loaded

	// UI state variables
	static bool show_save_dialog = false;
	static bool show_error_popup = false;
	static string error_message = "";
	static float side_menu_width = 300.0f; // Make resizable

	array<string, 33> supported_file_types = {
		".txt", ".cpp", ".h", ".hpp", ".c", ".py", ".js", ".html", ".css",
		".json", ".md", ".xml", ".yaml", ".ini", ".log", ".bat", ".sh", ".php",
		".rb", ".go", ".swift", ".ts", ".tsx", ".vue", ".sql", ".pl", ".lua",
		".r", ".dart", ".scala", ".rs", ".java", ".kt"};

	array<string, 3> supported_img_types = {
		".jpg", ".png", ".bmp"};

	while (!WindowShouldClose())
	{
		BeginDrawing();
		ClearBackground(BLACK);

		rlImGuiBegin();

		static bool open = false;
		static bool save = false;

		ImGui::GetStyle().FramePadding.y = 6.0f;

		// Main Menu Bar
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Open Directory"))
				{
					open = true;
					file_browser.Open();
				}
				if (ImGui::MenuItem("Save", "Ctrl+S", false, selected_file != fs::path() && file_loaded))
				{
					save = true;
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Exit"))
				{
					break;
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Edit"))
			{
				if (ImGui::MenuItem("New Folder"))
				{
					// TODO: Implement new folder functionality
				}
				if (ImGui::MenuItem("Rename", nullptr, false, false))
				{
					// TODO: Implement rename functionality
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

		float menu_bar_height = ImGui::GetFrameHeight();

		// Handle keyboard shortcuts
		if (ImGui::IsKeyPressed(ImGuiKey_S) && ImGui::GetIO().KeyCtrl && selected_file != fs::path() && file_loaded)
		{
			save = true;
		}

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

		// Explorer Side Panel (Resizable)
		ImGui::SetNextWindowPos(ImVec2(0, menu_bar_height), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(side_menu_width, static_cast<float>(GetScreenHeight() - menu_bar_height)), ImGuiCond_Always);

		ImGui::Begin("Explorer", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

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
		map<string, string> files = get_files_in_directory(current_path);

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
				string label = "[D] " + dir.first;
				bool is_selected = (selected_file == current_path / dir.first);

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
			}

			if (!regular_files.empty())
				ImGui::Separator();
		}

		// Display files
		if (!regular_files.empty())
		{
			ImGui::TextColored(ImVec4(0.7f, 1.0f, 0.7f, 1.0f), "Files:");
			for (const auto &file : regular_files)
			{
				fs::path file_path = current_path / file.first;
				bool is_selected = (selected_file == file_path);

				string icon = "../assets/Icons/Folder_2.png";
				string ext = fs::path(file.first).extension().string();
				if (find(supported_img_types.begin(), supported_img_types.end(), ext) != supported_img_types.end())
					icon = "[I]";
				else if (find(supported_file_types.begin(), supported_file_types.end(), ext) != supported_file_types.end())
					icon = "[C]";

				string label = icon + " " + file.first + " (" + file.second + ")";

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
			}
		}

		ImGui::Separator();

		if (files.empty())
		{
			ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Directory is empty");
		}

		ImGui::EndChild(); // End Navigation
		ImGui::End();	   // End Explorer window

		// Update side menu width for resizing
		if (ImGui::IsWindowHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
		{
			side_menu_width = ImGui::GetMousePos().x;
			side_menu_width = max(200.0f, min(side_menu_width, GetScreenWidth() * 0.6f));
		}

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

		rlImGuiEnd();
		EndDrawing();
	}

	// Clean up loaded texture before closing
	if (img_loaded && img_texture.id != 0)
	{
		UnloadTexture(img_texture);
	}

	rlImGuiShutdown();
	CloseWindow();
	return 0;
}