/************************************************************
* Filename: part1b-mandelbrot.c
* Student name: Wang, Michelle Yih-chyan
* Student no.: 3035124441
* Date: Nov 2, 2017
* version: 1.1
* Development platform: Course VM
* Compilation: gcc part1b-Mandelbrot.c –l SDL2 -lm
*************************************************************/

//Using SDL2 and standard IO
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "Mandel.h"
#include "draw.h"

//create pid_t for every child and pipe value
pid_t pids[10];
int fd[2], fd_t[2];
int num_task_complete;

//task infrastructure
typedef struct task {
    int star_row;
    int num_of_rows;
} TASK;

//message infrastructure
typedef struct message {
	int row_index;
	pid_t child_pid;
	float rowdata[IMAGE_WIDTH];
} MSG;

//signal handler for SIGINT and returns the number of task complete
void sigint_handler(int sig){
    printf("Process %d is interrupted by ^C. Bye Bye\n", getpid());
    exit(num_task_complete);
}

//signal handler for SIGUSR1 for child process
void sigusr1_handler(int sig){
    TASK buffertask;

    //receive task from parent process with task pipe
    if(read(fd_t[0], &buffertask, sizeof(TASK)) == 0){
        close(fd[1]);//Close write end of message pipe
        close(fd_t[0]);//Close read end of task pipe
    }
    else{
        
        struct timespec start_compute, end_compute;

        printf("Child(%ld): Start the computation ... \n", (long)getpid());
        clock_gettime(CLOCK_MONOTONIC, &start_compute);

        int x, y;
        float difftime;
        MSG to_return;
        to_return.child_pid = -1;

        //compute the mandelbrot image
        for (y=0; y<buffertask.num_of_rows; y++) {
            for (x=0; x<IMAGE_WIDTH; x++) {
            //compute a value for each point c (x, y)
            to_return.row_index = buffertask.star_row + y;
            to_return.rowdata[x] = Mandelbrot(x, y + buffertask.star_row);
            
            }
            //if the line is at the last line of the task, append pid to child_pid
            if(y==buffertask.num_of_rows-1)
                to_return.child_pid = getpid();
            //write the line with message pipe
            if (write(fd[1], &to_return, sizeof(MSG)) == -1) {
                printf("Error writing to the pipe");
            }
        }
        //record the number of task when complete one task
        num_task_complete++;


        //get the ending time of finishing child process
        clock_gettime(CLOCK_MONOTONIC, &end_compute);
        difftime = (end_compute.tv_nsec - start_compute.tv_nsec)/1000000.0 + (end_compute.tv_sec - start_compute.tv_sec)*1000.0;
        printf("Child(%ld) ... completed. Elapse time = %.3f ms\n", (long)getpid(), difftime);

    //close(fd_t[0]);
        close(fd[0]);

    }
}

