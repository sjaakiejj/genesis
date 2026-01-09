#pragma once

#include "../Data/World.h"
#include "TerrainGenerator.h"

namespace Genesis::Generator {

class ErosionGenerator {
public:
  struct Config {
    int iterations = 50000;
    float erosionRate = 0.5f;
    float depositionRate = 0.5f;
    float gravity = 4.0f;
    float evaporationRate = 0.05f;
    int erosionRadius = 3;
    float maxLifetime = 30; // Max steps per droplet
    float inertia = 0.05f;  // How much previous direction affects movement
    float startSpeed = 1.0f;
    float startWater = 1.0f;
    float minSlope = 0.05f;
    float capacityFactor = 4.0f; // Multiplier for sediment capacity
  };

  static void Execute(Data::World &world, const Config &config,
                      const TerrainGenerator::Config &terrainConfig);

private:
  struct Droplet {
    float x, z;
    float dirX, dirZ;
    float speed;
    float water;
    float sediment;
  };

  // Helper to get gradient at position
  static void GetGradient(Data::Terrain *terrain, float x, float z, float &gx,
                          float &gz);

  static float GetHeightInterpolated(Data::Terrain *terrain, float x, float z);
};

} // namespace Genesis::Generator
