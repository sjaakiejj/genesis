#pragma once

#include "raylib.h"
#include <vector>

namespace Genesis::Generator {

class TensorField {
public:
  TensorField(int width, int height);
  ~TensorField();

  // Initialize the field with noise and some basic rules
  void Generate(int seed);

  // Resize the grid
  void Resize(int width, int height);

  // Get the primary direction at world coordinates
  Vector2 Sample(float x, float z) const;

  // Draw debug lines for the field
  void DrawDebug(float yLevel);

private:
  int m_Width;
  int m_Height;
  float m_Scale = 1.0f; // World units per grid cell

  // We store the angle (in radians) for memory efficiency,
  // or unit vectors. Let's store unit vectors.
  std::vector<Vector2> m_Grid;

  // Helper to get grid index
  int GetIndex(int x, int y) const;
};

} // namespace Genesis::Generator
