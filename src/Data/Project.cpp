#include "Project.h"
#include <fstream>
#include <iostream>
#include <sstream>

namespace Genesis::Data {

void Project::Save(const std::string &filepath,
                   const ConfigSnapshot &currentConfig) {
  std::string json = ToJSON(currentConfig);
  std::ofstream out(filepath);
  out << json;
  out.close();
  this->path = filepath;
}

bool Project::Load(const std::string &filepath, ConfigSnapshot &outConfig) {
  std::ifstream in(filepath);
  if (!in.is_open())
    return false;

  std::stringstream buffer;
  buffer << in.rdbuf();

  if (FromJSON(buffer.str(), outConfig)) {
    this->path = filepath;
    // Clear history on load? Or keep it? Usually clear.
    history.clear();
    historyIndex = -1;
    PushSnapshot(outConfig);
    return true;
  }
  return false;
}

void Project::PushSnapshot(const ConfigSnapshot &snapshot) {
  // If we are not at the end, remove future history
  if (historyIndex < (int)history.size() - 1) {
    history.erase(history.begin() + historyIndex + 1, history.end());
  }
  history.push_back(snapshot);
  historyIndex++;
}

bool Project::Undo(ConfigSnapshot &outSnapshot) {
  if (CanUndo()) {
    historyIndex--;
    outSnapshot = history[historyIndex];
    return true;
  }
  return false;
}

bool Project::Redo(ConfigSnapshot &outSnapshot) {
  if (CanRedo()) {
    historyIndex++;
    outSnapshot = history[historyIndex];
    return true;
  }
  return false;
}

bool Project::CanUndo() const { return historyIndex > 0; }
bool Project::CanRedo() const { return historyIndex < (int)history.size() - 1; }

// Very simple manual JSON serializer for now
std::string Project::ToJSON(const ConfigSnapshot &config) {
  std::stringstream ss;
  ss << "{\n";
  ss << "  \"terrain\": {\n";
  ss << "    \"width\": " << config.terrain.width << ",\n";
  ss << "    \"depth\": " << config.terrain.depth << ",\n";
  ss << "    \"seed\": " << config.terrain.seed << ",\n";
  ss << "    \"noiseScale\": " << config.terrain.noiseScale << ",\n";
  ss << "    \"heightMultiplier\": " << config.terrain.heightMultiplier
     << ",\n";
  ss << "    \"seaLevel\": " << config.terrain.seaLevel << "\n";
  ss << "  }\n";
  ss << "}";
  return ss.str();
}

// Very simple manual JSON parser
// Helper to extract value by key
std::string GetValue(const std::string &data, const std::string &key) {
  size_t pos = data.find("\"" + key + "\"");
  if (pos == std::string::npos)
    return "";

  size_t start = data.find(":", pos) + 1;
  size_t end = data.find_first_of(",}", start);

  return data.substr(start, end - start);
}

bool Project::FromJSON(const std::string &data, ConfigSnapshot &outConfig) {
  try {
    // This is a naive parser but works for our simple flat structure
    std::string widthVal = GetValue(data, "width");
    if (!widthVal.empty())
      outConfig.terrain.width = std::stoi(widthVal);

    std::string depthVal = GetValue(data, "depth");
    if (!depthVal.empty())
      outConfig.terrain.depth = std::stoi(depthVal);

    std::string seedVal = GetValue(data, "seed");
    if (!seedVal.empty())
      outConfig.terrain.seed = std::stoi(seedVal);

    std::string scaleVal = GetValue(data, "noiseScale");
    if (!scaleVal.empty())
      outConfig.terrain.noiseScale = std::stof(scaleVal);

    std::string heightVal = GetValue(data, "heightMultiplier");
    if (!heightVal.empty())
      outConfig.terrain.heightMultiplier = std::stof(heightVal);

    std::string seaVal = GetValue(data, "seaLevel");
    if (!seaVal.empty())
      outConfig.terrain.seaLevel = std::stof(seaVal);

    return true;
  } catch (...) {
    return false;
  }
}

} // namespace Genesis::Data
