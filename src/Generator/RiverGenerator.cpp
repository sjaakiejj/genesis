#include "RiverGenerator.h"
#include "TerrainGenerator.h" // For re-generating mesh if needed (or we can just update colors)
#include "raymath.h"
#include <cstdlib>
#include <vector>

namespace Genesis::Generator {

void RiverGenerator::Generate(Data::World &world, const Config &config) {
  if (!world.terrain)
    return;
  auto terrain = world.terrain.get();

  // Reset simple river map if we want to add to existing, we probably shouldn't
  // clear unless requested? For now, let's assume we clear rivers when we click
  // "Generate Rivers" But wait, if we want multiple passes? Let's clear for now
  // to avoid mess. Actually, TerrainGenerator clears it. RiverGenerator might
  // assume fresh start. If riverMap is empty, resize it.
  if (terrain->riverMap.size() != terrain->heightMap.size()) {
    terrain->riverMap.assign(terrain->heightMap.size(), 0);
  }

  // Attempt to spawn rivers
  int riversCreated = 0;
  int attempts = 0;
  int maxAttempts = config.riverCount * 10;

  while (riversCreated < config.riverCount && attempts < maxAttempts) {
    attempts++;

    int x = GetRandomValue(0, terrain->width - 1);
    int z = GetRandomValue(0, terrain->depth - 1);

    float h = terrain->GetHeight(x, z);

    // Criteria for source
    if (h >= config.minSourceHeight) {
      // Check if already a river?
      if (terrain->GetRiverType(x, z) == 0) {
        TraceRiver(terrain, x, z);
        riversCreated++;
      }
    }
  }

  // After generating data, we need to update the mesh visuals.
  // Since we didn't change heights (yet), we can just update colors?
  // But TerrainGenerator::Generate is the only way to rebuild currently.
  // Let's grab the LAST terrain config? Or just assume we can rebuild with
  // current data? TerrainGenerator::Generate FROM config REBUILDS the heightmap
  // from noise, wiping our rivers! This is a problem.

  // SOLUTION: We need a "RegenerateMesh(Terrain*)" function in TerrainGenerator
  // that uses existing heightmap and rivermap data, without re-sampling noise.
  // For now, I'll cheat and just Re-run TerrainGenerator logic purely for mesh
  // building locally here OR refactor TerrainGenerator.

  // Let's refactor TerrainGenerator slightly later.
  // For now, I will create a temporary config that matches the terrain
  // dimensions BUT wait, TerrainGenerator::Generate overrides heightmap.

  // FIX: I will add a static helper in RiverGenerator (or better, in
  // TerrainGenerator) to UpdateMesh(Terrain* terrain, float seaLevel). I need
  // seaLevel. It's not stored in Terrain. I will assume seaLevel 0.2f for now
  // or pass it in Config.
}

void RiverGenerator::TraceRiver(Data::Terrain *terrain, int startX,
                                int startZ) {
  int cx = startX;
  int cz = startZ;

  // Limit length to avoid infinite loops (though downhill checking prevents
  // loops mostly)
  int maxSteps = 1000;

  for (int i = 0; i < maxSteps; i++) {
    // Mark current
    terrain->riverMap[cz * terrain->width + cx] = 2; // 2 = River Body

    // Find lowest neighbor
    float currentH = terrain->GetHeight(cx, cz);
    float lowestH = currentH;
    int nextX = -1;
    int nextZ = -1;

    // Check 8 neighbors
    for (int dz = -1; dz <= 1; dz++) {
      for (int dx = -1; dx <= 1; dx++) {
        if (dx == 0 && dz == 0)
          continue;

        int nx = cx + dx;
        int nz = cz + dz;

        if (nx >= 0 && nx < terrain->width && nz >= 0 && nz < terrain->depth) {
          float nh = terrain->GetHeight(nx, nz);
          if (nh < lowestH) {
            lowestH = nh;
            nextX = nx;
            nextZ = nz;
          }
        }
      }
    }

    // If no lower neighbor, we are in a local minimum (lake or sea)
    if (nextX == -1) {
      break;
    }

    // Move
    cx = nextX;
    cz = nextZ;

    // TODO: Check if we hit existing river or sea?
    // If we hit sea level (need to know sea level), stop.
    // For now, simple downhill.
  }
}

} // namespace Genesis::Generator
