// Raylib and ImGui headers for graphics and GUI
#include <raylib.h>
#include <rlImGui.h>
#include <imgui.h>

// File dialog header
#include <imfilebrowser.h>

// Standard C++ headers
#include <iostream>
#include <filesystem>
#include <iomanip>
#include <map>
#include <fstream>
#include <array>

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
        for (const fs::directory_entry &entry : fs::directory_iterator(path))
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

    array<string, 33> supported_file_types = {
        ".txt", ".cpp", ".h", ".hpp", ".c", ".py", ".js", ".html", ".css",
        ".json", ".md", ".xml", ".yaml", ".ini", ".log", ".bat", ".sh", ".php",
        ".rb", ".go", ".swift", ".ts", ".tsx", ".vue", ".sql", ".pl", ".lua",
        ".r", ".dart", ".scala", ".rs", ".java", ".kt"};

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        rlImGuiBegin();

        static bool open = false;

        ImGui::GetStyle().FramePadding.y = 6.0f;

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Open"))
                {
                    open = !open;
                }
                if (ImGui::MenuItem("Save"))
                {
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
                file_browser.ClearSelected();               // Clear selection for next use
                current_path = file_browser.GetDirectory(); // Update current path to selected directory
                open = false;                               // Close the file browser after selection
                file_browser.Close();                       // Close the file browser
            }
        }

        // Create a side menu window with fixed position and full height
        ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetFrameHeight()), ImGuiCond_Always);
        ImGui::SetNextWindowSizeConstraints(
            ImVec2(200, static_cast<float>(GetScreenHeight())),
            ImVec2(500, static_cast<float>(GetScreenHeight())));
        ImGui::Begin("Explorer", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);

        // Display current Folder Name
        ImGui::Text("%s", current_path.filename().string().c_str());

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
                }
            }
        }

        // File Editor
        if (selected_file != fs::path())
        {
            string file_name = selected_file.filename().string();
            float side_menu_width = ImGui::GetWindowWidth(); // Get the current width of side menu

            ImGui::SetNextWindowPos(ImVec2(210, ImGui::GetFrameHeight()), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(static_cast<float>(GetScreenWidth() - side_menu_width), static_cast<float>(GetScreenHeight())));
            ImGui::SetNextWindowPos(ImVec2(side_menu_width, ImGui::GetFrameHeight()), ImGuiCond_Always);
            ImGui::Begin(file_name.c_str(), nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

            if (find(supported_file_types.begin(), supported_file_types.end(), selected_file.extension()) != supported_file_types.end())
            {
                ifstream file(selected_file);
                if (file.is_open())
                {
                    string line;
                    while (getline(file, line))
                    {
                        ImGui::TextWrapped("%s", line.c_str());
                    }

                    file.close();
                }
                else
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Could not open file.");
                }
            }
            else
            {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Unsupported file format for preview");
            }
            ImGui::End();
        }

        ImGui::End();
        rlImGuiEnd();
        EndDrawing();
    }

    rlImGuiShutdown();
    CloseWindow();
    return 0;
}