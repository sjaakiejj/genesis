#include "RiverGenerator.h"
#include "TerrainGenerator.h" // For re-generating mesh if needed (or we can just update colors)
#include "raymath.h"
#include <cstdlib>
#include <vector>

namespace Genesis::Generator {

void RiverGenerator::Generate(Data::World &world, const Config &config,
                              const TerrainGenerator::Config &terrainConfig) {
  if (!world.terrain)
    return;
  auto terrain = world.terrain.get();

  // Restore base heightmap to clear previous erosion
  if (!terrain->baseHeightMap.empty() &&
      terrain->baseHeightMap.size() == terrain->heightMap.size()) {
    terrain->heightMap = terrain->baseHeightMap;
    // Invalidate erosion snapshot since we reverted to base
    terrain->preErosionHeightMap.clear();
  }

  // Always reset river map before generation
  terrain->riverMap.assign(terrain->heightMap.size(), 0);

  // Attempt to spawn rivers
  int riversCreated = 0;
  int attempts = 0;
  int maxAttempts = config.riverCount * 20; // Increase attempts logic

  while (riversCreated < config.riverCount && attempts < maxAttempts) {
    attempts++;

    int x = GetRandomValue(0, terrain->width - 1);
    int z = GetRandomValue(0, terrain->depth - 1);

    float h = terrain->GetHeight(x, z);

    // Criteria for source
    if (h >= config.minSourceHeight) {
      // Check if already a river?
      if (terrain->GetRiverType(x, z) == 0) {
        // Trace and check success (length)
        if (TraceRiver(terrain, x, z, terrainConfig.seaLevel,
                       config.minRiverLength)) {
          riversCreated++;
        }
      }
    }
  }

  // After generating data, we update the mesh visuals using the provided
  // terrain config
  TerrainGenerator::RebuildMesh(terrain, terrainConfig);
}

// Return true if river was successfully created (met min length)
bool RiverGenerator::TraceRiver(Data::Terrain *terrain, int startX, int startZ,
                                float seaLevel, int minLength) {
  int cx = startX;
  int cz = startZ;

  // Track path and changes to revert if needed
  struct Point {
    int x, z;
  };
  struct HeightChange {
    int x, z;
    float oldH;
  };

  std::vector<Point> path;
  std::vector<HeightChange> heightChanges;

  // Limit length to avoid infinite loops
  int maxSteps = 1000;
  bool reachedSea = false;
  bool stuckAndFailed = false;

  // We need to tentatively modify terrain for carving, but be able to revert?
  // Actually, revert is hard if we modify heightmap.
  // Better approach: We commit changes only if we confirm the river is valid?
  // But carving affects pathfinding.
  // Let's just run it, and if it fails, we keep the erosion (nature does that)
  // or we accept that failed rivers leave scars. For now, let's just check
  // length. If short and didn't reach sea, maybe clear the river markers but
  // keep height changes.

  for (int i = 0; i < maxSteps; i++) {
    // Mark current
    // Check if we are overwriting an existing river?
    // if (terrain->GetRiverType(cx, cz) == 2) { ... merge ... }

    terrain->riverMap[cz * terrain->width + cx] = 2; // 2 = River Body
    path.push_back({cx, cz});

    float currentH = terrain->GetHeight(cx, cz);

    // Stop if we reached the sea
    if (currentH < seaLevel) {
      reachedSea = true;
      break;
    }

    // Find lowest neighbor
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

    // If no lower neighbor, we are in a local minimum.
    // SEARCH RADIUS for lower ground to carve towards.
    if (nextX == -1) {
      bool foundTarget = false;
      int searchRadius = 20; // Increased radius
      int targetX = -1, targetZ = -1;
      float bestTargetH = currentH;

      // Spiral or box search
      for (int r = 1; r <= searchRadius; r++) {
        for (int dz = -r; dz <= r; dz++) {
          for (int dx = -r; dx <= r; dx++) {
            int nx = cx + dx;
            int nz = cz + dz;
            if (nx >= 0 && nx < terrain->width && nz >= 0 &&
                nz < terrain->depth) {
              float nh = terrain->GetHeight(nx, nz);
              if (nh < currentH) {
                // Found lower ground!
                targetX = nx;
                targetZ = nz;
                bestTargetH = nh;
                foundTarget = true;
                goto found_lower; // Break out of nested loops
              }
            }
          }
        }
      }

    found_lower:;

      if (foundTarget) {
        // Carve trench from current to target
        // Simple line carving: interpolate heights
        // We need to move step-by-step to target
        int dx = targetX - cx;
        int dz = targetZ - cz;
        int steps = std::max(abs(dx), abs(dz));

        for (int s = 1; s <= steps; s++) {
          float t = (float)s / steps;
          int ix = cx + (targetX - cx) * t;
          int iz = cz + (targetZ - cz) * t;

          // Record old height before change
          float oldVal = terrain->GetHeight(ix, iz);

          float interpolatedH = currentH + (bestTargetH - currentH) * t;
          terrain->SetHeight(ix, iz, interpolatedH);

          heightChanges.push_back({ix, iz, oldVal});
        }

        // Move to immediate neighbor towards target
        nextX =
            cx + (int)(copysign(1, dx) *
                       (abs(dx) > abs(dz) ? 1 : 0)); // Simplified Unit Step?
        // Actually, standard bresenham-like step
        if (abs(dx) >= abs(dz)) {
          nextX = cx + (dx > 0 ? 1 : -1);
          nextZ = cz + (int)((float)dz / abs(dx) + 0.5f); // Approx
        } else {
          nextX = cx + (int)((float)dx / abs(dz) + 0.5f);
          nextZ = cz + (dz > 0 ? 1 : -1);
        }

        // Force neighbor height lower if needed
        if (terrain->GetHeight(nextX, nextZ) >= currentH) {
          float oldVal = terrain->GetHeight(nextX, nextZ);
          float newVal = currentH - 0.001f;
          terrain->SetHeight(nextX, nextZ, newVal);
          heightChanges.push_back({nextX, nextZ, oldVal});
        }

      } else {
        // No lower ground found even in radius. Truly stuck or at global bottom
        // of a pit. Create a lake? For now, stop.
        stuckAndFailed = true;
        break;
      }
    }

    // Move
    cx = nextX;
    cz = nextZ;
  }

  // Validate Success
  // Fail if: Too short OR Stuck (didn't reach sea/lake target)
  // Actually, stuckAndFailed means we are in a pit with no escape.
  // If we just ran out of steps (maxSteps) but didn't stick, that's maybe ok?
  // No, likely just stopped. Ideally we want all rivers to reach sea.

  if (path.size() < minLength || stuckAndFailed) {
    // Revert Everything

    // 1. Clear river markers
    for (const auto &p : path) {
      terrain->riverMap[p.z * terrain->width + p.x] = 0;
    }

    // 2. Restore Heightmap
    // Iterate backwards to be safe (though order shouldn't matter for
    // independent cells)
    for (int i = heightChanges.size() - 1; i >= 0; i--) {
      terrain->SetHeight(heightChanges[i].x, heightChanges[i].z,
                         heightChanges[i].oldH);
    }

    return false;
  }

  return true;
}

} // namespace Genesis::Generator
