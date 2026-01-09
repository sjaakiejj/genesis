#pragma once

#include "../Data/World.h"

namespace Genesis::Generator {

class TerrainGenerator {
public:
  struct Config {
    int width = 100;
    int depth = 100;
    float noiseScale =
        0.1f; // High scale = Zoomed in (smooth), Low scale = Zoomed out (noisy)
    int seed = 12345;
    float heightMultiplier = 10.0f;
    float seaLevel = 0.2f; // Heights below this are water
  };

  // Reads config, generates heightmap + mesh, writes to ctx.
  static void Generate(Data::World &world, const Config &config);

  // Rebuilds just the mesh from existing terrain data (useful after
  // rivers/erosion)
  static void RebuildMesh(Data::Terrain *terrain, const Config &config);
};

} // namespace Genesis::Generator
