#include<stdio.h>
#include<string.h>
#include<stdlib.h>

#include<iostream>
#include<fstream>
#include<sstream>
#include<vector>
#include<cmath>
#include<iomanip>

using std::string;
using std::cout;
using std::endl;
using std::vector;

typedef struct{
    int backoff;
    int R;
    int cnt_collision;
} node;


int main(int argc, char** argv) {
    //printf("Number of arguments: %d", argc);
    if (argc != 2) {
        printf("Usage: ./csma input.txt\n");
        return -1;
    }

    std::ifstream inputfile(argv[1]);
    string fileline;
    int n_nodes;
    int l_pkt;
    int max_collision;
    int max_time;
    
    char place_holder;
    inputfile >> place_holder >> n_nodes;
    vector<node> nodes(n_nodes);
    inputfile >> place_holder >> l_pkt;
    inputfile >> place_holder >> max_collision;
    vector<int> Rs(max_collision);
    inputfile >> place_holder;
    for(int ii_R = 0; ii_R < max_collision; ii_R++){
        inputfile >> Rs[ii_R];
    }
    inputfile >> place_holder >> max_time;
    inputfile.close();
    
    // cout << n_nodes << " " << l_pkt << " " << max_collision << " " <<max_time << endl;

    for(int ii_node = 0; ii_node < n_nodes; ii_node++){
        nodes[ii_node].backoff = (ii_node + 0) % (nodes[ii_node].R = Rs[0]);
        nodes[ii_node].cnt_collision = 0;
    }
       
    int utilized_time = 0;
    int time_tick = 0;
    int temp_id;
    while(time_tick < max_time){
        vector<int> ready_nodes;
        for(int ii_node = 0; ii_node < n_nodes; ii_node++){
            if (nodes[ii_node].backoff == 0){
                ready_nodes.push_back(ii_node);
            }
            // cout << nodes[ii_node].backoff << " ";
        }
        // cout << endl;
        switch (ready_nodes.size()){
            case 0:
                for(int ii_node = 0; ii_node < n_nodes; ii_node++){
                    nodes[ii_node].backoff--;
                }
                time_tick++;
                break;
            case 1:
                time_tick += l_pkt;
                utilized_time += l_pkt;
                temp_id = ready_nodes[0];
                nodes[temp_id].backoff = (temp_id + time_tick) % (nodes[temp_id].R = Rs[nodes[temp_id].cnt_collision = 0]);
                break;
            default:
                time_tick++;
                for(auto idx_it = ready_nodes.begin(); idx_it != ready_nodes.end(); idx_it++){
                    if(++nodes[*idx_it].cnt_collision == max_collision){
                        nodes[*idx_it].cnt_collision = 0;
                    }
                    nodes[*idx_it].R = Rs[nodes[*idx_it].cnt_collision];
                    nodes[*idx_it].backoff = (*idx_it + time_tick) % nodes[*idx_it].R;
                }
                break;
        }
    }

    float utilization_rate = (utilized_time - (time_tick - max_time)) / (float)max_time;
    // cout << utilized_time << " " << time_tick << endl;
    utilization_rate = std::round(utilization_rate * 100) / 100.0;

    std::ofstream outfile("output.txt");
    outfile << std::fixed << std::setprecision(2) << utilization_rate << endl;
    outfile.close();

    return 0;
}

