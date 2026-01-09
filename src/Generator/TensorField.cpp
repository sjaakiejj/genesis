#include "TensorField.h"
#include "raymath.h"
#include <cmath>

namespace Genesis::Generator {

TensorField::TensorField(int width, int height)
    : m_Width(width), m_Height(height) {
  m_Grid.resize(width * height, {1.0f, 0.0f});
}

TensorField::~TensorField() {}

void TensorField::Resize(int width, int height) {
  m_Width = width;
  m_Height = height;
  m_Grid.resize(width * height);
}

void TensorField::Generate(int seed) {
  // Use Raylib's Image generation for noise
  // We generate an image and sample the colors to get noise values 0..1
  Image noiseImage = GenImagePerlinNoise(m_Width, m_Height, 0, 0, 5.0f);

  Color *pixels = LoadImageColors(noiseImage);

  for (int y = 0; y < m_Height; y++) {
    for (int x = 0; x < m_Width; x++) {
      int index = GetIndex(x, y);

      // Map pixel intensity (0-255) to an angle (0 - PI)
      // Tensor fields usually have 2 axes of symmetry, so 0-PI covers all lines
      float intensity = pixels[index].r / 255.0f;
      float angle = intensity * PI * 2.0f; // Full rotation for now to be safe

      // Convert angle to vector
      m_Grid[index] = {cosf(angle), sinf(angle)};
    }
  }

  UnloadImageColors(pixels);
  UnloadImage(noiseImage);
}

Vector2 TensorField::Sample(float x, float z) const {
  // Simple nearest neighbor or bilinear sampling
  // Mapping world (x, z) to grid coordinates
  // Assuming 1 unit = 1 cell for simplicity now
  int gx = (int)x;
  int gy = (int)z;

  // Clamp
  if (gx < 0)
    gx = 0;
  if (gx >= m_Width)
    gx = m_Width - 1;
  if (gy < 0)
    gy = 0;
  if (gy >= m_Height)
    gy = m_Height - 1;

  return m_Grid[GetIndex(gx, gy)];
}

int TensorField::GetIndex(int x, int y) const { return y * m_Width + x; }

void TensorField::DrawDebug(float yLevel) {
  // Draw a line for every cell
  // Optimize: Draw every Nth cell to avoid clutter
  int step = 2;

  for (int y = 0; y < m_Height; y += step) {
    for (int x = 0; x < m_Width; x += step) {
      Vector2 dir = m_Grid[GetIndex(x, y)];

      Vector3 start = {(float)x, yLevel, (float)y};
      Vector3 end = {(float)x + dir.x * 0.8f, yLevel,
                     (float)y + dir.y * 0.8f}; // Scale line by 0.8

      DrawLine3D(start, end, RED);

      // Draw cross field (perpendicular)
      Vector3 perpEnd = {(float)x + dir.y * 0.5f, yLevel,
                         (float)y - dir.x * 0.5f};
      DrawLine3D(start, perpEnd, BLUE);
    }
  }
}

} // namespace Genesis::Generator
