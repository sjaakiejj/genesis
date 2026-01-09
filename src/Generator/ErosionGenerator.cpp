#include "ErosionGenerator.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace Genesis::Generator {

void ErosionGenerator::Execute(Data::World &world, const Config &config,
                               const TerrainGenerator::Config &terrainConfig) {
  if (!world.terrain)
    return;
  auto terrain = world.terrain.get();

  // We modify terrain->heightMap directly
  // Snapshot logic to prevent additive erosion on repeated runs
  if (terrain->preErosionHeightMap.empty() ||
      terrain->preErosionHeightMap.size() != terrain->heightMap.size()) {
    // First run (or after river reset), take snapshot of CURRENT state (which
    // includes rivers)
    terrain->preErosionHeightMap = terrain->heightMap;
  } else {
    // Repeated run, restore from snapshot
    terrain->heightMap = terrain->preErosionHeightMap;
  }

  int width = terrain->width;
  int depth = terrain->depth;

  // Random setup
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<float> disX(0.0f, (float)width - 1.1f);
  std::uniform_real_distribution<float> disZ(0.0f, (float)depth - 1.1f);

  for (int iter = 0; iter < config.iterations; iter++) {
    // Spawn Droplet
    Droplet drop;
    drop.x = disX(gen);
    drop.z = disZ(gen);
    drop.dirX = 0;
    drop.dirZ = 0;
    drop.speed = config.startSpeed;
    drop.water = config.startWater;
    drop.sediment = 0;

    for (int step = 0; step < config.maxLifetime; step++) {
      int nodeX = (int)drop.x;
      int nodeZ = (int)drop.z;
      float cellOffsetX = drop.x - nodeX;
      float cellOffsetZ = drop.z - nodeZ;

      // Get gradient
      float gx, gz;
      GetGradient(terrain, drop.x, drop.z, gx, gz);

      // Update Direction
      drop.dirX = (drop.dirX * config.inertia - gx * (1 - config.inertia));
      drop.dirZ = (drop.dirZ * config.inertia - gz * (1 - config.inertia));

      // Normalize direction
      float len = std::sqrt(drop.dirX * drop.dirX + drop.dirZ * drop.dirZ);
      if (len != 0) {
        drop.dirX /= len;
        drop.dirZ /= len;
      }

      // Move
      drop.x += drop.dirX;
      drop.z += drop.dirZ;

      // Check bounds
      if (drop.x < 0 || drop.x >= width - 1 || drop.z < 0 ||
          drop.z >= depth - 1) {
        break;
      }

      // Calculate height difference
      float heightOld = GetHeightInterpolated(terrain, nodeX + cellOffsetX,
                                              nodeZ + cellOffsetZ);
      float heightNew = GetHeightInterpolated(terrain, drop.x, drop.z);
      float deltaH = heightNew - heightOld;

      // Calculate Capacity
      // Capacity is steeper slope + faster speed = more capacity
      float sedimentCapacity = std::max(-deltaH, config.minSlope) * drop.speed *
                               drop.water * config.capacityFactor;

      // Erosion or Deposition
      if (drop.sediment > sedimentCapacity || deltaH > 0) {
        // Deposit
        float amountToDeposit =
            (drop.sediment - sedimentCapacity) * config.depositionRate;
        if (deltaH > 0) {
          // Moving uphill? Dump all sediment to fill pit
          amountToDeposit = std::min(deltaH, drop.sediment);
        }

        drop.sediment -= amountToDeposit;

        // Add to heightmap (bilinear)
        // We add to the 4 nodes around old pos
        terrain->heightMap[nodeZ * width + nodeX] +=
            amountToDeposit * (1 - cellOffsetX) * (1 - cellOffsetZ);
        terrain->heightMap[nodeZ * width + (nodeX + 1)] +=
            amountToDeposit * cellOffsetX * (1 - cellOffsetZ);
        terrain->heightMap[(nodeZ + 1) * width + nodeX] +=
            amountToDeposit * (1 - cellOffsetX) * cellOffsetZ;
        terrain->heightMap[(nodeZ + 1) * width + (nodeX + 1)] +=
            amountToDeposit * cellOffsetX * cellOffsetZ;

      } else {
        // Erode
        float amountToErode = std::min(
            (sedimentCapacity - drop.sediment) * config.erosionRate, -deltaH);

        // Don't erode more than deltaH (don't dig a pit deeper than the step)

        // Apply erosion to nodes
        // For better look, we could erode with a radius (brush), but simple
        // bilinear is faster
        terrain->heightMap[nodeZ * width + nodeX] -=
            amountToErode * (1 - cellOffsetX) * (1 - cellOffsetZ);
        terrain->heightMap[nodeZ * width + (nodeX + 1)] -=
            amountToErode * cellOffsetX * (1 - cellOffsetZ);
        terrain->heightMap[(nodeZ + 1) * width + nodeX] -=
            amountToErode * (1 - cellOffsetX) * cellOffsetZ;
        terrain->heightMap[(nodeZ + 1) * width + (nodeX + 1)] -=
            amountToErode * cellOffsetX * cellOffsetZ;

        drop.sediment += amountToErode;
      }

      // Update Speed & Water
      float speedSq = drop.speed * drop.speed + deltaH * config.gravity;
      drop.speed = std::sqrt(std::max(0.0f, speedSq));
      drop.water *= (1 - config.evaporationRate);

      if (drop.water < 0.01f)
        break;
    }
  }

  // Rebuild mesh after erosion
  TerrainGenerator::RebuildMesh(terrain, terrainConfig);
}

void ErosionGenerator::GetGradient(Data::Terrain *terrain, float x, float z,
                                   float &gx, float &gz) {
  int nodeX = (int)x;
  int nodeZ = (int)z;
  float u = x - nodeX;
  float v = z - nodeZ;

  // Get heights of 4 neighbors
  // Indices must be safe due to loop bounds
  int w = terrain->width;
  float h00 = terrain->heightMap[nodeZ * w + nodeX];
  float h10 = terrain->heightMap[nodeZ * w + (nodeX + 1)];
  float h01 = terrain->heightMap[(nodeZ + 1) * w + nodeX];
  float h11 = terrain->heightMap[(nodeZ + 1) * w + (nodeX + 1)];

  // Bilinear gradient approximation
  gx = (h10 - h00) * (1 - v) + (h11 - h01) * v;
  gz = (h01 - h00) * (1 - u) + (h11 - h10) * u;
}

float ErosionGenerator::GetHeightInterpolated(Data::Terrain *terrain, float x,
                                              float z) {
  int nodeX = (int)x;
  int nodeZ = (int)z;
  float u = x - nodeX;
  float v = z - nodeZ;

  int w = terrain->width;
  float h00 = terrain->heightMap[nodeZ * w + nodeX];
  float h10 = terrain->heightMap[nodeZ * w + (nodeX + 1)];
  float h01 = terrain->heightMap[(nodeZ + 1) * w + nodeX];
  float h11 = terrain->heightMap[(nodeZ + 1) * w + (nodeX + 1)];

  return (h00 * (1 - u) * (1 - v)) + (h10 * u * (1 - v)) + (h01 * (1 - u) * v) +
         (h11 * u * v);
}

} // namespace Genesis::Generator
