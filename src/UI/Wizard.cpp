#include "Wizard.h"
#include "../Data/Project.h"
#include "../Data/World.h"
#include "../Generator/RiverGenerator.h"
#include "../Generator/TerrainGenerator.h"
#include <filesystem>
#include <map>
#include <string>
#include <vector>

// Cross-platform compatibility for strcpy_s
#ifndef _WIN32
#include <cstring>
#define strcpy_s(dest, size, src) strncpy(dest, src, size)
#endif

namespace Genesis::UI {

// Map enum to display names
const std::map<WizardStep, std::string> StepNames = {
    {WizardStep::Macro_Terrain, "1. Macro: Terrain"},
    {WizardStep::Rivers_Water, "2. Macro: Rivers"},
    {WizardStep::Infrastructure_Roads, "3. Infrastructure: Roads"},
    {WizardStep::Zoning_Districts, "4. Zoning: Districts"},
    {WizardStep::Parcels_Subdivision, "5. Parcels: Subdivision"},
    {WizardStep::Buildings_Structure, "6. Buildings: Structure"},
    {WizardStep::Interiors_Furnishing, "7. Interiors: Furnishing"},
    {WizardStep::Export, "8. Export"}};

Wizard::Wizard() {}

void Wizard::Draw(std::shared_ptr<Genesis::Data::World> world,
                  Genesis::Data::Project &project) {
  ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(300, 600), ImGuiCond_FirstUseEver);

  if (ImGui::Begin("Genesis Wizard", nullptr,
                   ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse)) {
    DrawMenuBar(project, world); // Draw Menu Bar
    DrawSidebar();
    ImGui::Separator();
    DrawCurrentStepFor(world, project);
  }
  ImGui::End();
}

// Modal State
static bool showNewProjectModal = false;
static bool showSaveAsModal = false;
static bool showLoadModal = false;
static char inputProjectName[128] = "Untitled";
static char inputFileName[128] = "project.json";

