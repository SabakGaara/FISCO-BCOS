/*
 * @CopyRight:
 * FISCO-BCOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * FISCO-BCOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FISCO-BCOS.  If not, see <http://www.gnu.org/licenses/>
 * (c) 2016-2018 fisco-dev contributors.
 */
/**
 * @brief : DAG(Directed Acyclic Graph) basic implement
 * @author: jimmyshi
 * @date: 2019-1-8
 */


#pragma once
#include "Common.h"
#include <libdevcore/Guards.h>
#include <tbb/concurrent_queue.h>
#include <condition_variable>
#include <cstdint>
#include <queue>
#include <thread>
#include <vector>


#define OUT      1
#define TO_CHECK 2
#define MARKED   4
#define VISITED  8

typedef struct Graph {
    int **adjacency;
    int *adj_size, *adj_capacity;
    char *mark;
    int *v, *x, *y;
    int *dele;
    int size, capacity;
} Graph;

typedef struct Array {
    int *t;
    int size, capacity;
} Array;

Graph *init_graph(int);
int add_vertex(Graph *);
void add_edge(int, int, Graph *);
void extend_graph(Graph *);
void free_graph(Graph *);
int get_degree(int, Graph *);
char are_connected(int, int, Graph *);
int remove_vertex(Graph *graph, int u);
//    Graph *read_graph(char *);

Array *new_array();
void push(int, Array *);
int pop(Array *);
void free_array(Array *);
void conc(Array *, Array *);
int compare_int(const void *, const void *);
void sort_array(Array *);
void write_array(Array *, char *);

// mis.c
Array *mis(Graph *);
Array *core(Graph *);
Array *check(Graph *);
Array *apply_to_components(Graph *);
void vertex_out(Graph *);

// fold.c
char is_folding(int, Graph *);
void fold(int, Graph *);
void add_tild(int, int, int, Graph *);
void unfold(int, int, Graph *);

// mirror.c
void get_mirrors(int, Graph *);
char check_delta(int, int, Graph *);



namespace dev
{
    namespace blockverifier
    {
        using ID = uint32_t;
        using IDs = std::vector<ID>;
        static const ID INVALID_ID = (ID(0) - 1);



        struct Vertex
        {
            std::atomic<ID> inDegree;
            std::vector<ID> outEdge;
        };

        class DAG
        {
            // Just algorithm, not thread safe
        public:
            DAG(){};
            ~DAG();

            // Init DAG basic memory, should call before other function
            // _maxSize is max ID + 1
            void init(ID _maxSize);

            // Add edge between vertex
            void addEdge(ID _f, ID _t);

            // Generate DAG
            void generate();

            // Wait until topLevel is not empty, return INVALID_ID if DAG reach the end
            ID waitPop(bool _needWait = true);
            void verifyInit(ID _size);

            // Consume the top and add new top in top queue (thread safe)
            ID consume(ID _id);

            // Clear all data of this class (thread safe)
            void clear();

        private:
            std::vector<std::shared_ptr<Vertex>> m_vtxs;
            tbb::concurrent_queue<ID> m_topLevel;

            ID m_totalVtxs = 0;
            std::atomic<ID> m_totalConsume;
            Graph *graph;

        private:
            void printVtx(ID _id);
            mutable std::mutex x_topLevel;
            std::condition_variable cv_topLevel;
        };

    }  // namespace blockverifier
}  // namespace dev
