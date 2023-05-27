#include <assert.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <numeric>
#include <algorithm>
#include <queue>
#include <regex>
#include <string.h>
#include <stdio.h>
#include <fstream>
#include <thread>
#include <chrono>

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

bool Graph::add_node(Node* node) {
    uint32_t node_id = node->get_node_id();

    auto node_iter = node_map.find(node_id);
    assert(node_iter == node_map.end());
    const auto [it, success] = node_map.insert({node_id, node});
    assert(success);
    const auto [it2, success2] = node_name_map.insert({node->get_node_name(), node});
    if (!success2) {
        std::cout << "node->get_node_name() " << node->get_node_name() << std::endl;
    }
    //assert(success2);

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

WalkMan::WalkMan() :
    flag1(false),
    flag2(false),
    flag3(false),
    analysisd_file_count(0),
    walk_file_count(0)
    {}

WalkMan::WalkMan(const std::string& dir) :
    flag1(false),
    flag2(false),
    flag3(false),
    analysisd_file_count(0),
    walk_file_count(0) {
    base_dir = fs::path(dir);
}

WalkMan::~WalkMan() {

}

bool WalkMan::check_extension(fs::path extension) {
    const char* c = extension.c_str();
    if (strcmp(c, ".h") == 0
     || strcmp(c, ".c") == 0
     || strcmp(c, ".cpp") == 0
     || strcmp(c, ".hpp") == 0) {
        return true;
    }
    return false;
}

void WalkMan::walk_dir() {
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

        walk_file_count += files.size();

        const std::lock_guard<std::mutex> lk(m1);
        for (auto& elem : files) {
            file_path_queue.push(std::move(elem));
        }
        files.clear();
    }
    // set finish flag
    flag1.store(true);
    std::cout << "thread walk_dir exit..." << std::endl;
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
    for (size_t line{1}; std::getline(is, s); ++line) {
        if (std::regex_search(begin(s), end(s), re)) {
            std::smatch matchRes;
            std::regex_search(s, matchRes, re_filename);
            assert(matchRes[1].str().size() > 0);
            include_info.emplace_back(std::move(matchRes[1].str()));
        }
    }
    assert(include_info.size() > 0);
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
            if (flag1.load() == true) {
                flag2.store(true);
                break;
            } else {
                continue;
            }

        }
        lk.~lock_guard();

        // vector 第一个元素是源文件的名字，其他元素是这个源文件引用的头文件
        std::vector<std::string> include_info = get_include_lines(path);
        assert(include_info.size() > 0);

        const std::lock_guard<std::mutex> lk2(m2);
        include_info_queue.push(include_info);
        lk2.~lock_guard();

        analysisd_file_count += 1;
    }
    std::cout << "thread analysis_file exit..." << std::endl;
}

void WalkMan::build_graph() {
    std::vector<std::string> include_vec;

    while (true) {
        const std::lock_guard<std::mutex> lk(m2);
        if (!include_info_queue.empty()) {
            include_vec = include_info_queue.front();
            include_info_queue.pop();
        } else {
            if (flag2.load() == true) {
                flag3.store(true);
                break;
            } else {
                continue;
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
    std::cout << "thread build_graph exit..." << std::endl;
}

void WalkMan::start() {
    std::thread t1(&WalkMan::walk_dir, this);
    std::thread t2(&WalkMan::analysis_file, this);
    std::thread t3(&WalkMan::build_graph, this);

    while (flag3.load() != true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "tick..." << std::endl;
        std::cout << "walk_file_count: " << walk_file_count.load() << std::endl;
        std::cout << "analysisd_file_count: " << analysisd_file_count.load() << std::endl;

        std::string f1 = flag1.load() == true ? "true" : "false";
        std::string f2 = flag2.load() == true ? "true" : "false";
        std::string f3 = flag3.load() == true ? "true" : "false";

        std::cout << "flag1:" << f1 << std::endl;
        std::cout << "flag2:" << f2 << std::endl;
        std::cout << "flag3:" << f3 << std::endl;
    }

    g.gen_dot_file();

    exit(0);
}


int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "usage: headerFileAnalysys.exe path/to/project/" << std::endl;
        exit(0);
    }

    std::string base_dir {argv[1]};

    WalkMan walkMan(base_dir);
    walkMan.start();

    return 0;
}
