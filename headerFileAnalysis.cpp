#include <cassert>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <queue>
#include <set>
#include <regex>
#include <fstream>
#include <vector>

#include "headerFileAnalysis.h"

Node::Node() :
  node_id(0),
  node_name("")
{}

Node::Node(uint32_t id, const std::string& name) {
  assert(id > 0);
  assert(name.size() > 0);
  node_id = id;
  node_name = name;
}

Node::~Node() {
  child_nodes.clear();
}

bool Node::set_node_name(const std::string& name) {
  if (name.size() == 0 || name.size() > 100) {
    return false;
  }
  node_name = name;
  return true;
}

const std::string& Node::get_node_name() {
  return node_name;
}

bool Node::set_node_id(uint32_t id) {
  assert(id > 0);
  node_id = id;
  return true;
}

uint32_t Node::get_node_id() {
  return node_id;
}

bool Node::add_child_node(uint32_t id) {
  assert(id > 0);
  assert(id != node_id);

  child_nodes.push_back(id);
  return true;
}

uint32_t Node::get_child_node_count() {
  return child_nodes.size();
}

std::vector<uint32_t>& Node::get_child_nodes() {
  return child_nodes;
}

// ------------------------------------------------------

Graph::Graph() :
  node_count(0)
{}

Graph::~Graph() {}

uint32_t Graph::get_node_count() {
  return node_count;
}

// 项目中不允许出现同名文件
bool Graph::add_node(Node* node) {
  std::string name = node->get_node_name();
  if (node_name_map.count(name) > 0) {
    return false;
  }
  uint32_t node_id = node->get_node_id();
  auto node_iter = node_map.find(node_id);
  assert(node_iter == node_map.end());
  const auto [it, succ] = node_map.insert({node_id, node});
  assert(succ);
  const auto [it2, succ2] = node_name_map.insert({node->get_node_name(), node});
  assert(succ2);

  ++node_count;
  return true;
}

Node* Graph::get_node_by_id(uint32_t id) {
  assert(id > 0);
  auto node_iter = node_map.find(id);
  if (node_iter == node_map.end())
    return nullptr;
  else
    return node_iter->second;
}

Node* Graph::get_node_by_name(const std::string& name) {
  assert(name.size() > 0);
  auto node_iter = node_name_map.find(name);
  if (node_iter == node_name_map.end())
    return nullptr;
  else
    return node_iter->second;
}

bool Graph::add_record(Node* node, std::string& parent_name) {
  assert(node != nullptr);
  assert(parent_name.size() > 0);

  // 1. find parent node. if parent node not exist, create it
  Node* parent_node = get_node_by_name(parent_name);
  if (parent_node == nullptr) {
    parent_node = make_node(parent_name);
    add_node(parent_node);
  }

  // 2. insert node into it's parent node
  parent_node->add_child_node(node->get_node_id());

  return true;
}

void Graph::print_circle(std::vector<uint32_t>& circle_link) {
  for (int i = 0; i < circle_link.size(); i++) {
    uint32_t node_id = circle_link.at(i);
    Node* node = get_node_by_id(node_id);
    if (i == 0) {
      std::cout << node->get_node_name();
    } else {
      std::cout << " -> " << node->get_node_name();
    }
  }
  std::cout << std::endl << std::endl;
}

bool Graph::check_circle_dfs(Node* node, std::vector<uint32_t>& circle_link) {
  visiting.insert(node->get_node_id());
  circle_link.push_back(node->get_node_id());
  std::vector<uint32_t>& childrens = node->get_child_nodes();
  for (uint32_t child_node_id : childrens) {
    assert(node->get_node_id() != child_node_id);
    Node* child_node = get_node_by_id(child_node_id);
    if (visited.count(child_node_id) > 0)
      continue;
    if (visiting.count(child_node_id) > 0) {
      std::cout << "circle include: " << get_node_by_id(child_node_id)->get_node_name() << std::endl;
      print_circle(circle_link);
      circle_link.pop_back();
      visiting.erase(node->get_node_id());
      return true;
    }
    if (check_circle_dfs(child_node, circle_link)) {
      circle_link.pop_back();
      visiting.erase(node->get_node_id());
      return true;
    }
  }
  circle_link.pop_back();
  visiting.erase(node->get_node_id());
  return false;
}

void Graph::check_circle() {
  std::vector<uint32_t> circle_link;
  for (auto& item : node_map) {
    int node_id = item.first;
    Node* node = item.second;
    check_circle_dfs(node, circle_link);
    visited.insert(node_id);
    visiting.clear();
    assert(circle_link.size() == 0);
  }
}

