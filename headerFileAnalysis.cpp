#include <assert.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <numeric>
#include <algorithm>
#include <queue>
#include <regex>
#include <string.h>

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
}

uint32_t Node::get_node_id() {
    return node_id;
}

bool Node::add_child_node(uint32_t id) {
    assert(id > 0);
    assert(id != node_id);

    child_nodes.push_back(id);
}

uint32_t Node::get_child_node_count() {
    return child_nodes.size();
}



Graph::Graph() :
    node_count(0)
    {}

Graph::~Graph() {}

uint32_t Graph::get_node_count() {
    return node_count;
}

bool Graph::add_node(Node* node) {
    uint32_t node_id = node->get_node_id();
    
    auto node_iter = node_map.find(node_id);
    assert(node_iter != node_map.end());
    const auto [it, success] = node_map.insert({node_id, node});
    assert(success);
    const auto [it2, success2] = node_name_map.insert({node->get_node_name(), node});
    assert(success2);

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

bool Graph::gen_dot_file() {
    
}


WalkMan::WalkMan() :
    flag1(false),
    flag2(false),
    flag3(false)
    {}

WalkMan::WalkMan(const std::string& dir) :
    flag1(false),
    flag2(false),
    flag3(false) {
    base_dir = fs::path(dir);
}

WalkMan::~WalkMan() {

}

bool WalkMan::check_extension(fs::path& extension) {
    const char* c = extension.c_str();
    if (strcmp(c, ".h") == 0
     || strcmp(c, ".c") == 0
     || strcmp(c, ".cpp") == 0
     || strcmp(c, ".hpp") == 0) {
        return true;
    }
        return false;
}

void WalkMan::walk_dir(fs::path base_dir) {
    // 使用迭代写法而不是递归写法
    std::queue<fs::path>dirs;
    dirs.push(base_dir);
    
    std::vector<fs::path> files;
    fs::path cur_dir;

    while (!dirs.empty()) {
        cur_dir = std::move(dirs.front());
        dirs.pop();

        for (auto& entry : fs::directory_iterator{cur_dir}) {
            if (entry.is_regular_file()) {
                if (check_extension(entry.path().extension())) {
                    files.push_back(entry.path());
                }
            } else if (entry.is_directory()) {
                dirs.push(entry.path());
            }
        }

        const std::lock_guard<std::mutex> lk(m1);
        file_path_queue.insert(file_path_queue.end(),
                                std::make_move_iterator(files.begin()),
                                std::make_move_iterator(files.end()));
        files.clear();
    }
    // set finish flag
    flag1 = true;
}


std::vector<std::string>&& WalkMan::get_include_lines(const fs::path& path) {
    // regex expression from:
    // https://stackoverflow.com/questions/26492513/write-c-regular-expression-to-match-a-include-preprocessing-directive
    static std::regex re{"^\\s*#\\s*include\\s+[<\"][^>\"]*[>\"]\\s*"}
    std::vector<std::string> include_info{ path.c_str() };

    std::ifstream is { path.c_str() };
    std::string s;
    for (size_t line{1}; std::getline(is, s); ++line) {
        if (std::regex_search(begin(s), end(s), re)) {
            include_info.emplace_back(std::move(s));
        }
    }
    return include_info;
}


void WalkMan::analysis_file() {
    // 读取一个文件然后分析出头文件引用
    fs::path path;

    while (true) {
        const std::lock_guard<std::mutex> lk(m1);
        if (!file_path_queue.empty()) {
            path = std::move(file_path_queue.front());
            file_path_queue.pop();
        } else {
            if (flag1 == true) {
                flag2 = true;
                break;
            }
                
        }
        lk.~lock_guard();

        // vector 第一个元素是源文件的名字，其他元素是这个源文件引用的头文件
        std::vector<std::string> include_info = get_include_lines(path);

        const std::lock_guard<std::mutex> lk2(m2);
        include_info_queue.emplace(std::move(include_info));
        lk2.~lock_guard();
    }
}

void WalkMan::build_graph() {
    std::vector<std::string> include_vec;

    while (true) {
        const std::lock_guard<std::mutex> lk(m2);
        if (!include_info_queue.empty()) {
            include_vec = std::move(include_info_queue.front());
            include_info_queue.pop();
        } else {
            if (flag2 == true) {
                flag3 = true;
                break;
            }
        }

        lk.~lock_guard();
        
        // 至少要包含源文件自己的文件名
        assert(include_vec.size() >= 1);

        // 1. 添加自身节点
        Node* self_node = make_node(include_vec.at(0));
        g.add_node(self_node);

        // 2. 添加引用关系
        for (int n = include_vec.size(), i = 1; i < n; ++i) {
            g.add_record(self_node, include_vec.at(i));
        }
    }
}

void WalkMan::start() {

}


int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "usage: headerFileAnalysys.exe path/to/project/" << std::endl;
        exit(0);
    }

    std::string base_dir {argv[1]};

    WalkMan walkMan(base_dir);

}