int main( int argc, char* args[] )
{
	//data structure to store the start and end times of the whole program
	struct timespec start_time, end_time;
	//get the start time
	clock_gettime(CLOCK_MONOTONIC, &start_time);

        //check input length, if the number of argument is wrong, program exits
        if(argc!=3){
            printf("Wrong Input Arguments!\n");
            exit(1);
        }

	//data structure to store the start and end times of the computation

	//generate mandelbrot image and store each pixel for later display
	//each pixel is represented as a value in the range of [0,1]

	//store the 2D image as a linear array of pixels (in row-major format)
	float * pixels;

    pixels = (float *) malloc(sizeof(float) * IMAGE_WIDTH * IMAGE_HEIGHT);
	if (pixels == NULL) {
		printf("Out of memory!!\n");
		exit(1);
	}
    //set the signal handler, SIGINT to sigint_handler, SIGUSR1 to sigusr1_handler
    	struct sigaction sia, sua;
    	sigaction(SIGINT, NULL, &sia);
    	sia.sa_handler = sigint_handler;
    	sigaction(SIGINT, &sia, NULL);
    	sigaction(SIGUSR1, NULL, &sua);
    	sua.sa_handler = sigusr1_handler;
    	sigaction(SIGUSR1, &sua, NULL);


	//Obtain the number of child processes the user wants to create
	int num_child = 0;
	int k=0;
	while(args[1][k]!='\0'){
        num_child = num_child * 10 + ( (int) args[1][k] - '0');
        k++;
	}
	
	//Obtain the number of tasks the user wants to create
	k=0;
	int taskSize = 0;
	while(args[2][k]!='\0'){
        taskSize = taskSize * 10 + ( (int) args[2][k] - '0');
        k++;
	}

    	int row_complete = 0;

	int i;
	num_task_complete = 0;

	

	//set up pipe arguments
    pipe(fd);
    pipe(fd_t);

	//create a number of child processes
	for(i=0;i<num_child;i++){
		//Error creating child process
		if((pids[i] = fork()) < 0){
		perror("fork");
		abort();

		}else if(pids[i] ==0){
		//in child
            close(fd[0]);
            //sleep(2);
            close(fd_t[1]);

			printf("Child(%d): Start up. Wait for task!\n", getpid());
			//compute the mandelbrot image
			//keep track of the execution time - we are going to parallelize this part

			while(1){
                sleep(1);
                }
			

		}
	}




    MSG to_receive;

    printf("Start collecting the image lines\n");

    TASK assigntask;
    MSG buffermsg;


    close(fd[1]); //Only receive data
    close(fd_t[0]); //Only write data
    //set the number of rows to process in a task
    assigntask.num_of_rows = taskSize;
    //number of works (lines) equals to the image height
    int num_work= IMAGE_HEIGHT;

    //assign tasks to child processes
    for(i=0;i<num_child && row_complete < IMAGE_HEIGHT;i++){
        assigntask.star_row = i * taskSize;
        kill(pids[i], SIGUSR1);
        if(assigntask.star_row + assigntask.num_of_rows > IMAGE_HEIGHT){
            assigntask.num_of_rows = IMAGE_HEIGHT - assigntask.star_row;
        }

	if(write(fd_t[1], &assigntask, sizeof(TASK)) == -1){
		printf("Error fd_t");
	}
	
	//calculate the rows calculation completed
    	row_complete += taskSize;
    }



    while(read(fd[0], &buffermsg, sizeof(MSG)) !=0 && num_work>0 ){ // -1 for reserving space for terminating null-character
	//when the parent process receive a work, the number of work decreases by 1
        
        num_work --;

	//record the result from pipe to pixels
        for(i=0;i<IMAGE_WIDTH;i++){
            pixels[buffermsg.row_index * IMAGE_WIDTH + i] = buffermsg.rowdata[i];
        }
        if(buffermsg.child_pid!=-1){
            if(row_complete < IMAGE_HEIGHT){
                //the first line of the new task starts from the row complete
                assigntask.star_row = row_complete;
                row_complete += taskSize;
		
		//if the last line of the last task number exceeds the image heigh, adjust
                if(assigntask.star_row + assigntask.num_of_rows > IMAGE_HEIGHT){
                    assigntask.num_of_rows = IMAGE_HEIGHT - assigntask.star_row;
                }
		//assign the task to the idling child
                write(fd_t[1], &assigntask, sizeof(TASK));
            }else{
                buffermsg.row_index = -1;
                close(fd_t[1]);
            }
            kill(buffermsg.child_pid, SIGUSR1);
        }
	if(num_work == 0){
		break;
	}


        //printf("after %d\n", row_complete);
        //sleep(3);
    }

    for(i=0;i<num_child;i++)
        kill(pids[i], SIGINT);

    printf("Done reading\n");
    close(fd[0]);



	struct rusage usage_children, usage_self;
	struct timespec now_time;
	int status, n = num_child;
	pid_t pid;


	//wait for all child process to end
	while(n>0){
		pid = wait(&status);
		//printf("Child with PID %ld exited with status 0x%x.\n", (long)pid, status);
		printf("Child process (%d) terminated and completes %d task\n", pid, WEXITSTATUS(status));
		--n;
	}
	getrusage(RUSAGE_CHILDREN, &usage_children);
	getrusage(RUSAGE_SELF, &usage_self);

	//calculate the processing time by parent process and children process
	printf("All Child processes have complete\n");

	//print the information
	printf("Total time spent by all child processes in user mode = %.3f ms \n", usage_children.ru_utime.tv_sec * 1000.0+usage_children.ru_utime.tv_usec/1000.0);
	printf("Total time spent by all child processes in system mode = %.3f ms \n", usage_children.ru_stime.tv_sec * 1000.0+usage_children.ru_stime.tv_usec/1000.0);
	printf("Total time spent by parent processes in user mode = %.3f ms \n", usage_self.ru_utime.tv_sec * 1000.0+usage_self.ru_utime.tv_usec/1000.0);
	printf("Total time spent by parent processes in system mode = %.3f ms \n", usage_self.ru_stime.tv_sec * 1000.0+usage_self.ru_stime.tv_usec/1000.0);

	//get the time now and print the total elapse time
	clock_gettime(CLOCK_MONOTONIC, &now_time);

	printf("Total elapse time measured by parent process = %.3f ms\n", (now_time.tv_nsec - start_time.tv_nsec)/1000000.0 + (now_time.tv_sec - start_time.tv_sec)*1000.0);

	printf("Draw the image\n");
	//Draw the image by using the SDL2 library
	DrawImage(pixels, IMAGE_WIDTH, IMAGE_HEIGHT, "Mandelbrot demo", 3000);




	return 0;
}

