#include "Application.h"
#include "raymath.h"
#include "rlgl.h"

namespace Genesis {

Application::Application() {
  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
  InitWindow(screenWidth, screenHeight, "Genesis - Procedural City Generator");
  SetTargetFPS(60);

  rlImGuiSetup(true);

  // Initialize World Context
  world = std::make_shared<Data::World>();

  ResetCamera();

  // Basic Lighting Shader
  const char *vs = R"(
            #version 330
            in vec3 vertexPosition;
            in vec2 vertexTexCoord;
            in vec3 vertexNormal;
            in vec4 vertexColor;
            out vec3 fragPosition;
            out vec2 fragTexCoord;
            out vec4 fragColor;
            out vec3 fragNormal;
            uniform mat4 mvp;
            uniform mat4 matModel;
            uniform mat4 matNormal;
            void main() {
                fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));
                fragTexCoord = vertexTexCoord;
                fragColor = vertexColor;
                fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 1.0)));
                gl_Position = mvp * vec4(vertexPosition, 1.0);
            }
        )";

  const char *fs = R"(
            #version 330
            in vec3 fragPosition;
            in vec2 fragTexCoord;
            in vec4 fragColor;
            in vec3 fragNormal;
            out vec4 finalColor;
            uniform vec3 lightDir;
            uniform vec4 lightColor;
            uniform vec4 ambientColor;
            void main() {
                float NdotL = max(dot(fragNormal, -lightDir), 0.0);
                vec4 diffuse = lightColor * NdotL;
                vec4 ambient = ambientColor;
                finalColor = fragColor * (ambient + diffuse);
            }
        )";

  lightingShader = LoadShaderFromMemory(vs, fs);

  Vector3 lightDir = Vector3Normalize((Vector3){-1.0f, -1.0f, -1.0f});
  Vector4 lightColor = (Vector4){1.0f, 1.0f, 0.9f, 1.0f};
  Vector4 ambientColor = (Vector4){0.1f, 0.1f, 0.15f, 1.0f};

  int lightDirLoc = GetShaderLocation(lightingShader, "lightDir");
  int lightColorLoc = GetShaderLocation(lightingShader, "lightColor");
  int ambientColorLoc = GetShaderLocation(lightingShader, "ambientColor");

  SetShaderValue(lightingShader, lightDirLoc, &lightDir, SHADER_UNIFORM_VEC3);
  SetShaderValue(lightingShader, lightColorLoc, &lightColor,
                 SHADER_UNIFORM_VEC4);
  SetShaderValue(lightingShader, ambientColorLoc, &ambientColor,
                 SHADER_UNIFORM_VEC4);

  // Basic Unlit Shader (Vertex Color Pass-through)
  const char *unlitVs = R"(
            #version 330
            in vec3 vertexPosition;
            in vec2 vertexTexCoord;
            in vec3 vertexNormal;
            in vec4 vertexColor;
            out vec4 fragColor;
            uniform mat4 mvp;
            void main() {
                fragColor = vertexColor;
                gl_Position = mvp * vec4(vertexPosition, 1.0);
            }
        )";
  const char *unlitFs = R"(
            #version 330
            in vec4 fragColor;
            out vec4 finalColor;
            void main() {
                finalColor = fragColor;
            }
        )";
  unlitShader = LoadShaderFromMemory(unlitVs, unlitFs);
}

void Application::ResetCamera() {
  cameraAngleY = 0.0f;
  cameraDistance = 70.0f;
  cameraTarget = (Vector3){50.0f, 0.0f, 50.0f};

  camera.position = (Vector3){0.0f, 20.0f, 0.0f};
  camera.target = cameraTarget;
  camera.up = (Vector3){0.0f, 1.0f, 0.0f};
  camera.fovy = 45.0f;
  camera.projection = CAMERA_PERSPECTIVE;
  UpdateCustomCamera();
}

