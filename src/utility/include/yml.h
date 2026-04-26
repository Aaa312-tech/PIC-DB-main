#pragma once
#include <yaml-cpp/yaml.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "log.h"

class Yml
{
 public:
  Yml() : _filename(""), _config(YAML::Node()) {}

  Yml(const std::string& _filename) : _filename(_filename)
  {
    OPDB::logger->info("[YML] Loading YAML file: {}", _filename);
    try {
      _config = YAML::LoadFile(_filename);
    } catch (const YAML::Exception& e) {
      OPDB::logger->error(
          "[YML] Error loading YAML file {}: {}", _filename, e.what());
      exit(1);
    }
  }

  void printNode(const YAML::Node& node, int level) const
  {
    std::string indent(level * 2, ' ');
    if (node.IsMap()) {
      for (const auto& it : node) {
        std::cout << indent << it.first.as<std::string>() << ": " << std::endl;
        printNode(it.second, level + 1);
      }
    } else if (node.IsSequence()) {
      for (std::size_t i = 0; i < node.size(); ++i) {
        std::cout << indent << "- ";
        printNode(node[i], level + 1);
      }
    } else if (node.IsScalar()) {
      std::cout << indent << node.as<std::string>() << std::endl;
    }
  }

  void print() const { printNode(_config, 0); }

  YAML::Node getNode(const std::string& path) const
  {
    return getNodeFromPath(_config, path);
  }

  void addNode(const std::string& path, const YAML::Node& value)
  {
    addNodeToPath(_config, path, value);
    saveToFile();
  }

  void updateNode(const std::string& path, const YAML::Node& value)
  {
    updateNodeAtPath(_config, path, value);
    saveToFile();
  }

  void deleteNode(const std::string& path)
  {
    deleteNodeFromPath(_config, path);
    saveToFile();
  }

  bool exists(const std::string& path) const
  {
    YAML::Node node = getNode(path);
    return node && !node.IsNull();
  }

  template <typename T>
  static void read(const YAML::Node& node, const std::string& key, T& var)
  {
    if (node[key]) {
      var = node[key].as<T>();
    }
  }

 private:
  std::string _filename;
  YAML::Node  _config;

  void saveToFile() const
  {
    try {
      std::ofstream fout(_filename);
      fout << _config;
    } catch (const std::exception& e) {
      std::cerr << "Error saving to YAML file: " << e.what() << std::endl;
    }
  }

  YAML::Node getNodeFromPath(const YAML::Node&  node,
                             const std::string& path) const
  {
    std::vector<std::string> keys        = splitPath(path);
    const YAML::Node*        currentNode = &node;

    for (const std::string& key : keys) {
      if (currentNode->IsMap() && (*currentNode)[key]) {
        YAML::Node temp = (*currentNode)[key];
        currentNode     = &temp;
      } else {
        return YAML::Node();
      }
    }

    return *currentNode;
  }

  void addNodeToPath(YAML::Node&        node,
                     const std::string& path,
                     const YAML::Node&  value)
  {
    std::vector<std::string> keys = splitPath(path);
    for (size_t i = 0; i < keys.size() - 1; ++i) {
      if (node.IsMap() && node[keys[i]]) {
        node = node[keys[i]];
      } else {
        node[keys[i]] = YAML::Node();
        node          = node[keys[i]];
      }
    }
    node[keys.back()] = value;
  }

  void updateNodeAtPath(YAML::Node&        node,
                        const std::string& path,
                        const YAML::Node&  value)
  {
    std::vector<std::string> keys = splitPath(path);
    for (size_t i = 0; i < keys.size() - 1; ++i) {
      if (node.IsMap() && node[keys[i]]) {
        node = node[keys[i]];
      } else {
        node[keys[i]] = YAML::Node();
        node          = node[keys[i]];
      }
    }
    node[keys.back()] = value;
  }

  void deleteNodeFromPath(YAML::Node& node, const std::string& path)
  {
    std::vector<std::string> keys = splitPath(path);
    for (size_t i = 0; i < keys.size() - 1; ++i) {
      if (node.IsMap() && node[keys[i]]) {
        node = node[keys[i]];
      } else {
        return;
      }
    }
    node.remove(keys.back());
  }

  std::vector<std::string> splitPath(const std::string& path) const
  {
    std::vector<std::string> keys;
    std::string              key;
    std::stringstream        ss(path);
    while (std::getline(ss, key, '.')) {
      keys.push_back(key);
    }
    return keys;
  }
};
