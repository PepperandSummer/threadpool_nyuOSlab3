
#ifndef _THREADPOOL_H
#define _THREADPOOL_H

#include "threadpool.h"
#include <pthread.h>


// typedef struct Func_args {
//     char *encoding_str;  //encoding str
//     size_t length; //length
//     char *result_str; //result
// }Func_args;

//struct task
typedef struct Result
{
    char *result_str;
    size_t length;
}Result;

typedef struct Task
{
    void * (*function)(void *);
    void * arg;
    
}Task;

typedef struct Chunk
{   size_t id;
    char * encoding_str;  //encoding str
    size_t length; //length
}Chunk;

//thread pool
typedef struct ThreadPool
{
    //task queue
    size_t queue_size; // current number of tasks
    Chunk *chunk_arr;

    //thread
    pthread_t *threadIDs; // worker IDs
    int max_num; // max threads num


    int done_num; // exit threads num
    Result *result_arr;
    
    pthread_mutex_t mutex_pool; // lock pool
    pthread_mutex_t mutex_done; // lock result

    int shutdown; // whether destroy the pool 1-> yes 0-> no
    pthread_cond_t notEmpty; // task queue is empty ?
    pthread_cond_t cond_done;

}ThreadPool;

extern Result *result_arr;
extern Chunk *chunk_arr;
extern size_t total_queue_size;
// extern size_t result_count;
// extern size_t task_count;
// create and initialize the pool
int create_thread_pool(ThreadPool ** pool, int max_num);

// destory the pool
int destroy_thread_pool(ThreadPool * pool);

// add tasks to the pool
void add_tasks(ThreadPool * pool, int argc_i, int argc, char *argv[]);

// consumsers
void* worker(void* arg);

Result parallel_encoding(const Chunk * task);

// collect_result
void collect_result(ThreadPool * pool, size_t total_queue_size);

#endif // _THREADPOOL_H