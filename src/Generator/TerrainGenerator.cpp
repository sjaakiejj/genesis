#include "TerrainGenerator.h"
#include "raymath.h"
#include <cmath>
#include <vector>

namespace Genesis::Generator {

// Simple helper to map height to color
Color GetColorForHeight(float h, float seaLevel, int riverType) {
  if (riverType > 0)
    return BLUE; // River
  if (h < seaLevel)
    return {0, 105, 148, 255}; // Deep Sea Blue
  if (h < seaLevel + 0.05f)
    return BEIGE; // Sand
  if (h < 0.6f)
    return DARKGREEN; // Grass
  if (h < 0.8f)
    return GRAY; // Rock
  return WHITE;  // Snow
}

// Helper to calculate vertex normal using central differences
Vector3 GetVertexNormal(Data::Terrain *terrain, int x, int z,
                        float heightMultiplier) {
  float hL = terrain->GetHeight(x - 1, z) * heightMultiplier;
  float hR = terrain->GetHeight(x + 1, z) * heightMultiplier;
  float hD = terrain->GetHeight(x, z - 1) * heightMultiplier;
  float hU = terrain->GetHeight(x, z + 1) * heightMultiplier;

  // Vectors corresponding to the slope
  Vector3 vHorizontal = {2.0f, hR - hL, 0.0f};
  Vector3 vVertical = {0.0f, hU - hD, 2.0f};

  return Vector3Normalize(Vector3CrossProduct(vVertical, vHorizontal));
}

void TerrainGenerator::Generate(Data::World &world, const Config &config) {
  // 1. Prepare Data
  auto terrain = world.terrain;
  terrain->width = config.width;
  terrain->depth = config.depth;
  terrain->scale = 1.0f;

  terrain->heightMap.resize(config.width * config.depth);
  // Resize and clear river map
  terrain->riverMap.assign(config.width * config.depth, 0);

  Mesh mesh = {0};
  mesh.triangleCount = (config.width - 1) * (config.depth - 1) * 2;
  mesh.vertexCount = mesh.triangleCount * 3;

  mesh.vertices = (float *)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
  mesh.normals = (float *)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
  mesh.colors =
      (unsigned char *)MemAlloc(mesh.vertexCount * 4 * sizeof(unsigned char));

  // Generate Noise Image used for data
  Image noiseImage = GenImagePerlinNoise(
      config.width, config.depth, config.seed, config.seed, config.noiseScale);
  Color *pixels = LoadImageColors(noiseImage);

  // Fill Heightmap
  terrain->baseHeightMap.resize(config.width * config.depth);
  for (int i = 0; i < config.width * config.depth; i++) {
    float h = pixels[i].r / 255.0f;
    terrain->heightMap[i] = h;
    terrain->baseHeightMap[i] = h;
  }

  UnloadImageColors(pixels);
  UnloadImage(noiseImage);

  // Rebuild mesh with new heightmap
  RebuildMesh(terrain.get(), config);
}

void TerrainGenerator::RebuildMesh(Data::Terrain *terrain,
                                   const Config &config) {
  if (terrain->heightMap.empty())
    return;

  if (terrain->isModelLoaded) {
    UnloadModel(terrain->model);
  }

  Mesh mesh = {0};
  mesh.triangleCount = (config.width - 1) * (config.depth - 1) * 2;
  mesh.vertexCount = mesh.triangleCount * 3;

  mesh.vertices = (float *)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
  mesh.normals = (float *)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
  mesh.colors =
      (unsigned char *)MemAlloc(mesh.vertexCount * 4 * sizeof(unsigned char));

  // Build Mesh
  int vCounter = 0;
  for (int z = 0; z < config.depth - 1; z++) {
    for (int x = 0; x < config.width - 1; x++) {

      // Get heights
      float h00 = terrain->GetHeight(x, z);
      float h10 = terrain->GetHeight(x + 1, z);
      float h01 = terrain->GetHeight(x, z + 1);
      float h11 = terrain->GetHeight(x + 1, z + 1);

      // Calculate Normals for each vertex (Smooth Shading)
      Vector3 n00 = GetVertexNormal(terrain, x, z, config.heightMultiplier);
      Vector3 n10 = GetVertexNormal(terrain, x + 1, z, config.heightMultiplier);
      Vector3 n01 = GetVertexNormal(terrain, x, z + 1, config.heightMultiplier);
      Vector3 n11 =
          GetVertexNormal(terrain, x + 1, z + 1, config.heightMultiplier);

      // Triangle 1 (Bottom Left, Top Left, Bottom Right) -> CCW
      // BL (x, z)
      mesh.vertices[vCounter * 3] = (float)x;
      mesh.vertices[vCounter * 3 + 1] = h00 * config.heightMultiplier;
      mesh.vertices[vCounter * 3 + 2] = (float)z;

      int r00 = terrain->GetRiverType(x, z);

      Color c = GetColorForHeight(h00, config.seaLevel, r00);
      mesh.colors[vCounter * 4] = c.r;
      mesh.colors[vCounter * 4 + 1] = c.g;
      mesh.colors[vCounter * 4 + 2] = c.b;
      mesh.colors[vCounter * 4 + 3] = c.a;
      mesh.normals[vCounter * 3] = n00.x;
      mesh.normals[vCounter * 3 + 1] = n00.y;
      mesh.normals[vCounter * 3 + 2] = n00.z;
      vCounter++;

      // TL (x, z+1)
      mesh.vertices[vCounter * 3] = (float)x;
      mesh.vertices[vCounter * 3 + 1] = h01 * config.heightMultiplier;
      mesh.vertices[vCounter * 3 + 2] = (float)(z + 1);

      int r01 = terrain->GetRiverType(x, z + 1);

      c = GetColorForHeight(h01, config.seaLevel, r01);
      mesh.colors[vCounter * 4] = c.r;
      mesh.colors[vCounter * 4 + 1] = c.g;
      mesh.colors[vCounter * 4 + 2] = c.b;
      mesh.colors[vCounter * 4 + 3] = c.a;
      mesh.normals[vCounter * 3] = n01.x;
      mesh.normals[vCounter * 3 + 1] = n01.y;
      mesh.normals[vCounter * 3 + 2] = n01.z;
      vCounter++;

      // BR (x+1, z)
      mesh.vertices[vCounter * 3] = (float)(x + 1);
      mesh.vertices[vCounter * 3 + 1] = h10 * config.heightMultiplier;
      mesh.vertices[vCounter * 3 + 2] = (float)z;

      int r10 = terrain->GetRiverType(x + 1, z);

      c = GetColorForHeight(h10, config.seaLevel, r10);
      mesh.colors[vCounter * 4] = c.r;
      mesh.colors[vCounter * 4 + 1] = c.g;
      mesh.colors[vCounter * 4 + 2] = c.b;
      mesh.colors[vCounter * 4 + 3] = c.a;
      mesh.normals[vCounter * 3] = n10.x;
      mesh.normals[vCounter * 3 + 1] = n10.y;
      mesh.normals[vCounter * 3 + 2] = n10.z;
      vCounter++;

      // Triangle 2 (Top Left, Top Right, Bottom Right) -> CCW
      // TL (x, z+1)
      mesh.vertices[vCounter * 3] = (float)x;
      mesh.vertices[vCounter * 3 + 1] = h01 * config.heightMultiplier;
      mesh.vertices[vCounter * 3 + 2] = (float)(z + 1);

      // r01
      c = GetColorForHeight(h01, config.seaLevel, r01);
      mesh.colors[vCounter * 4] = c.r;
      mesh.colors[vCounter * 4 + 1] = c.g;
      mesh.colors[vCounter * 4 + 2] = c.b;
      mesh.colors[vCounter * 4 + 3] = c.a;
      mesh.normals[vCounter * 3] = n01.x;
      mesh.normals[vCounter * 3 + 1] = n01.y;
      mesh.normals[vCounter * 3 + 2] = n01.z;
      vCounter++;

      // TR (x+1, z+1)
      mesh.vertices[vCounter * 3] = (float)(x + 1);
      mesh.vertices[vCounter * 3 + 1] = h11 * config.heightMultiplier;
      mesh.vertices[vCounter * 3 + 2] = (float)(z + 1);

      int r11 = terrain->GetRiverType(x + 1, z + 1);

      c = GetColorForHeight(h11, config.seaLevel, r11);
      mesh.colors[vCounter * 4] = c.r;
      mesh.colors[vCounter * 4 + 1] = c.g;
      mesh.colors[vCounter * 4 + 2] = c.b;
      mesh.colors[vCounter * 4 + 3] = c.a;
      mesh.normals[vCounter * 3] = n11.x;
      mesh.normals[vCounter * 3 + 1] = n11.y;
      mesh.normals[vCounter * 3 + 2] = n11.z;
      vCounter++;

      // BR (x+1, z)
      mesh.vertices[vCounter * 3] = (float)(x + 1);
      mesh.vertices[vCounter * 3 + 1] = h10 * config.heightMultiplier;
      mesh.vertices[vCounter * 3 + 2] = (float)z;

      // r10
      c = GetColorForHeight(h10, config.seaLevel, r10);
      mesh.colors[vCounter * 4] = c.r;
      mesh.colors[vCounter * 4 + 1] = c.g;
      mesh.colors[vCounter * 4 + 2] = c.b;
      mesh.colors[vCounter * 4 + 3] = c.a;
      mesh.normals[vCounter * 3] = n10.x;
      mesh.normals[vCounter * 3 + 1] = n10.y;
      mesh.normals[vCounter * 3 + 2] = n10.z;
      vCounter++;
    }
  }

  UploadMesh(&mesh, false);
  terrain->model = LoadModelFromMesh(mesh);
  // Default material uses VERTEX_COLOR
  terrain->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
  terrain->isModelLoaded = true;
}
} // namespace Genesis::Generator
