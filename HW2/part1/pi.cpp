#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>


pthread_mutex_t mutexSum;

// Serial Program (Monte Carlo Method)
// number_in_circle = 0;
// for ( toss = 0; toss < number_of_tosses; toss ++) {
//     x = random double between -1 and 1;
//     y = random double between -1 and 1;
//     distance_squared = x * x + y * y;
//     if ( distance_squared <= 1)
//         number_in_circle++;
// }
// pi_estimate = 4 * number_in_circle /(( double ) number_of_tosses);

//Global variable structure
struct argpp
{
    /* data */
    int start_p;
    int end_p;
    long long int *total_inside;
    long long int *total_samples;
    int NUM_THREADS;
    int NUM_SAMPLES;

};

//Calculate pi

void *monte_carlo_pi(void *arg){

    struct argpp *argu = (struct argpp *)arg;
    int start_p=argu->start_p;
    int end_p=argu->end_p;
    unsigned int seed=1;
    long long int *total_inside=argu->total_inside;
    long long int *total_samples=argu->total_samples;
    long long int local_inside = 0;

    for (int i=start_p;i<end_p;i++){
        double x = (double)rand_r(&seed) / RAND_MAX;
        double y = (double)rand_r(&seed) / RAND_MAX;

        if (x * x + y * y <= 1.0) {
            local_inside++;
    }}
    // Mutex lock
    pthread_mutex_lock(&mutexSum);
    *total_inside += local_inside; // global variable
    pthread_mutex_unlock(&mutexSum);
    // Mutex unlock

    pthread_exit(NULL);

}

// main function

int main(int argc, char** argv){

    int NUM_THREADS=atoi(argv[1]);
    long long int NUM_SAMPLES=atoll(argv[2]);
    int partition=(NUM_SAMPLES/NUM_THREADS);
    // create threads
    pthread_t threads[NUM_THREADS];
    // Create thread arguments
    struct argpp arg[NUM_THREADS];
    // inti mutex lock
    pthread_mutex_init(&mutexSum, NULL);

    // set pthread attribute and let pthread joinable
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    // thread arguments
    // Init the number of res as zero
    long long int *total_inside = (long long int *) malloc(sizeof(*total_inside));
    *total_inside = 0;
    // Tread Calculation
    for (int i = 0; i < NUM_THREADS; i++) {
        arg[i].start_p = partition * i;
        arg[i].end_p = partition * (i+1);
        arg[i].NUM_SAMPLES = NUM_SAMPLES;
        arg[i].NUM_THREADS = NUM_THREADS;
        arg[i].total_inside=total_inside;
        pthread_create(&threads[i], &attr, monte_carlo_pi, (void *)&arg[i]);
    }

    pthread_mutex_destroy(&mutexSum);

    // Join threads
    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join Fail");
            exit(1);
        }
    }

    // Calculate pi
    double pi = 4 * ((*total_inside) / (double)NUM_SAMPLES);
    printf("%lf\n", pi);
    return 0;
}
