#pragma once

#include "raylib.h"
#include <vector>

namespace Genesis::Data {

struct Terrain {
  int width = 0;
  int depth = 0;
  float scale = 1.0f;

  // The raw height data (0.0f - 1.0f)
  std::vector<float> heightMap;
  std::vector<float> baseHeightMap; // Original heightmap for resetting
  // 0 = No River, 1 = River Source, 2 = River Body
  std::vector<int> riverMap;

  // The visual representation
  Mesh mesh = {0};
  Model model = {0};
  bool isModelLoaded = false;

  // Helper to get height at integer coordinates
  float GetHeight(int x, int z) const {
    if (x < 0 || x >= width || z < 0 || z >= depth)
      return 0.0f;
    return heightMap[z * width + x];
  }

  int GetRiverType(int x, int z) const {
    if (x < 0 || x >= width || z < 0 || z >= depth || riverMap.empty())
      return 0;
    return riverMap[z * width + x];
  }

  void SetHeight(int x, int z, float h) {
    if (x < 0 || x >= width || z < 0 || z >= depth)
      return;
    heightMap[z * width + x] = h;
  }

  // Destructor to clean up GPU resources
  ~Terrain() {
    if (isModelLoaded) {
      UnloadModel(model);
      // UnloadModel unloads meshes too usually, depending on how they were
      // created Raylib's GenMeshHeightmap ownership is tricky. We'll trust
      // Raylib's UnloadModel for now.
    }
  }
};

} // namespace Genesis::Data
