#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <pthread.h>

#include "threadpool.h"

#define CHECK_CHAR_J "-j"
#define CHUNK_SIZE 4096
#define NTHREADS 10
#define MAX_NUM 1024
#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE);} while (0)

//global variables

size_t total_file_size = 0;
char * input_str;

//Result * result_arr = NULL;

// ./nyuenc file.txt file.txt > file2.enc
void read_files(int argc, char const *args[]) {
    //截取所有的文件名
    FILE *fp[argc-1]; //store file pointer
    for (int i = 1; i != argc; i++){
        if(i == 1) {
            fp[i-1] = fopen(args[i], "rt+");
            if (fp[i-1] == NULL) {
                exit(0);
            }
            fseek(fp[i-1],0L,SEEK_END);
        } else {
            fp[i-1] = fopen(args[i], "r");
            if (fp[i-1] == NULL) {  
                exit(0);
            }
            char ch=fgetc(fp[i-1]);
	        while(!feof(fp[i-1])){
		        fprintf(fp[0],"%c",ch);	
		        ch=fgetc(fp[i-1]);
	        } 
        }
    }
    printf("Merged all files\n");
    for (int i = 0 ; i < argc - 1; i++) {
        fclose(fp[i]);
    }
    // args = NULL;
}


void encoding(char * addr, size_t file_size) {
    char chr;
    int index = 0;
    unsigned char count;
    //printf("file size is %ld\n", file_size);
    for (size_t i = 0; i < file_size; i++) {
        chr = addr[i];
        count = 1;
        while(i < file_size - 1 && addr[i] == addr[i + 1] && count < 255) {
            count++;
            i++;
        }
        //res[index]=p[i];
        //printf("%c", chr);
        write(STDOUT_FILENO, &chr, 1);
        //printf("%d\n", count);
        //unsigned char count_char = (unsigned char)count;
        write(STDOUT_FILENO, &count, 1);
        //res[index + 1]=count_char;
        // sprintf(res + index + 1, "%d", count);
        // index += 1 + strlen(res + index + 1);
        index += 2;
    }
    return;
}


char* get_pre_str(char * input_str, int argc_i, int argc, char *argv[]) {
    // get pre_encoding str
    char * addr;
    size_t size;
    struct stat sb;
    //char * input;
    int fd;
    size_t offset = 0;
    
    //printf("argc is %d\n", argc);
    for (int j = 0; argc_i < argc; argc_i++, j++){
        fd = open(argv[argc_i], O_RDONLY);
        /* Get the size of the file. */
        int status = fstat (fd, &sb);
        size = sb.st_size;
        total_file_size += size;
        if (fd == -1) {
            handle_error("open");
        }
        if (status == -1) {       /* To obtain file size */
            handle_error("fstat");
        }
        if (size == 0) {
            handle_error("empty file");
        }
        addr = mmap(0, size, PROT_READ, MAP_SHARED, fd, 0);
        if (addr == MAP_FAILED) {
            handle_error("mmap");
        }
        //strncpy(input_str,addr,size);
        memcpy(input_str + offset, addr, size);
        offset += size;
        munmap(addr, size);
        close(fd);
    }
    //printf("%s\n",input_str);
    return input_str;
    
}


int main(int argc, char *argv[])
{
    
    int num_threads = 1;
    int argc_i = 1;
    int opt = 0;
    //get num_threads
    while ((opt = getopt (argc, argv, "j:")) != -1)
    {
        switch (opt)
        {
            case 'j':
                num_threads = atoi(optarg);
                break;
            case '?':
                break;
        }
    }
    argc_i = optind;

    //printf("argc_i: %d\n", argc_i);
    //sequential or parallel
    if (num_threads == 1) {
        char *input_str = (char *)malloc(MAX_NUM * MAX_NUM * MAX_NUM * sizeof(char));
        input_str = get_pre_str(input_str, argc_i, argc, argv);
        //printf("pre input_str is %s\n", input_str);
        //printf("total file size is %ld\n", total_file_size);
        encoding(input_str, total_file_size);
        free(input_str);
        return 0;
    } else {
    //pthread pool
        //create thread pool
        ThreadPool * pool = NULL;
        if(0 != create_thread_pool(&pool, num_threads)){
        //printf("create_tpool failed!\n");
            return -1;
        }
        // add task into pool
        add_tasks(pool,  argc_i, argc, argv);

        // collect result
        collect_result(pool, total_queue_size);
   
    //free
        if(pool->chunk_arr) {
            free(pool->chunk_arr);
            pool->chunk_arr = NULL;
        }
    // if(result_arr) {
    //     for (int i = 0; i <(int) total_queue_size; i++) {
    //             if (result_arr[i].result_str) {
    //                 free(result_arr[i].result_str);
    //                 result_arr[i].result_str = NULL;
    //             }
    //     }
    //     free(result_arr);
    //     result_arr = NULL;
    // }
    
    //exit(EXIT_SUCCESS);
    }
}

//reference
// https://stackoverflow.com/questions/20460670/reading-a-file-to-string-with-mmap
// https://man7.org/linux/man-pages/man2/mmap.2.html
// https://jameshfisher.com/2020/01/08/run-length-encoding-in-c/
// https://www.cs.cmu.edu/afs/cs/academic/class/15492-f07/www/pthreads.html
// https://stackoverflow.com/questions/46636641/how-does-optind-get-assigned-in-c

