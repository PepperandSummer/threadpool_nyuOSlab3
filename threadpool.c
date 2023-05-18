#include "threadpool.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <pthread.h>

#define CHUNK_SIZE 4096
#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE);} while (0)

size_t chunk_index = 0;
size_t total_queue_size = 0;

//Chunk *chunk_arr = NULL;
//Result *result_arr = NULL;


// parallel_encoding
Result parallel_encoding(const Chunk * task) {
    
    //printf("Worker encoding str is %s\n",task->encoding_str);
    char *encoded = (char *)malloc(task->length * 2);
    size_t n = task->length; //length
    char chr;
    unsigned char count;
    size_t index = 0; //encoded index
    for (size_t i = 0; i < n; i++) {
        chr = task->encoding_str[i];
        count = 1;
        while(i < n - 1 && task->encoding_str[i] == task->encoding_str[i + 1]) {
            count++;
            i++;
        }
        //printf("%c", chr);
        encoded[index++] = chr;
        encoded[index++] = count;
    }
    Result result;
    result.length = index;
    result.result_str = encoded;
    return result;
}

// create thread pool
int create_thread_pool(ThreadPool ** pool, int max) {
    //create thread pool
    (* pool) = (ThreadPool *)malloc(sizeof(ThreadPool));
    do {
        if (* pool == NULL) {
            //printf("malloc threadpool fail...\n");
            break;
        }

        (* pool)->threadIDs = (pthread_t *)malloc(sizeof(pthread_t) * max);
        if ((* pool)->threadIDs == NULL) {
            //printf("malloc threadIDs fail...\n");
            break;
        }
        memset((* pool)->threadIDs, 0, sizeof(pthread_t) * max);
        (* pool)->max_num = max;
        
        if( pthread_mutex_init(&(* pool)->mutex_pool, NULL) != 0 ||
            pthread_cond_init(&(* pool)->notEmpty, NULL) != 0 ||
            pthread_cond_init(&(* pool)->cond_done, NULL) != 0 ||
            pthread_mutex_init(&(* pool)->mutex_done, NULL) != 0) 
        {
            //printf("mutex or condition init fail... \n");
            break;
        }
        (* pool)->chunk_arr = (Chunk *)malloc(sizeof(Chunk));
        (* pool)->result_arr = (Result *)malloc(sizeof(Result));

        //create task queue
        (* pool)->queue_size = 0;
        (* pool)->shutdown = 0;
        (* pool)->done_num = 0;


        //create threads
        for (int i = 0; i < max; i++) {
            if (pthread_create(&(* pool)->threadIDs[i], NULL, worker, (void*)(* pool)) != 0) {
                //printf("Error creating thread %d\n", i);
                exit(1);
            } else {
                //printf("Thread %d created successfully\n", i);
            }
        }
        return 0;
    } while(0);
    
    //free memory
    if((* pool) && (* pool)->threadIDs) {
        free((* pool)->threadIDs);
        (* pool)->threadIDs = NULL;
    }
    if((* pool)) {
        free((* pool));
        (* pool) = NULL;
    }
    return 0;
}

//consumer: take a task
void * worker(void* arg) {
    ThreadPool* pool = (ThreadPool *)arg;
    //pthread_t tid = pthread_self();
    Chunk task;
    Result result;
    while(1) {
        
        pthread_mutex_lock(&pool->mutex_pool);
        // if task queue is empty and pool is open, waiting for tasks
        while (pool->queue_size == 0) {
            // block worker
            pthread_cond_wait(&pool->notEmpty, &pool->mutex_pool); // prevent deadlock
        }
        task = pool->chunk_arr[chunk_index++];
        pool->queue_size --;
        pthread_mutex_unlock(&pool->mutex_pool);

        //encoding
        result = parallel_encoding(&task);
        
        //notify collection
        pthread_mutex_lock(&pool->mutex_done);
        pool->result_arr[task.id] = result;
        pool->done_num++;
        pthread_cond_signal(&pool->cond_done);
        pthread_mutex_unlock(&pool->mutex_done);
        
    }

    return NULL;
}


