#pragma once

#include "Data/Project.h"
#include "Data/World.h"
#include "UI/Wizard.h"
#include "imgui.h"
#include "raylib.h"
#include "rlImGui.h"

namespace Genesis {

class Application {
public:
  Application();
  ~Application();

  void Run();

private:
  const int screenWidth = 1600;
  const int screenHeight = 900;

  Camera3D camera = {0};
  UI::Wizard wizard;

  // Systems
  std::shared_ptr<Data::World> world;
  Data::Project project;

  // Visualization
  enum class RenderMode { Unlit, Lit, Wireframe };
  RenderMode currentRenderMode = RenderMode::Lit;
  Shader lightingShader;
  Shader unlitShader;

  // Camera Control State
  void UpdateCustomCamera();
  void ResetCamera();
  float cameraAngleY = 0.0f;    // Rotation around Y axis
  float cameraDistance = 30.0f; // Distance from target
  Vector3 cameraTarget = {
      50.0f, 0.0f, 50.0f}; // Target view point (center of terrain approx)
};

} // namespace Genesis
