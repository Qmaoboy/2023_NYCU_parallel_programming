#include "page_rank.h"

#include <stdlib.h>
#include <cmath>
#include <omp.h>
#include <utility>

#include "../common/CycleTimer.h"
#include "../common/graph.h"

// pageRank --
//
// g:           graph to process (see common/graph.h)
// solution:    array of per-vertex vertex scores (length of array is num_nodes(g))
// damping:     page-rank algorithm's damping parameter
// convergence: page-rank algorithm's convergence threshold
//
void pageRank(Graph g, double *solution, double damping, double convergence)
{

  // initialize vertex weights to uniform probability. Double
  // precision scores are used to avoid underflow for large graphs

  int numNodes = num_nodes(g);
  double equal_prob = 1.0 / numNodes;

  double no_sum;
  double g_diff;
  double *inter_score = (double *)malloc(sizeof(double) * g->num_nodes);

  int numOfthread = omp_get_max_threads();
  double thread_noOutgoing_sum[numOfthread][8]; // padding 64 byte to aviod false sharing (Assuming cache line is 64 bytes)
  double thread_global_diff[numOfthread][8];


  #pragma omp parallel for
  for (int i = 0; i < numNodes; ++i)
  {
    solution[i] = equal_prob;
  }
  do {
    no_sum=0.0;
    g_diff=0.0;
  // initialzation
  for (int i =0;i< numOfthread;i++){
    thread_noOutgoing_sum[i][0]=0.0;
    thread_global_diff[i][0]=0.0;
  }

  // score_old[vi] = 1/numNodes;

  #pragma omp parallel
  {
    int thread_id = omp_get_thread_num();
    int n_thrds = omp_get_num_threads();
    #pragma omp for schedule(dynamic,1024)
    for (int i=0;i < numNodes;i++) {
      int numOfoutgoing = outgoing_size(g, i);

      if (numOfoutgoing == 0) {
        thread_noOutgoing_sum[thread_id][0] += solution[i];
      } else {
        inter_score[i] = solution[i] / numOfoutgoing;
      }
    }

  }
  ////
  for (int i = 0;i < numOfthread;i++) {
      no_sum += thread_noOutgoing_sum[i][0];
    }
    no_sum *= damping / numNodes;
  #pragma omp parallel
  {
    int thread_id = omp_get_thread_num();
    int n_thrds = omp_get_num_threads();

    #pragma omp for schedule(dynamic, 1024)
    for (int i = 0;i < numNodes;i++) {
      const Vertex *start = incoming_begin(g, i);
      const Vertex *end = incoming_end(g, i);
      double sum = 0.0, abs_diff;

      for (const Vertex *v = start;v != end;v++) {
        sum += inter_score[*v];
      }

      sum = (damping*sum) + (1.0-damping)/numNodes + no_sum;

      // abs
      abs_diff = solution[i] - sum;
      if (abs_diff < 0) {abs_diff = -abs_diff;} // turn to positive
      thread_global_diff[thread_id][0] += abs_diff;
      solution[i] = sum;
    }
  }
    for (int i = 0;i < numOfthread;i++) { // join all threads
      g_diff += thread_global_diff[i][0];
    }

  }while (g_diff >= convergence); // converged = (global_diff < convergence)

  free(inter_score);
  /*
     For PP students: Implement the page rank algorithm here.  You
     are expected to parallelize the algorithm using openMP.  Your
     solution may need to allocate (and free) temporary arrays.

     Basic page rank pseudocode is provided below to get you started:

     // initialization: see example code above
     score_old[vi] = 1/numNodes;

     while (!converged) {

       // compute score_new[vi] for all nodes vi:
       score_new[vi] = sum over all nodes vj reachable from incoming edges
                          { score_old[vj] / number of edges leaving vj  }
       score_new[vi] = (damping * score_new[vi]) + (1.0-damping) / numNodes;

       score_new[vi] += sum over all nodes v in graph with no outgoing edges
                          { damping * score_old[v] / numNodes }

       // compute how much per-node scores have changed
       // quit once algorithm has converged

       global_diff = sum over all nodes vi { abs(score_new[vi] - score_old[vi]) };
       converged = (global_diff < convergence)
     }

   */
}