void Application::UpdateCustomCamera() {
  float speed = 0.5f;
  float rotateSpeed = 0.02f;
  float zoomSpeed = 2.0f;

  // Pan
  if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
    Vector2 delta = GetMouseDelta();
    Vector3 forward =
        Vector3Normalize(Vector3Subtract(cameraTarget, camera.position));
    forward.y = 0;
    forward = Vector3Normalize(forward);
    Vector3 right = Vector3CrossProduct(forward, camera.up);

    // Fixed Pan Direction: Removed negative sign on delta.y to match natural
    // drag
    Vector3 move = Vector3Add(Vector3Scale(right, -delta.x * speed * 0.1f),
                              Vector3Scale(forward, delta.y * speed * 0.1f));
    cameraTarget = Vector3Add(cameraTarget, move);
  }

  // Zoom
  float wheel = GetMouseWheelMove();
  if (wheel != 0) {
    cameraDistance -= wheel * zoomSpeed;
    if (cameraDistance < 2.0f)
      cameraDistance = 2.0f;
  }

  // Rotate (WASD)
  if (IsKeyDown(KEY_A))
    cameraAngleY -= rotateSpeed;
  if (IsKeyDown(KEY_D))
    cameraAngleY += rotateSpeed;

  // Orbit
  float yOffset = cameraDistance * 0.6f;
  float distOnPlane = cameraDistance * 0.8f;

  camera.position.x = cameraTarget.x + sinf(cameraAngleY) * distOnPlane;
  camera.position.z = cameraTarget.z + cosf(cameraAngleY) * distOnPlane;
  camera.position.y = yOffset;

  camera.target = cameraTarget;
}

Application::~Application() {
  UnloadShader(lightingShader);
  UnloadShader(unlitShader);
  rlImGuiShutdown();
  CloseWindow();
}

void Application::Run() {
  while (!WindowShouldClose()) {
    UpdateCustomCamera();

    if (IsKeyPressed(KEY_F1))
      currentRenderMode = RenderMode::Unlit;
    if (IsKeyPressed(KEY_F2))
      currentRenderMode = RenderMode::Lit;
    if (IsKeyPressed(KEY_F3))
      currentRenderMode = RenderMode::Wireframe;
    if (IsKeyPressed(KEY_R))
      ResetCamera();

    BeginDrawing();
    ClearBackground(Color{30, 30, 30, 255});

    BeginMode3D(camera);
    DrawGrid(200, 1.0f);

    if (world->terrain->isModelLoaded) {
      Model &model = world->terrain->model;
      if (currentRenderMode == RenderMode::Lit) {
        model.materials[0].shader = lightingShader;
        DrawModel(model, {0, 0, 0}, 1.0f, WHITE);
      } else if (currentRenderMode == RenderMode::Unlit) {
        model.materials[0].shader = unlitShader;
        DrawModel(model, {0, 0, 0}, 1.0f, WHITE);
      } else if (currentRenderMode == RenderMode::Wireframe) {
        DrawModelWires(model, {0, 0, 0}, 1.0f, GREEN);
      }
    }

    world->tensorField->DrawDebug(0.1f);
    EndMode3D();

    rlImGuiBegin();
    wizard.Draw(world, project);

    ImGui::SetNextWindowPos(ImVec2(screenWidth - 220, 10));
    ImGui::Begin("Controls", nullptr,
                 ImGuiWindowFlags_NoDecoration |
                     ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoBackground);
    ImGui::TextColored(ImVec4(0, 1, 0, 1), "Controls:");
    ImGui::Text("Drag Left Mouse: Pan");
    ImGui::Text("Scroll: Zoom");
    ImGui::Text("A/D: Rotate View");
    ImGui::Text("R: Reset Camera");
    ImGui::Separator();
    ImGui::Text("F1: Unlit  F2: Lit  F3: Wireframe");
    ImGui::End();
    rlImGuiEnd();

    EndDrawing();
  }
}

} // namespace Genesis