// producer: put tasks
void add_tasks(ThreadPool * pool, int argc_i, int argc, char *argv[]) {

    char **addr = malloc((argc - argc_i) * sizeof(char) * CHUNK_SIZE);
    //int j = 0; //addr index
    while (argc_i < argc) {
        size_t size;
        struct stat sb;
        int fd;

        fd = open(argv[argc_i], O_RDONLY);
        int status = fstat (fd, &sb);
        size = sb.st_size;
        if (fd == -1) {
            handle_error("open");
        }
        if (status == -1) {
            handle_error("fstat");
        }
        if (size == 0) {
            handle_error("empty file");
        }
        addr[argc_i] = (char *) mmap (NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (addr[argc_i] == MAP_FAILED) {
            handle_error("mmap");
        }

        size_t chunk_num = (size + CHUNK_SIZE - 1) / CHUNK_SIZE;

        pthread_mutex_lock(&pool->mutex_pool);
        pool->chunk_arr = realloc(pool->chunk_arr, (total_queue_size + chunk_num) * sizeof(Chunk));
        pthread_mutex_unlock(&pool->mutex_pool);

        pthread_mutex_lock(&pool->mutex_done);
        pool->result_arr = realloc(pool->result_arr, (total_queue_size + chunk_num) * sizeof(Result));
        pthread_mutex_unlock(&pool->mutex_done);

        size_t chunk_start;
        size_t chunk_end;
        size_t chunk_length = 0;
        // adding chunks to queue
        
        for (size_t i = 0; i < chunk_num; i++) {
        // a chunk
            Chunk chunk;
            chunk_start = i * CHUNK_SIZE;
            chunk_end = chunk_start + CHUNK_SIZE - 1;
            if (size <= chunk_end) {
                chunk_end = size - 1;
            }
            chunk_length = chunk_end - chunk_start + 1;
            
            chunk.id = total_queue_size + i;
            chunk.encoding_str = addr[argc_i] + chunk_start;
            chunk.length = chunk_length;
            pthread_mutex_lock(&pool->mutex_pool);
            //adding chunk into the pool taskQ
            pool->chunk_arr[total_queue_size + i] = chunk;
            pool->queue_size++;
            //printf("Added: chunk_arr[%ld]->encoding_str is %s\n",i + total, taskQ->encoding_str);
            //printf("task is added...\n");
            pthread_cond_signal(&pool->notEmpty);
            pthread_mutex_unlock(&pool->mutex_pool);
        }
        total_queue_size += chunk_num;
        close(fd);
        argc_i++;
    }
    
    for (int i = 0; i < argc - argc_i; i++) {
        free(addr[i]);
        }
        free(addr);
    
    //free(addr);
}


//Main collect result
void collect_result(ThreadPool * pool, size_t total) {
    char last_chr = 0;
    unsigned char last_num = 0;
    char first_chr;
    unsigned char first_num;
    //printf("in collect total is %ld \n", total);
    
    for(int index = 0; index < (int)total; index++) {
        pthread_mutex_lock(&pool->mutex_done);
        while (pool->result_arr[index].result_str == NULL || pool->done_num == 0) {
            //printf("result count is %ld\n", result_count);
            if(pool->result_arr[index].result_str == NULL)
            pthread_cond_wait(&pool->cond_done, &pool->mutex_done);
        }
        pthread_mutex_unlock(&pool->mutex_done);
        // point to the result
        Result *p_result = &pool->result_arr[index];
        size_t len = p_result->length;
        
        //remove duplicates 
        if(len > 1){ //get it
            //printf("%s\n",p_result->result_str);
            if(index == 0) {
                // write 0 - n-2
                pool->result_arr[index].length= pool->result_arr[index].length-2;
                write(STDOUT_FILENO, p_result->result_str, pool->result_arr[index].length);
                last_chr = p_result->result_str[len-2];
                last_num = p_result->result_str[len-1];
                
            }
            else if(index > 0){
                first_chr = p_result->result_str[0];
                first_num = p_result->result_str[1];
                if (first_chr == last_chr)  {
                    //unsigned new_num = last_num + first_num; //update num of first chr of first chunk
                    //printf("---if first_chr = last_chr\n");
                    pool->result_arr[index].result_str[1] = last_num + first_num;
                    p_result->length= pool->result_arr[index].length-2;  // update length of first chunk
                    //printf("----------------\n");
                    write(STDOUT_FILENO, &pool->result_arr[index].result_str[0], p_result->length);
                } else {
                    write(STDOUT_FILENO, &last_chr, 1);
                    write(STDOUT_FILENO, &last_num, 1); //end the last chunk
                    p_result->length= pool->result_arr[index].length-2;
                    write(STDOUT_FILENO, p_result->result_str, p_result->length);
                }
                //printf("----------------\n");
                last_chr = p_result->result_str[len-2]; //get current chunk last chr
                last_num = p_result->result_str[len-1]; //get current chunk last num
            }    
        }
        
    }
    write(STDOUT_FILENO, &last_chr, 1);
    write(STDOUT_FILENO, &last_num, 1);
} 

int destroy_thread_pool(ThreadPool * pool){
    // if (pool == NULL) {
    //     return -1;
    // }

    // //shut down pool
    // pthread_mutex_lock(&pool->mutex_pool);
    // if (pool->shutdown != 1) {pool->shutdown = 1;}
    // pthread_mutex_unlock(&pool->mutex_pool);

    // pthread_cond_broadcast(&pool->notEmpty);
    
    if (pool->threadIDs) {
        free(pool->threadIDs);
        pool->threadIDs = NULL;
    }
    
    pthread_mutex_destroy(&pool->mutex_pool);
    pthread_mutex_destroy(&pool->mutex_done);
    pthread_cond_destroy(&pool->notEmpty);
    pthread_cond_destroy(&pool->cond_done);
    
    free(pool);
    pool = NULL;
    return 0;
}