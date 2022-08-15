#ifndef _GRAPH_H_
#define _GRAPH_H_
#include<stdio.h>
#include<string.h>
#include<stdlib.h>

#include <vector>
#include <tuple>
#include <iostream>
#include <fstream>
#include <sstream>
#include "assert.h"
#include <algorithm>

using std::string;
using std::cout;
using std::endl;

#define NON_EDGE -999
#define LARGEST_MAGIC_NUMBER 0x3FFFFFFF
class Node;
typedef struct{
    Node* node_ptr;
    int edge_cost;
    int distance_cost;
    bool visited_flag;
    int ancestor_id;
    int nexthop_id;
} node_info;

template<class T>std::tuple<int, int> pop_heap(T& heap){
    auto min_it = heap.begin();
    for(auto it = heap.begin(); it != heap.end(); it++){
        if(*it < *min_it){
            min_it = it;
        }
    }
    auto res = *min_it;
    heap.erase(min_it);
    return res;
}
class Node{
public:
    Node(const int id):id(id), size(id), node_vector(std::vector<node_info>()){
        node_info nif;
        for(int ii = 1; ii < id; ii++){
            nif.node_ptr = NULL;
            nif.edge_cost = LARGEST_MAGIC_NUMBER;
            nif.distance_cost = LARGEST_MAGIC_NUMBER;
            nif.visited_flag = false;
            nif.ancestor_id = id;
            nif.nexthop_id = 0;
            node_vector.push_back(nif);
        }
        nif.node_ptr = this;
        nif.edge_cost = 0;
        nif.distance_cost = 0;
        nif.visited_flag = false;
        nif.ancestor_id = id;
        nif.nexthop_id = id;
        node_vector.push_back(nif);
    }
    ~Node(){}
    int get_id(){
        return this->id;
    }
    int get_size(){
        return size;
    }
    std::vector<node_info>& get_vector(){
        return node_vector;
    }
    void set_edge(Node* neighbor, const int cost){
        std::vector<std::tuple<int, int>>::iterator it;
        int neighbor_id = neighbor->get_id();
        int temp_cost = (cost == NON_EDGE) ? LARGEST_MAGIC_NUMBER : cost;
        if(neighbor_id <= size){
            node_vector[neighbor_id - 1].node_ptr = neighbor;
            node_vector[neighbor_id - 1].edge_cost = temp_cost;
            node_vector[neighbor_id - 1].distance_cost = temp_cost;
            node_vector[neighbor_id - 1].visited_flag = false;
            node_vector[neighbor_id - 1].ancestor_id = id;
            node_vector[neighbor_id - 1].nexthop_id = neighbor_id;
        } else {
            node_info nif;
            for(int ii = size + 1; ii < neighbor_id; ii++){
                nif.node_ptr = NULL;
                nif.edge_cost = LARGEST_MAGIC_NUMBER;
                nif.distance_cost = LARGEST_MAGIC_NUMBER;
                nif.visited_flag = false;
                nif.ancestor_id = id;
                nif.nexthop_id = 0;
                node_vector.push_back(nif);
            }
            nif.node_ptr = neighbor;
            nif.edge_cost = temp_cost;
            nif.distance_cost = temp_cost;
            nif.visited_flag = false;
            nif.ancestor_id = id;
            nif.nexthop_id = neighbor_id;
            node_vector.push_back(nif);
            size = neighbor_id;
        }
    }
    void increase_size2(const int new_size){
        if(new_size < size){
            cout << "why new size smaller?" << endl;
            return;
        }
        if(new_size == size){
            return;
        }
        node_info nif;
        for(int ii = size + 1; ii <= new_size; ii++){
            nif.node_ptr = NULL;
            nif.edge_cost = LARGEST_MAGIC_NUMBER;
            nif.distance_cost = LARGEST_MAGIC_NUMBER;
            nif.visited_flag = false;
            nif.ancestor_id = 0;
            nif.nexthop_id = 0;
            node_vector.push_back(nif);
        }
        size = new_size;
    }
    void clear_vector(){
        for(int i = 1; i <= size; i++){
            if(i != id){
                node_vector[i - 1].distance_cost = node_vector[i - 1].edge_cost;
                node_vector[i - 1].visited_flag = false;
                node_vector[i - 1].ancestor_id = id;
                node_vector[i - 1].nexthop_id = i;
            }
        }
    }
    void set_vector_LS(){
        // array of [distance, node_id]
        std::vector<std::tuple<int, int>> heap;
        int temp_dist;
        int temp_id = 1;
        int temp_anc;
        Node* temp_node;
        node_info* temp_nif;
        for(auto it = node_vector.begin(); it != node_vector.end(); it++){
            temp_dist = it->edge_cost;
            heap.push_back(std::make_tuple(temp_dist, temp_id));
            temp_id++;
        }
        //std::make_heap(heap.begin(), heap.end(), std::greater<>{});
        while(!heap.empty()){
            // print();
            // for(auto it = heap.begin(); it != heap.end(); it++){
            //     cout << std::get<0>(*it) << " " << std::get<1>(*it) << endl;
            //     cout << endl;
            // }
            // std::pop_heap(heap.begin(), heap.end(), std::greater<>{});
            auto temp_res = pop_heap(heap);
            temp_dist= std::get<0>(temp_res);
            temp_id = std::get<1>(temp_res);
            if(temp_dist >= LARGEST_MAGIC_NUMBER){
                break;
            }
            temp_nif = &node_vector[temp_id - 1];
            temp_nif->distance_cost = temp_dist;
            temp_nif->visited_flag = true;
            temp_anc = temp_id;
            while(node_vector[temp_anc - 1].ancestor_id != id){
                temp_anc = node_vector[temp_anc - 1].ancestor_id;
            }
            temp_nif->nexthop_id = temp_anc;
            temp_node = temp_nif->node_ptr;
            // cout << temp_nif->node_ptr << " " << temp_id << " " << temp_dist<< endl;
            for(int ii = 0; ii < temp_node->get_size(); ii++){
                node_info* ii_nif = &temp_node->get_vector()[ii];
                if(ii >= size){
                    size = ii + 1;
                    node_info nif;
                    nif.node_ptr = ii_nif->node_ptr;
                    nif.edge_cost = LARGEST_MAGIC_NUMBER;
                    if(ii_nif->edge_cost >= LARGEST_MAGIC_NUMBER){
                        nif.distance_cost = LARGEST_MAGIC_NUMBER;
                        nif.ancestor_id = 0;
                    } else {
                        nif.distance_cost = ii_nif->edge_cost + temp_dist;
                        nif.ancestor_id = temp_id;
                    }
                    nif.visited_flag = false;
                    nif.nexthop_id = 0;
                    node_vector.push_back(nif);
                    heap.push_back(std::make_tuple(nif.distance_cost, ii + 1));
                    // std::push_heap(heap.begin(), heap.end(), std::greater<>{});
                    continue;
                }
                if(ii_nif->edge_cost >= LARGEST_MAGIC_NUMBER){
                    continue;
                }
                if(!node_vector[ii].visited_flag && ii + 1 != temp_id){
                    int new_dist = ii_nif->edge_cost + temp_dist;
                    if(new_dist < node_vector[ii].distance_cost ||
                            (new_dist == node_vector[ii].distance_cost && temp_id < node_vector[ii].ancestor_id)){
                        node_vector[ii].node_ptr = ii_nif->node_ptr;
                        node_vector[ii].distance_cost = new_dist;
                        node_vector[ii].ancestor_id = temp_id;
                        // no handler so...
                        for(auto it2 = heap.begin(); it2 != heap.end(); it2++){
                            if(std::get<1>(*it2) == ii + 1){
                                *it2 = std::make_tuple(new_dist, ii + 1);
                                // std::push_heap(heap.begin(), heap.end(), std::greater<>{});
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    bool set_vector_DV(){
        bool change_flag = false;
        int dest_id = 1;
        int neighbor_id;
        for(auto dest_node = node_vector.begin(); dest_node != node_vector.end(); dest_node++){
            if(dest_id != id){
                auto old_dist = dest_node->distance_cost;
                auto old_nexthop = dest_node->nexthop_id;
                neighbor_id = 0;
                for(auto neighbor_node = node_vector.begin(); neighbor_node != node_vector.end(); neighbor_node++){
                    neighbor_id++;
                    if(neighbor_node->edge_cost < LARGEST_MAGIC_NUMBER && neighbor_id != id){
                        auto& neighbor_dest_nif = neighbor_node->node_ptr->get_vector()[dest_id - 1];
                        auto new_dist = neighbor_node->edge_cost + neighbor_dest_nif.distance_cost;
                        if(new_dist >= LARGEST_MAGIC_NUMBER){
                            continue;
                        }
                        if(new_dist < old_dist || (new_dist == old_dist && neighbor_id < old_nexthop)){
                            change_flag = true;
                            dest_node->node_ptr = neighbor_dest_nif.node_ptr;
                            dest_node->ancestor_id = neighbor_dest_nif.ancestor_id;
                            dest_node->nexthop_id = neighbor_id;
                            dest_node->distance_cost = new_dist;
                        }
                    }
                }
            }
            dest_id++;
        }
        return change_flag;
    }
    void print(){
        cout<<"!id: "<<id<<" size: "<<size <<endl;
        int i = 1;
        for(auto it = node_vector.begin(); it != node_vector.end(); it++){
            cout << i++ << " edge: " << it->edge_cost << " distance: " << it->distance_cost << " nexthop: " << it->nexthop_id << " ancestor: " << it->ancestor_id <<endl;
        }
        cout<<endl;
    }
private:
    int id;
    int size;
    std::vector<node_info> node_vector;
};

class Graph{
public:
    Graph():size(0), nodes(std::vector<Node*>()){}
    ~Graph(){
        for(auto it = nodes.begin(); it != nodes.end(); it++){
            delete *it;
        }  
    }
    int get_size(){
        return size;
    }
    void clear_Vectors(){
        for(auto it = nodes.begin(); it != nodes.end(); it++){
            (*it)->clear_vector();
        }   
    }
    void parseFile(std::ifstream& file){
        string curline;
        while(std::getline(file, curline)){
            parseLineChange(curline);
        }
        for(auto it = nodes.begin(); it != nodes.end(); it++){
            (*it)->increase_size2(size);
        }
    }
    void parseLineChange(const string& str){
        std::size_t found = -1;
        std::size_t found_old_next = 0;
        int id1, id2, cost;
        found_old_next = found + 1;
        found = str.find(' ', found_old_next);
        assert(found != string::npos);
        id1 = atoi(str.substr(found_old_next, found - found_old_next).c_str());
        found_old_next = found + 1;
        found = str.find(' ', found_old_next);
        assert(found != string::npos);
        id2 = atoi(str.substr(found_old_next, found - found_old_next).c_str());
        found_old_next = found + 1;
        cost = atoi(str.substr(found_old_next, str.length() - found_old_next).c_str());
        if(id1 > size){
            for(int ii = size + 1; ii <= id1; ii++){
                nodes.push_back(new Node(ii));
            }
            size = id1;
        }
        if(id2 > size){
            for(int ii = size + 1; ii <= id2; ii++){
                nodes.push_back(new Node(ii));
            }
            size = id2;
        }
        nodes[id1 - 1]->set_edge(nodes[id2 - 1], cost);
        nodes[id2 - 1]->set_edge(nodes[id1 - 1], cost);
        // print();
    }
    void output_LS(std::ofstream& fpOut, std::ifstream& message_file, std::ifstream& changes_file){
        string change_line;
    OUTPUT_LS_HEAD:;
        for(auto node = nodes.begin(); node != nodes.end(); node++){
            (*node)->set_vector_LS();
            int i = 1;
            for(auto info = (*node)->get_vector().begin(); info != (*node)->get_vector().end(); info++){
                if(info->distance_cost < LARGEST_MAGIC_NUMBER){
                    fpOut << i << " " << info->nexthop_id << " " << info->distance_cost << endl;
                }
                i++;
            }
            // fpOut << endl;
        }
        // print();
        output_message(fpOut, message_file);
        if(std::getline(changes_file, change_line)){
            clear_Vectors();
            parseLineChange(change_line);
            // fpOut << endl;
            message_file.clear();
            message_file.seekg(0);
            goto OUTPUT_LS_HEAD;
        }
    }
    void output_DV(std::ofstream& fpOut, std::ifstream& message_file, std::ifstream& changes_file){
        string change_line;
        bool change_flag;
    OUTPUT_LS_HEAD:;
        do{
            change_flag = false;
            for(auto node = nodes.begin(); node != nodes.end(); node++){
                change_flag |= (*node)->set_vector_DV();
            }
        } while(change_flag);
        for(auto node = nodes.begin(); node != nodes.end(); node++){
            int i = 1;
            for(auto info = (*node)->get_vector().begin(); info != (*node)->get_vector().end(); info++){
                if(info->distance_cost < LARGEST_MAGIC_NUMBER){
                    fpOut << i << " " << info->nexthop_id << " " << info->distance_cost << endl;
                }
                i++;
            }
            // fpOut << endl;
        }
        
        // print();
        output_message(fpOut, message_file);
        if(std::getline(changes_file, change_line)){
            clear_Vectors();
            parseLineChange(change_line);
            // fpOut << endl;
            message_file.clear();
            message_file.seekg(0);
            goto OUTPUT_LS_HEAD;
        }     
    }
    void print(){
        for(auto node = nodes.begin(); node != nodes.end(); node++){
            (*node)->print();
        }
    }
    void output_message(std::ofstream& fpOut, std::ifstream& message_file){
        string message_line;
        string message;
        int found, found_old_next;
        int src_id, dest_id;
        int distance;
        int cur_id;
        while(std::getline(message_file, message_line)){
            found = -1;
            found_old_next = 0;
            found = message_line.find(' ', found_old_next);
            if(found == string::npos){
                break;
            }
            src_id = atoi(message_line.substr(found_old_next, found - found_old_next).c_str());
            found_old_next = found + 1;
            found = message_line.find(' ', found_old_next);
            assert(found != string::npos);
            dest_id = atoi(message_line.substr(found_old_next, found - found_old_next).c_str());
            found_old_next = found + 1;
            message = message_line.substr(found_old_next, message_line.length() - found_old_next);
            fpOut << "from " << src_id << " to " << dest_id << " cost ";
            // cout << "from " << src_id << " to " << dest_id << endl;
            if(src_id > size || dest_id > size){
                fpOut << "infinite" << " hops " << "unreachable" << " message " << message << endl;
                continue; 
            }
            if(dest_id > nodes[src_id - 1]->get_size()){
                fpOut << "infinite" << " hops " << "unreachable" << " message " << message << endl;
                continue;
            }
            nodes[src_id - 1]->get_vector()[dest_id - 1].distance_cost;
            distance = nodes[src_id - 1]->get_vector()[dest_id - 1].distance_cost;
            if(distance == LARGEST_MAGIC_NUMBER){
                fpOut << "infinite" << " hops " << "unreachable" << " message " << message << endl;
                continue;
            }
            fpOut << distance << " hops";
            cur_id = src_id;
            // print();
            while(cur_id != dest_id){
                fpOut << " " << cur_id;
                cur_id = nodes[cur_id - 1]->get_vector()[dest_id - 1].nexthop_id;
            }
            fpOut << " message " << message << endl;
        }
    }
private:
    int size;
    std::vector<Node*> nodes;
};

#endif