bool Graph::gen_dot_file() {
  FILE* fp = fopen("include_graph.dot", "w");
  assert(fp != nullptr);
  // gen node define
  Node* tmp_node = nullptr;
  // ratio 图像高:宽
  // splines=false; 使用直线绘图
  fprintf(fp, "digraph {\n\n graph [ratio = 0.5];\n");
  fprintf(fp, "// node define\n");
  for (auto& item : node_map) {
    tmp_node = item.second;
    fprintf(fp, "%d[label=\"%s %d\"];\n",
            item.first,
            tmp_node->get_node_name().c_str(),
            tmp_node->get_child_node_count());
  }

  fprintf(fp, "\n\n");

  // gen line
  for (auto& item : node_map) {
    Node* node = item.second;
    uint32_t self_node_id = item.first;

    for (auto& child_id : node->get_child_nodes()) {
      fprintf(fp, "%d -> %d;\n", child_id, self_node_id);
    }
  }
  fprintf(fp, "\n}");
  fclose(fp);
  return true;
}

// ------------------------------------------------------

WalkMan::WalkMan() {}

WalkMan::WalkMan(const std::string& dir)
  : base_dir(fs::path(dir)){}

WalkMan::~WalkMan() {}

bool WalkMan::is_source_code(fs::path extension) {
  const char* ext = extension.c_str();
  if (strcmp(ext, ".h") == 0 ||
      strcmp(ext, ".c") == 0 ||
      strcmp(ext, ".cpp") == 0 ||
      strcmp(ext, ".hpp") == 0) {
    return true;
  }
  return false;
}

bool WalkMan::is_header_file(fs::path extension) {
  const char* ext = extension.c_str();
  if (strcmp(ext, ".h") == 0 || strcmp(ext, ".hpp") == 0) {
    return true;
  }
  return false;
}

void WalkMan::walk_dir() {
  std::queue<fs::path>dirs;
  dirs.push(base_dir);
  fs::path cur_dir;

  while (!dirs.empty()) {
    cur_dir = std::move(dirs.front());
    dirs.pop();
    for (auto& entry : fs::directory_iterator{cur_dir}) {
      if (entry.is_regular_file()) {
	if (is_source_code(entry.path().extension())) {
	  source_file_paths.push_back(entry.path());
	}
      } else if (entry.is_directory()) {
	dirs.push(entry.path());
      }
    }
  }
}

void WalkMan::init_headers() {
  for (auto& file_path : source_file_paths) {
    if (is_header_file(file_path.extension())) {
      headers.insert(file_path.filename().string());
    }
  }
}

std::vector<std::string> WalkMan::get_include_lines(const fs::path& path) {
  // regex expression from:
  // https://stackoverflow.com/questions/26492513/write-c-regular-expression-to-match-a-include-preprocessing-directive
  static std::regex re{"^\\s*#\\s*include\\s+[<\"][^>\"]*[>\"]\\s*"};
  //static std::regex re_filename{"\"(.*?)\"|(<.*?>)"};
  static std::regex re_filename{"[<\"](.*?)[>\"]"};
  std::vector<std::string> include_info{ path.filename() };

  std::ifstream is { path.c_str() };
  std::string s;
  while (std::getline(is, s)) {
    if (std::regex_search(begin(s), end(s), re)) {
      std::smatch matchRes;
      std::regex_search(s, matchRes, re_filename);
      std::string filename = matchRes[1].str();
      assert(filename.size() > 0);
      if (headers.count(filename) == 0) { // filter stdlib headers
	continue;
      }
      include_info.emplace_back(std::move(filename));
    }
  }
  assert(include_info.size() > 0);
  return include_info;
}

void WalkMan::analysis_file() {
  // 读取一个文件然后分析出头文件引用
  for (auto& path : source_file_paths) {
    // vector 第一个元素是源文件的名字，其他元素是这个源文件引用的头文件
    std::vector<std::string> include_info = get_include_lines(path);
    assert(include_info.size() > 0);
    include_infos.push_back(include_info);
  }
}

void WalkMan::build_graph() {
  for (std::vector<std::string>& include_info : include_infos) {
    // 至少要包含源文件自己的文件名
    assert(include_info.size() >= 1);
    // 1. 添加自身节点
    Node* self_node;
    self_node = g.get_node_by_name(include_info.at(0));
    if (self_node == nullptr) {
      self_node = make_node(include_info.at(0));
      g.add_node(self_node);
    }
    // 2. 添加引用关系
    for (int n = include_info.size(), i = 1; i < n; ++i) {
      g.add_record(self_node, include_info.at(i));
    }
  }
}

void WalkMan::start() {
  walk_dir();
  init_headers();
  analysis_file();
  build_graph();

  g.check_circle();
  g.gen_dot_file();

  std::cout << "source files number: " << source_file_paths.size() << std::endl;
  std::cout << "header files number: " << headers.size() << std::endl;
  std::cout << "graph  node  number: " << g.get_node_count() << std::endl;
}


int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cout << "usage: ./headerFileAnalysys path/to/project/" << std::endl;
    exit(0);
  }

  std::string base_dir {argv[1]};

  WalkMan walkMan(base_dir);
  walkMan.start();

  return 0;
}
