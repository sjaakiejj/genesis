#pragma once

#include "../Data/World.h"

#include "../Generator/TerrainGenerator.h"

namespace Genesis::Generator {

class RiverGenerator {
public:
  struct Config {
    int riverCount = 5;
    int minRiverLength = 10;
    float minSourceHeight = 0.5f; // Only start rivers high up
  };

  static void Generate(Data::World &world, const Config &config,
                       const TerrainGenerator::Config &terrainConfig);

private:
  static void TraceRiver(Data::Terrain *terrain, int startX, int startZ);
};

} // namespace Genesis::Generator
