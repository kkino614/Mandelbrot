/************************************************************
* Filename: part2-mandelbrot.c
* Student name: Wang, Michelle Yih-chyan
* Student no.: 3035124441
* Date: Nov 22, 2017
* version: 2
* Development platform: Course VM
* Compilation: gcc part12-Mandelbrot.c -l SDL2 -lm
*************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include "Mandel.h"
#include "draw.h"


typedef struct task {
    int star_row;
    int num_of_rows;
} TASK;

typedef struct message {
	int row_index;
	float rowdata[IMAGE_WIDTH];
} MSG;

//allocate memory to store the pixels
float * pixels;



//create the mutexes and conditional variables, for task_pool manipulations
pthread_mutex_t task_pool_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t task_complete_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t task_pool_free_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t task_pool_full_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t task_complete_cond = PTHREAD_COND_INITIALIZER;

//create task pool variables
//total task number
int task_total_number = 0;
//Rows completed by the threads
int row_complete = 0;
//task already allocated by the main process
int task_allocate = 0;
//Task completed by the threads
int task_complete = 0;
//rows assigned for each task
int task_size = 10;
//task pool length
int task_pool_num = 5;
//number of threads
int num_thread = 5;
//current number of tasks allocated in the pool
int current_num_task = 0;
//pool for tasks
TASK * task_pool;
int pixel_accessing = 0;

void *functionC(void* id_num)
{


	int i;
	int SubSum=0;
    int x, y;
    int * task_thread_complete= (int *) malloc(sizeof (int));
    * task_thread_complete = 0;

	int thread_id = * ( (int *) id_num );

    float difftime;

	//get the assigned range
    struct timespec start_compute, end_compute;
    //printf("Child(%d): Start the computation ... \n", *((int *)id_num) );
	printf("Worker(%d): Start up. Wait for task!\n", thread_id);

    while(1){

        printf("Worker(%d): Start the computation ...\n", thread_id);
        //lock the mutex
        pthread_mutex_lock(&task_pool_mutex);
        clock_gettime(CLOCK_MONOTONIC, &start_compute);

        while(current_num_task == 0){
            pthread_cond_wait(&task_pool_full_cond, &task_pool_mutex);
        }

        //load the task in the task pool
		TASK task_todo;

		task_todo.star_row = task_pool[current_num_task-1].star_row;
        task_todo.num_of_rows = task_pool[current_num_task-1].num_of_rows;

        task_thread_complete++;
        row_complete += task_size;


		//compute the mandelbrot image
        for (y=task_todo.star_row; y<task_todo.star_row + task_todo.num_of_rows; y++) {
            for (x=0; x<IMAGE_WIDTH; x++) {
                //compute a value for each point c (x, y)
                pixels[y*IMAGE_WIDTH+x] = Mandelbrot(x, y);
            }
        }


		current_num_task--;


        //inform the main process time to put the tasks
        pthread_cond_signal(&task_pool_free_cond);
        //unlocak the mutex
		pthread_mutex_unlock(&task_pool_mutex);


        //record the number of task when complete one task

		if(task_todo.star_row <0){
			pthread_exit( (void *) task_thread_complete );
		}




		pthread_mutex_lock(&task_complete_mutex);
		task_complete++;

		if(task_complete==task_total_number)
			pthread_cond_signal(&task_complete_cond);

		pthread_mutex_unlock(&task_complete_mutex);


        //get the ending time of finishing child process
        clock_gettime(CLOCK_MONOTONIC, &end_compute);
        difftime = (end_compute.tv_nsec - start_compute.tv_nsec)/1000000.0 + (end_compute.tv_sec - start_compute.tv_sec)*1000.0;

        //printf("Child(%d) ... completed. Elapse time = %.3f ms\n", (int) getpid(), difftime);
        printf("Worker(%d) : ... completed. Elapse time = %.3f ms\n", thread_id, difftime);
    }

	//

}

int main(int argc, char* args[])
{



	struct timespec start_time, end_time;
	//get the start time
	clock_gettime(CLOCK_MONOTONIC, &start_time);
	pthread_t * threads;

    if(argc!=4){

        printf("Three parameters: [number of workers] pnumber of rows in a task] [number of buffers]\n");
        exit(1);
    }

    num_thread = atoi(args[1]);

    task_size= atoi(args[2]);

    if(IMAGE_HEIGHT % task_size!=0){
        task_total_number = (IMAGE_HEIGHT - IMAGE_HEIGHT%task_size) / task_size;
        task_total_number++;
    }else{
        task_total_number = IMAGE_HEIGHT / task_size;
    }

    task_pool_num = atoi(args[2]);
    task_allocate = 0;

    //printf("%d, %d, %d\n", num_thread, task_total_number, task_pool_num);

	//allocate the spaces for thread and image information
	threads = (pthread_t *) malloc(sizeof(pthread_t) * num_thread);
    pixels = (float *) malloc(sizeof(float) * IMAGE_WIDTH * IMAGE_HEIGHT);
    task_pool = (TASK *) malloc( sizeof(TASK) * task_pool_num);

    int * id;
	id = (int *) malloc (sizeof(int) * num_thread);

    //create threads
	int i=0;
	for(i=0;i<num_thread;i++){
		id[i] = i;
        if( (pthread_create(&threads[i], NULL, functionC, (void *) (&id[i]) ) ) )
            printf("Thread creation failed: \n");

	}

    while(task_allocate < task_total_number){

        TASK task_to_add;
		task_to_add.star_row = task_allocate * task_size + 1;

        //modify the number of rows inside each task
		if(row_complete + task_size > IMAGE_HEIGHT)
            task_to_add.num_of_rows = IMAGE_HEIGHT - task_to_add.star_row + 1;
        else
            task_to_add.num_of_rows = task_size;

        pthread_mutex_lock(&task_pool_mutex);
        //wait for the pool to be not full to allocate tasks
        while(current_num_task == task_pool_num){
            pthread_cond_wait(&task_pool_free_cond, &task_pool_mutex);
        }

        task_pool[current_num_task] = task_to_add;
		//printf("taskpool: %d\n", task_pool[current_num_task].star_row);

        row_complete += task_size;
        //record the current number of task in the task pool
        current_num_task++;

		task_allocate++;
		pthread_cond_signal(&task_pool_full_cond);
        pthread_mutex_unlock(&task_pool_mutex);


    }

    //Wait for all tasks are complete
	pthread_mutex_lock(&task_complete_mutex);

	while(task_complete != task_total_number){
		pthread_cond_wait( &task_complete_cond ,&task_complete_mutex);
	}

	pthread_mutex_unlock(&task_complete_mutex);


    //kill the threads by giving task whose start of row less than 0
    for(i=0;i<num_thread;i++){
        //create the finishing task
		TASK task_finish_signal;
		task_finish_signal.star_row = -1;
		task_finish_signal.num_of_rows = -1;

		pthread_mutex_lock(&task_pool_mutex);

        while(current_num_task == task_pool_num){
            pthread_cond_wait(&task_pool_free_cond, &task_pool_mutex);
        }
        task_pool[current_num_task] = task_finish_signal;
		//printf("taskpool: %d\n", task_pool[current_num_task].star_row);

        current_num_task++;

		pthread_cond_signal(&task_pool_full_cond);
        pthread_mutex_unlock(&task_pool_mutex);


    }

    //wait for the thread to exit
	for(i=0;i<num_thread;i++){
		int * num_complete_by_thread;

		pthread_join(threads[i], (void **) &num_complete_by_thread );
		printf("Worker thread %d has terminated and completed %d task\n", i, *num_complete_by_thread);

	}

    printf("All workders have terminated\n");

	//time recording
	struct rusage usage_self;
	struct timespec now_time;
	getrusage(RUSAGE_SELF, &usage_self);

	//print the time information
	printf("Total time spent by process and its threads in user mode = %.3f ms \n", usage_self.ru_utime.tv_sec * 1000.0+usage_self.ru_utime.tv_usec/1000.0);
	printf("Total time spent by process and its threads in system mode = %.3f ms \n", usage_self.ru_stime.tv_sec * 1000.0+usage_self.ru_stime.tv_usec/1000.0);

	//get the time now and print the total elapse time
	clock_gettime(CLOCK_MONOTONIC, &now_time);
	printf("Total elapse time measured by parent process = %.3f ms\n", (now_time.tv_nsec - start_time.tv_nsec)/1000000.0 + (now_time.tv_sec - start_time.tv_sec)*1000.0);

	printf("Draw the image\n");
	DrawImage(pixels, IMAGE_WIDTH, IMAGE_HEIGHT, "Mandelbrot demo", 3000);


	//Release mutex;
	pthread_mutex_destroy(&task_pool_mutex);
	pthread_mutex_destroy(&task_complete_mutex);
	pthread_cond_destroy(&task_pool_free_cond);
	pthread_cond_destroy(&task_pool_full_cond);
	pthread_cond_destroy(&task_complete_cond);

	//free the resources allocated
	free(pixels);
	free(threads);
	free(id);
	return 0;
}

