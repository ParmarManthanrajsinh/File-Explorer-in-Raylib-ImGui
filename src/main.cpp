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

    // Initialize File Browser
    ImGui::FileBrowser file_browser(ImGuiFileBrowserFlags_SelectDirectory | ImGuiFileBrowserFlags_EnterNewFilename |
                                    ImGuiFileBrowserFlags_NoModal | ImGuiFileBrowserFlags_NoTitleBar |
                                    ImGuiFileBrowserFlags_NoStatusBar | ImGuiFileBrowserFlags_CloseOnEsc);

    static fs::path current_path = fs::current_path(); // Start with the current working directory
    static fs::path selected_file = fs::path();        // To store the selected file path
    static string file_content;                        // Store file content for editing
    static bool file_loaded = false;                   // Track if file is loaded

    array<string, 33> supported_file_types = {
        ".txt", ".cpp", ".h", ".hpp", ".c", ".py", ".js", ".html", ".css",
        ".json", ".md", ".xml", ".yaml", ".ini", ".log", ".bat", ".sh", ".php",
        ".rb", ".go", ".swift", ".ts", ".tsx", ".vue", ".sql", ".pl", ".lua",
        ".r", ".dart", ".scala", ".rs", ".java", ".kt"};

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BLACK);

        rlImGuiBegin();

        static bool open = false;
        static bool save = false;

        ImGui::GetStyle().FramePadding.y = 6.0f;

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Open"))
                {
                    open = !open;
                }
                if (ImGui::MenuItem("Save") && selected_file != fs::path())
                {
                    save = !save;
                }
                if (ImGui::MenuItem("Exit"))
                {
                    break;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem("new"))
                {
                    string new_dir_name = "New Folder";
                    fs::path new_dir_path = current_path / new_dir_name;
                    size_t counter = 1;
                    while (fs::exists(new_dir_path))
                    {
                        new_dir_name = "New Folder (" + to_string(counter++) + ")";
                        new_dir_path = current_path / new_dir_name;
                    }
                    fs::create_directory(new_dir_path);
                }
                if (ImGui::MenuItem("rename"))
                {
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Help"))
            {
                if (ImGui::MenuItem("About"))
                {
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        if (open)
        {
            file_browser.Open();
            file_browser.Display();

            if (file_browser.HasSelected())
            {
                open = false;                              
                file_browser.ClearSelected();               // Clear selection for next use
                current_path = file_browser.GetDirectory(); // Update current path to selected directory
                file_browser.Close();
            }
        }

        if (save && selected_file != fs::path())
        {
            ofstream out_file(selected_file, ios::out | ios::binary);
            if (out_file.is_open())
            {
                out_file.write(file_content.data(), file_content.size());
                out_file.close();
                save = false; // Reset save flag after saving
            }
            else
            {
                ImGui::TextColored({1, 0, 0, 1}, // RGBA Red color
                                   "Could not save file: %s", selected_file.string().c_str());
            }
        }

        // Create a side menu window with fixed position and full height
        ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetFrameHeight()), ImGuiCond_Always);
        ImGui::SetNextWindowSizeConstraints(
            ImVec2(200, static_cast<float>(GetScreenHeight())),
            ImVec2(500, static_cast<float>(GetScreenHeight())));
        ImGui::Begin("Explorer", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);

        // Display current Folder Name
        ImGui::TextWrapped("%s", current_path.filename().string().c_str());

        // Add "Back" button
        if (ImGui::Selectable("..") && current_path.has_parent_path())
        {
            current_path = current_path.parent_path();
        }

        // Get and display files
        map<string, string> files = get_files_in_directory(current_path);
        for (const auto &file : files)
        {
            string label = "[F] " + file.first + "  " + file.second;

            if (file.second == "[D]")
            {
                label = "[D] " + file.first;
            }

            if (ImGui::Selectable(label.c_str()))
            {
                if (file.second == "[D]")
                {
                    current_path /= file.first;
                }
                else
                {
                    selected_file = current_path / file.first;
                    file_loaded = false;
                }
            }
        }

        // File Editor
        if (selected_file != fs::path())
        {
            string file_name = selected_file.filename().string();
            float side_menu_width = ImGui::GetWindowWidth();

            ImGui::SetNextWindowPos(ImVec2(210, ImGui::GetFrameHeight()), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(static_cast<float>(GetScreenWidth() - side_menu_width),
                                            static_cast<float>(GetScreenHeight() - ImGui::GetFrameHeight())));
            ImGui::SetNextWindowPos(ImVec2(side_menu_width, ImGui::GetFrameHeight()), ImGuiCond_Always);
            ImGui::Begin(file_name.c_str(), nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

            if (find(supported_file_types.begin(), supported_file_types.end(),
                     selected_file.extension()) != supported_file_types.end())
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
                            ImGui::TextColored({1, 0, 0, 1}, // RGBA Red color
                                               "File too large! Maximum size: %d MB",
                                               MAX_BUFFER_SIZE / (1024 * 1024));
                        }
                        else
                        {
                            file_content.resize(fileSize);
                            file.read(&file_content[0], fileSize);
                            file_loaded = true;
                        }
                        file.close();
                    }
                }

                if (file_loaded)
                {
                    ImGui::InputTextMultiline("##multiline", &file_content[0], file_content.capacity(), ImVec2(-1, -1));
                }
            }
            else
            {
                ImGui::TextColored({1, 0, 0, 1}, "Unsupported file format for preview"); // RGBA Red color
            }
            ImGui::End();
        }
        else
        {
            file_loaded = false;
        }

        ImGui::End();
        rlImGuiEnd();
        EndDrawing();
    }

    rlImGuiShutdown();
    CloseWindow();
    return 0;
}