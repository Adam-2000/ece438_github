#include<stdio.h>
#include<string.h>
#include<stdlib.h>

#include <vector>
#include <tuple>
#include <iostream>
#include <fstream>
#include <sstream>
#include "assert.h"

#include "graph.h"

using std::string;
using std::cout;
using std::endl;

#define NON_EDGE -999
#define LARGEST_MAGIC_NUMBER 0x3FFFFFFF

int main(int argc, char** argv) {
    //printf("Number of arguments: %d", argc);
    if (argc != 4) {
        printf("Usage: ./linkstate topofile messagefile changesfile\n");
        return -1;
    }

    std::ifstream topo_file(argv[1]);
    std::ifstream message_file(argv[2]);
    std::ifstream changes_file(argv[3]);
    Graph g;
    g.parseFile(topo_file);
    topo_file.close();
    // g.print();
    std::ofstream outfile("output.txt");
    g.output_LS(outfile, message_file, changes_file);
    outfile.close();
    message_file.close();
    changes_file.close();
    
    return 0;
}