void Wizard::DrawMenuBar(Genesis::Data::Project &project,
                         std::shared_ptr<Genesis::Data::World> world) {
  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("New Project")) {
        showNewProjectModal = true;
        strcpy_s(inputProjectName, 128, "Untitled");
      }
      if (ImGui::MenuItem("Save Project", "Ctrl+S")) {
        if (project.path.empty()) {
          showSaveAsModal = true;
          strcpy_s(inputFileName, 128, "project.json");
        } else {
          // Capture current config
          Genesis::Data::Project::ConfigSnapshot snapshot;
          snapshot.terrain = currentTerrainConfig;
          project.Save(project.path, snapshot);
        }
      }
      if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {
        showSaveAsModal = true;
        strcpy_s(inputFileName, 128,
                 project.path.empty() ? "project.json" : project.path.c_str());
      }
      if (ImGui::MenuItem("Load Project", "Ctrl+O")) {
        showLoadModal = true;
      }

      ImGui::Separator();

      if (ImGui::MenuItem("Undo", "Ctrl+Z", false, project.CanUndo())) {
        Genesis::Data::Project::ConfigSnapshot snapshot;
        if (project.Undo(snapshot)) {
          Genesis::Generator::TerrainGenerator::Generate(*world,
                                                         snapshot.terrain);
          // Sync UI
          currentTerrainConfig = snapshot.terrain;
        }
      }
      if (ImGui::MenuItem("Redo", "Ctrl+Y", false, project.CanRedo())) {
        Genesis::Data::Project::ConfigSnapshot snapshot;
        if (project.Redo(snapshot)) {
          Genesis::Generator::TerrainGenerator::Generate(*world,
                                                         snapshot.terrain);
          // Sync UI
          currentTerrainConfig = snapshot.terrain;
        }
      }

      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }

  // --- Modals ---

  // 1. New Project Modal
  if (showNewProjectModal)
    ImGui::OpenPopup("New Project");
  if (ImGui::BeginPopupModal("New Project", NULL,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::InputText("Project Name", inputProjectName, 128);

    if (ImGui::Button("Create", ImVec2(120, 0))) {
      project.name = inputProjectName;
      project.path = "";
      // Reset logic here if needed
      showNewProjectModal = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
      showNewProjectModal = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  // 2. Save As Modal
  if (showSaveAsModal)
    ImGui::OpenPopup("Save Project As");
  if (ImGui::BeginPopupModal("Save Project As", NULL,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::InputText("Filename", inputFileName, 128);

    if (ImGui::Button("Save", ImVec2(120, 0))) {
      Genesis::Data::Project::ConfigSnapshot snapshot;
      snapshot.terrain = currentTerrainConfig;

      project.Save(inputFileName, snapshot);
      showSaveAsModal = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
      showSaveAsModal = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  // 3. Load Modal
  if (showLoadModal)
    ImGui::OpenPopup("Load Project");
  if (ImGui::BeginPopupModal("Load Project", NULL,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::Text("Select a file:");
    ImGui::Separator();

    // List files in current directory
    ImGui::BeginChild("FileList", ImVec2(300, 200), true);
    namespace fs = std::filesystem;
    // Safety check for filesystem support
    try {
      for (const auto &entry : fs::directory_iterator(".")) {
        if (entry.path().extension() == ".json") {
          if (ImGui::Selectable(entry.path().filename().string().c_str())) {
            strcpy_s(inputFileName, 128,
                     entry.path().filename().string().c_str());
          }
        }
      }
    } catch (...) {
    }
    ImGui::EndChild();

    ImGui::InputText("Filename", inputFileName, 128);

    if (ImGui::Button("Load", ImVec2(120, 0))) {
      Genesis::Data::Project::ConfigSnapshot snapshot;
      if (project.Load(inputFileName, snapshot)) {
        Genesis::Generator::TerrainGenerator::Generate(*world,
                                                       snapshot.terrain);
        // Update UI state
        currentTerrainConfig = snapshot.terrain;

        // Also restore history? For now just snapshot.
        project.PushSnapshot(snapshot);
      }
      showLoadModal = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
      showLoadModal = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

void Wizard::DrawSidebar() {
  ImGui::BeginChild("Sidebar", ImVec2(0, 150), true);
  for (auto const &[step, name] : StepNames) {
    bool isSelected = (currentStep == step);
    if (ImGui::Selectable(name.c_str(), isSelected)) {
      currentStep = step;
    }
  }
  ImGui::EndChild();
}

void Wizard::DrawCurrentStepFor(std::shared_ptr<Genesis::Data::World> world,
                                Genesis::Data::Project &project) {
  ImGui::Dummy(ImVec2(0, 10));
  std::string stepName = "Unknown";
  if (StepNames.find(currentStep) != StepNames.end()) {
    stepName = StepNames.at(currentStep);
  }
  ImGui::TextColored(ImVec4(1, 1, 0, 1), "Configuration: %s", stepName.c_str());
  ImGui::Separator();

  // Use member variable 'currentTerrainConfig'

  switch (currentStep) {
  case WizardStep::Macro_Terrain: {
    ImGui::Text("Terrain Settings");

    ImGui::InputInt("Seed", &currentTerrainConfig.seed);
    if (ImGui::Button("Randomize Seed")) {
      currentTerrainConfig.seed = GetRandomValue(0, 1000000);
    }

    ImGui::Separator();
    ImGui::SliderInt("Size", &currentTerrainConfig.width, 50, 500);
    currentTerrainConfig.depth = currentTerrainConfig.width;

    ImGui::SliderFloat("Noise Scale", &currentTerrainConfig.noiseScale, 0.1f,
                       20.0f);
    ImGui::SliderFloat("Height", &currentTerrainConfig.heightMultiplier, 1.0f,
                       50.0f);
    ImGui::SliderFloat("Sea Level", &currentTerrainConfig.seaLevel, 0.0f, 1.0f);

    if (ImGui::Button("Generate Terrain", ImVec2(280, 30))) {
      Genesis::Generator::TerrainGenerator::Generate(*world,
                                                     currentTerrainConfig);

      if (world->tensorField)
        world->tensorField->Resize(currentTerrainConfig.width,
                                   currentTerrainConfig.depth);

      // Record History
      Genesis::Data::Project::ConfigSnapshot snapshot;
      snapshot.terrain = currentTerrainConfig;
      project.PushSnapshot(snapshot);
    }
    break;
  }
  case WizardStep::Rivers_Water: {
    ImGui::Text("River Generation");
    ImGui::TextWrapped("Click to sprout rivers from random high points.");

    static Generator::RiverGenerator::Config riverConfig;
    ImGui::SliderInt("River Count", &riverConfig.riverCount, 1, 50);
    ImGui::SliderInt("Min Length", &riverConfig.minRiverLength, 5, 50);
    ImGui::SliderFloat("Source H", &riverConfig.minSourceHeight, 0.0f, 1.0f);

    if (ImGui::Button("Generate Rivers", ImVec2(280, 30))) {
      Genesis::Generator::RiverGenerator::Generate(*world, riverConfig,
                                                   currentTerrainConfig);

      // History? Rivers are part of terrain state now if stored in riverMap.
      // But riverMap is checked by TerrainGenerator ONLY IF it exists.
      // Yes, we should probably record this. But Snapshots only save Configs
      // currently... NOT the actual map data. This is a flaw in my Project Save
      // system. The current Project System only saves CONFIGs, not DATA. So if
      // I restart, the rivers are gone unless I re-run the generator with the
      // SAME seed. BUT RiverGenerator uses random placement! So I need a seed
      // for RiverGenerator too.
    }
    break;
  }
  case WizardStep::Infrastructure_Roads: {
    ImGui::Text("Tensor Field Settings");
    static int tensorSeed = 12345;
    ImGui::InputInt("Tensor Seed", &tensorSeed);

    if (ImGui::Button("Calculate Tensor Field")) {
      if (world->tensorField)
        world->tensorField->Generate(tensorSeed);
    }
    break;
  }
  default:
    ImGui::Text("Not implemented yet.");
    break;
  }
}

} // namespace Genesis::UI
