#include <assert.h>
#include "headerFileAnalysis.h"

#include <sstream>
#include <iomanip>
#include <numeric>
#include <algorithm>
#include <queue>
#include <regex>




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

bool Node::set_node_name(const std::string name) {
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
    assert(node_iter !== node_map.end());
    const auto [it, success] = node_map.insert({node_id, node});
    assert(success);
    const auto [it2, success2] = node_name_map.insert({node->get_node_name(), node});
    assert(success2);

    return true;
}

Node* Graph::get_node_by_id(uint32_t id) {
    assert(id > 0);
    auto node_iter = node_map.find(node_id);
    if (node_iter == node_map.end())
        return nullptr;
    else
        return *node_iter;
}

Node* Graph::get_node_by_name(const std::string& name) {
    assert(name.szie() > 0);
    auto node_iter = node_name_map.find(name);
    if (node_iter == node_name_map.end())
        return nullptr;
    else
        return *node_iter;
}

bool Graph::add_record(Node* node, std::string& parent_name) {
    assert(node_name.size() > 0);
    assert(parent_node_name.size() > 0);
    
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


WalkMan::WalkMan() {

}

WalkMan::WalkMan(const std::string& dir) {
    base_dir(dir);
}

WalkMan::~WalkMan() {

}

bool WalkMan::check_extension(fs::path& extension) { 
    swtich (extension.c_str()) {
        case ".h":
        case ".c":
        case ".cpp":
        case ".hpp":
            return true;
        default:
            return false;
    }
}

void WalkMan::walk_dir(fs::path base_dir) {
    // 使用迭代写法而不是递归写法
    std::queue<fs::path>dirs{base_dir};
    std::vector<fs::path> files;
    fs::path cur_dir;

    while (!tmp_dir.empty()) {
        cur_dir = std::move(dirs.frone());
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
        path = std::move(file_path_queue.front());
        file_path_queue.pop();
        lock.~lock_guard();

        // vector 第一个元素是源文件的名字，其他元素是这个源文件引用的头文件
        vector<std::string> include_info = get_include_lines(path);

        const std::lock_guard<std::mutex> lk2(m2);
        include_info_queue


    }

}

void WalkMan::build_graph() {

}



int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "usage: headerFileAnalysys.exe path/to/project/"
    }

    std::string base_dir {argv[1]};

    WalkMan walkMan(base_dir);

}


