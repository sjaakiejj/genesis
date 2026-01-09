#pragma once
#include "../Generator/TerrainGenerator.h"
#include "World.h"
#include <string>
#include <vector>

namespace Genesis::Data {

class Project {
public:
  std::string name = "Untitled";
  std::string path = "";

  // We hold the world state here
  // Note: In a real app we might separate the "Document" model from the
  // "Runtime" world, but for now the Project *is* the container for the World.
  // The World.h struct holds pointers to data, so we might need to be careful
  // about deep copying for Undo/Redo. For now, let's just focus on
  // saving/loading the CONFIGURATION, not the generated mesh data (which can be
  // regenerated).

  struct ConfigSnapshot {
    Genesis::Generator::TerrainGenerator::Config terrain;
    // Add Tensor/Road configs here later
  };

  void Save(const std::string &filepath, const ConfigSnapshot &currentConfig);
  bool Load(const std::string &filepath, ConfigSnapshot &outConfig);

  // History / Undo / Redo
  void PushSnapshot(const ConfigSnapshot &snapshot);
  bool Undo(ConfigSnapshot &outSnapshot);
  bool Redo(ConfigSnapshot &outSnapshot);
  bool CanUndo() const;
  bool CanRedo() const;

private:
  std::vector<ConfigSnapshot> history;
  int historyIndex = -1; // Points to current state

  std::string ToJSON(const ConfigSnapshot &config);
  bool FromJSON(const std::string &data, ConfigSnapshot &outConfig);
};

} // namespace Genesis::Data
