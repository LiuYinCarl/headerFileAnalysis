#pragma once

#include <vector>
#include <map>
#include <algorithm>
#include <string>
#include <map>
#include <tuple>
#include <mutex>
#include <filesystem>

namespace fs = std::filesystem;

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
    
};



class Graph {
private:
    uint32_t node_count;
    std::map<uint32_t, Node*> node_map;
    std::map<std::string, Node*> node_name_map;

public:
    Graph();
    ~Graph();
    uint32_t get_node_count();
    bool add_node(Node* node);
    Node* get_node_by_id(uint32_t id);
    Node* get_node_by_name(const std::string& name);
    bool add_record(std::string& name, std::string& parent_name);


    bool gen_dot_file();
};

static uint32_t gen_id() {
    static uint32_t id = 0;
    return ++id;
}

static Node* make_node(const std::string& name) {
    uint32_t id = gen_id();
    Node* node = new Node(id, name);
    return node;
}

// ����a ���� b

// �߳� 1 ����Ŀ¼�������е�c++ �ļ��г������������ a
// �߳� 2 �Ӷ��� a ��ȡ���ļ��ľ���·�������������а������ļ���������� b
// �߳� 3 �Ӷ��� b ��ȡ�������ļ���Ϣ�����з���
class WalkMan {
private:
    fs::path base_dir;

    std::mutex m1;
    std::mutex m2;

    std::queue<fs::path> file_path_queue;
    std::queue<std::vector<std::string>> include_info_queue;

    std::regex;

public:
    WalkMan();
    WalkMan(const std::string& dir);
    ~WalkMan();


    // thread 1
    void walk_dir();

    // thread 2
    void analysis_file();

    // thread 3
    void build_graph();

    bool check_extension(fs::path& path);
    std::vector<std::string> get_include_lines(std::string& path);

};
