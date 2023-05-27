#pragma once

#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;;

class Node {
private:
  uint32_t node_id;
  std::string node_name;
  std::vector<uint32_t> child_nodes;

public:
  Node();
  Node(uint32_t id, const std::string& name);
  ~Node();

  bool set_node_name(const std::string& name);
  const std::string& get_node_name();
  bool set_node_id(uint32_t id);
  uint32_t get_node_id();
  bool add_child_node(uint32_t id);
  uint32_t get_child_node_count();
  std::vector<uint32_t>& get_child_nodes();
};


class Graph {
private:
  uint32_t node_count;
  std::map<uint32_t, Node*> node_map;
  std::map<std::string, Node*> node_name_map;
  // for circle check
  std::set<uint32_t> visited;
  std::set<uint32_t> visiting;

public:
  Graph();
  ~Graph();
  uint32_t get_node_count();
  bool add_node(Node* node);
  Node* get_node_by_id(uint32_t id);
  Node* get_node_by_name(const std::string& name);
  bool add_record(Node* node, std::string& parent_name);
  bool gen_dot_file();
  void check_circle();
  bool check_circle_dfs(Node* node, std::vector<uint32_t>& cirlce_link);
  void print_circle(std::vector<uint32_t>& circle_link);
};

static uint32_t gen_id() {
  static uint32_t id = 0;
  return ++id;
}

static Node* make_node(const std::string& name) {
  assert(name != "");
  uint32_t id = gen_id();
  Node* node = new Node(id, name);
  return node;
}

class WalkMan {
private:
  Graph g;

  fs::path base_dir;
  std::vector<fs::path> source_file_paths;
  std::set<std::string> headers;
  std::vector<std::vector<std::string>> include_infos;

public:
  WalkMan();
  WalkMan(const std::string& dir);
  ~WalkMan();

  void start();
  void walk_dir();
  void init_headers();
  void analysis_file();
  void build_graph();
  bool is_source_code(fs::path path);
  bool is_header_file(fs::path path);
  std::vector<std::string> get_include_lines(const fs::path& path);
};
