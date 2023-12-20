#include "bfs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cstddef>
#include <omp.h>

#include "../common/CycleTimer.h"
#include "../common/graph.h"

#define ROOT_NODE_ID 0
#define NOT_VISITED_MARKER -1

void vertex_set_clear(vertex_set *list)
{
    list->count = 0;
}

void vertex_set_init(vertex_set *list, int count)
{
    list->max_vertices = count;
    list->vertices = (int *)malloc(sizeof(int) * list->max_vertices);
    vertex_set_clear(list);
}

// Take one step of "top-down" BFS.  For each vertex on the frontier,
// follow all outgoing edges, and add all neighboring vertices to the
// new_frontier.
void top_down_step(
    Graph g,
    vertex_set *frontier,
    vertex_set *new_frontier,
    int *distances,int depth)
{
    int numOfthread = omp_get_max_threads();
    int count[numOfthread][16] = {0}; // list to control threading ; padding 64 byte to aviod false sharing (Assuming cache line is 64 bytes)
    #pragma omp parallel
    {
    int th_id = omp_get_thread_num();
    #pragma omp for schedule(dynamic, 1024)
    for (int i = 0; i < frontier->count; i++)
    {

        int node = frontier->vertices[i];

        int start_edge = g->outgoing_starts[node];
        int end_edge = (node == g->num_nodes - 1)
                           ? g->num_edges
                           : g->outgoing_starts[node + 1];

        // attempt to add all neighbors to the new frontier
        for (int neighbor = start_edge; neighbor < end_edge; neighbor++)
        {
            int outgoing = g->outgoing_edges[neighbor];

            if (distances[outgoing] == NOT_VISITED_MARKER)
            {
                distances[outgoing] = depth + 1;
                // int index = new_frontier->count++;
                // new_frontier->vertices[index] = outgoing;
            }
        }
    }

    #pragma omp for
    for (int i = 0;i < g->num_nodes;++i) {
        if (distances[i] == depth+1) {
            count[th_id][0]++;
        }
    }

    int ver_idx = 0;
    for (int i = 0;i < th_id;++i) {
        ver_idx += count[i][0];
    }

    #pragma omp for nowait
    for (int i = 0;i < g->num_nodes;++i) {
        if (distances[i] == depth+1) {
            new_frontier->vertices[ver_idx] = i;
            ver_idx++;
        }
    }
    __sync_fetch_and_add(&new_frontier->count, count[th_id][0]); // join avoid race condition
}}

// void top_down_step_origin(
//     Graph g,
//     vertex_set *frontier,
//     vertex_set *new_frontier,
//     int *distances)
// {
//     for (int i = 0; i < frontier->count; i++)
//     {

//         int node = frontier->vertices[i];

//         int start_edge = g->outgoing_starts[node];
//         int end_edge = (node == g->num_nodes - 1)
//                            ? g->num_edges
//                            : g->outgoing_starts[node + 1];

//         // attempt to add all neighbors to the new frontier
//         for (int neighbor = start_edge; neighbor < end_edge; neighbor++)
//         {
//             int outgoing = g->outgoing_edges[neighbor];

//             if (distances[outgoing] == NOT_VISITED_MARKER)
//             {
//                 distances[outgoing] = distances[node] + 1;
//                 int index = new_frontier->count++;
//                 new_frontier->vertices[index] = outgoing;
//             }
//         }
//     }
// }

// Implements top-down BFS.
//
// Result of execution is that, for each node in the graph, the
// distance to the root is stored in sol.distances.
void bfs_top_down(Graph graph, solution *sol)
{

    vertex_set list1;
    vertex_set list2;
    vertex_set_init(&list1, graph->num_nodes);
    vertex_set_init(&list2, graph->num_nodes);

    vertex_set *frontier = &list1;
    vertex_set *new_frontier = &list2;
    int depth=0;
    // initialize all nodes to NOT_VISITED
    #pragma omp parallel for
    for (int i = 0; i < graph->num_nodes; i++)
        sol->distances[i] = NOT_VISITED_MARKER;

    // setup frontier with the root node
    frontier->vertices[frontier->count++] = ROOT_NODE_ID;
    sol->distances[ROOT_NODE_ID] = 0;

    while (frontier->count != 0)
    {

#ifdef VERBOSE
        double start_time = CycleTimer::currentSeconds();
#endif

        vertex_set_clear(new_frontier);

        top_down_step(graph, frontier, new_frontier, sol->distances,depth);

#ifdef VERBOSE
        double end_time = CycleTimer::currentSeconds();
        printf("frontier=%-10d %.4f sec\n", frontier->count, end_time - start_time);
#endif

        // swap pointers
        vertex_set *tmp = frontier;
        frontier = new_frontier;
        new_frontier = tmp;
        depth++;
    }
     //save memory for very large graph;
    free(list1.vertices);
    free(list2.vertices);

}
void bottom_up_step(Graph g,vertex_set *frontier, vertex_set *new_frontier,int *distances,int depth,int *_changeflag)
{
    *_changeflag=0; // flag Event Terminal purpose
    #pragma omp parallel for schedule(dynamic, 1024)
    for (int i = 0;i < g->num_nodes;i++) {
        if (distances[i] == NOT_VISITED_MARKER) {
            int start_edge = g->incoming_starts[i];
            int end_edge = (i == g->num_nodes - 1) ? g->num_edges : g->incoming_starts[i + 1];

            for (int neighbor = start_edge;neighbor < end_edge;neighbor++) {
                int incoming = g->incoming_edges[neighbor];
                if (frontier->vertices[incoming]) {
                    new_frontier->vertices[i] = 1;
                    distances[i] = depth;
                    *_changeflag = 1;
                    break;
                }
            }
        }
    }
    }

