#pragma once

#include "../Generator/TerrainGenerator.h"
#include "imgui.h"
#include <memory>
#include <string>
#include <vector>

// Forward declaration
namespace Genesis::Data {
struct World;
class Project;
} // namespace Genesis::Data

namespace Genesis::UI {

enum class WizardStep {
  Macro_Terrain,
  Rivers_Water, // New Step
  Infrastructure_Roads,
  Zoning_Districts,
  Parcels_Subdivision,
  Buildings_Structure,
  Interiors_Furnishing,
  Export
};

class Wizard {
public:
  Wizard();
  // Main draw method takes the state logic
  void Draw(std::shared_ptr<Genesis::Data::World> world,
            Genesis::Data::Project &project);

private:
  WizardStep currentStep = WizardStep::Macro_Terrain;

  // Helper to draw the sidebar navigation
  void DrawSidebar();

  // Helper to draw the content area for the current step
  void DrawCurrentStepFor(std::shared_ptr<Genesis::Data::World> world,
                          Genesis::Data::Project &project);

  // Draw Menu Bar
  void DrawMenuBar(Genesis::Data::Project &project,
                   std::shared_ptr<Genesis::Data::World> world);

  // Current Configuration State for UI
  Genesis::Generator::TerrainGenerator::Config currentTerrainConfig;
};

} // namespace Genesis::UI