void bottom_up_step_Hybrid(Graph g,vertex_set *frontier, vertex_set *new_frontier,int *distances,int depth)
{
    int numOfthread = omp_get_max_threads();
    int cnt[numOfthread][16] = {0}; // padding 64 byte to aviod false sharing (Assuming cache line is 64 bytes)

    #pragma omp parallel
    {
    int thread_id = omp_get_thread_num();
    Vertex *local_point;

    local_point = (Vertex *) malloc(sizeof(Vertex) * g->num_nodes);

    #pragma omp for schedule(dynamic, 1024) // for all the for loop in this function
    for (int i = 0;i < g->num_nodes;i++) {
        if (distances[i] == NOT_VISITED_MARKER) {
            int start_edge = g->incoming_starts[i];
            int end_edge = (i == g->num_nodes - 1) ? g->num_edges : g->incoming_starts[i + 1];

            for (int neighbor = start_edge;neighbor < end_edge;neighbor++) {
                int incoming = g->incoming_edges[neighbor];
                if (distances[incoming] == depth) {
                    distances[i] = depth + 1;
                    local_point[cnt[thread_id][0]] = i;
                    cnt[thread_id][0]++;
                    break;
                }
            }
        }
    }

    // Hybrid or debugging mode
    int ver_idx = 0;
    for (int i = 0;i < thread_id;++i) {
        ver_idx += cnt[i][0];
    }

    for (int i = 0;i < cnt[thread_id][0];++i) {
        new_frontier->vertices[ver_idx] = local_point[i];
        ver_idx++;
    }

    // global count
    __sync_fetch_and_add(&new_frontier->count, cnt[thread_id][0]);

    free(local_point);

    }
}
void bfs_bottom_up(Graph graph, solution *sol)
{
    // For PP students:
    //
    // You will need to implement the "bottom up" BFS here as
    // described in the handout.
    //
    // As a result of your code's execution, sol.distances should be
    // correctly populated for all nodes in the graph.
    //
    // As was done in the top-down case, you may wish to organize your
    // code by creating subroutine bottom_up_step() that is called in
    // each step of the BFS process.
    vertex_set list1;
    vertex_set list2;
    vertex_set_init(&list1, graph->num_nodes);
    vertex_set_init(&list2, graph->num_nodes);

    vertex_set *frontier = &list1;
    vertex_set *new_frontier = &list2;

    // initialize all nodes to NOT_VISITED
    #pragma omp parallel for
    for (int i = 0; i < graph->num_nodes; i++)
        sol->distances[i] = NOT_VISITED_MARKER;

    // setup frontier with the root node
    frontier->vertices[frontier->count++] = 1;
    sol->distances[ROOT_NODE_ID] = 0;

    int depth=1; // bottom up start from 1
    int _changeflag=1; // for terminal event issue


    // int depth=1; // bottom up start from 1
    // int _changeflag=1; // for terminal event issue

    while (_changeflag)
    {
#ifdef VERBOSE
        double start_time = CycleTimer::currentSeconds();
#endif
        vertex_set_clear(new_frontier);

        bottom_up_step(graph, frontier, new_frontier, sol->distances, depth, &_changeflag);

#ifdef VERBOSE
        double end_time = CycleTimer::currentSeconds();
        printf("frontier=%-10d %.4f sec\n", frontier->count, end_time - start_time);
#endif
      // swap pointers // or use __sync_bool_compare_and_swap
        vertex_set *tmp = frontier;
        frontier = new_frontier;
        new_frontier = tmp;

        // increment the depth of bfs
        depth++;
    }
    free(list1.vertices);
    free(list2.vertices);
}



void bfs_hybrid(Graph graph, solution *sol)
{
    // For PP students:
    //
    // You will need to implement the "hybrid" BFS here as
    // described in the handout.
    vertex_set list1;
    vertex_set list2;
    vertex_set_init(&list1, graph->num_nodes);
    vertex_set_init(&list2, graph->num_nodes);

    vertex_set *frontier = &list1;
    vertex_set *new_frontier = &list2;
    // initialize all nodes to NOT_VISITED
    #pragma omp parallel for
    for (int i = 0; i < graph->num_nodes; i++)
        sol->distances[i] = NOT_VISITED_MARKER;

    int depth=0;
    int _changeflag=1; // not use in hybrid version
    // setup frontier with the root node
    frontier->vertices[frontier->count++] = ROOT_NODE_ID;
    sol->distances[ROOT_NODE_ID] = 0;

    while(frontier-> count !=0)
    {
#ifdef VERBOSE
        double start_time = CycleTimer::currentSeconds();
#endif
    vertex_set_clear(new_frontier);
    int topdown = ((float)frontier->count / graph->num_nodes) < 0.3 ? 1 : 0;
    if (topdown){
        top_down_step(graph, frontier, new_frontier, sol->distances, depth);
    }else{
        bottom_up_step_Hybrid(graph, frontier, new_frontier, sol->distances, depth);
    }
#ifdef VERBOSE
        double end_time = CycleTimer::currentSeconds();
        printf("frontier=%-10d %.4f sec\n", frontier->count, end_time - start_time);
#endif
        vertex_set *tmp = frontier;
        frontier = new_frontier;
        new_frontier = tmp;

        // increment the depth of bfs
        depth++;
    }

    free(list1.vertices);
    free(list2.vertices);
}
