/************************************************************
* Filename: part1a-mandelbrot.c
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

typedef struct message {
	int row_index;
	float rowdata[IMAGE_WIDTH];
} MSG;

int main( int argc, char* args[] )
{
	//data structure to store the start and end times of the whole program
	struct timespec start_time, end_time;
	//get the start time
	clock_gettime(CLOCK_MONOTONIC, &start_time);

	//data structure to store the start and end times of the computation
	struct timespec start_compute, end_compute;

	//generate mandelbrot image and store each pixel for later display
	//each pixel is represented as a value in the range of [0,1]

	//store the 2D image as a linear array of pixels (in row-major format)
	float * pixels;

    pixels = (float *) malloc(sizeof(float) * IMAGE_WIDTH * IMAGE_HEIGHT);
	if (pixels == NULL) {
		printf("Out of memory!!\n");
		exit(1);
	}

    int fd[2];

	//Obtain the number of child processes the user wants to create
	
    int num_child = 0;
	int k=0;
	while(args[1][k]!='\0'){
        num_child = num_child * 10 + ( (int) args[1][k] - '0');
        k++;
	}

	int part_image_height = IMAGE_HEIGHT / num_child;
	int to_cal[11]={0};


	int i;
	//calculate the number of task each child should complete
	for(i=1;i<=num_child;i++){
        to_cal[i] = part_image_height * i;
        if(i==num_child)
            to_cal[i] = IMAGE_HEIGHT + 1;
	}


	//open the pipe arguments
	pid_t pids[10];
    pipe(fd);


	for(i=0;i<num_child;i++){
		if((pids[i] = fork()) < 0){
		perror("fork");
		abort();

		}else if(pids[i] ==0){
		//in child
            close(fd[0]);
            //sleep(2);

			//compute the mandelbrot image
			//keep track of the execution time - we are going to parallelize this part
			printf("Child(%ld): Start the computation ...\n", (long)getpid());
			clock_gettime(CLOCK_MONOTONIC, &start_compute);

			int x, y;
			float difftime;
			MSG to_return;

		  	for (y=to_cal[i]; y<to_cal[i+1]; y++) {
		    	for (x=0; x<IMAGE_WIDTH; x++) {
				//compute a value for each point c (x, y)

				to_return.row_index = y;
                to_return.rowdata[x] = Mandelbrot(x, y);
				//Mandelbrot(x,y);
				 //Only send data
                


		    	}
		    	if (write(fd[1], &to_return, sizeof(MSG)) == -1) {
                    printf("Error writing to the pipe");
                }
		   	}
		   	close(fd[1]);//Close write end

            //get the ending time of finishing child process
			clock_gettime(CLOCK_MONOTONIC, &end_compute);
			difftime = (end_compute.tv_nsec - start_compute.tv_nsec)/1000000.0 + (end_compute.tv_sec - start_compute.tv_sec)*1000.0;
			printf("Child(%ld) ... completed. Elapse time = %.3f ms\n", (long)getpid(), difftime);

			//Report timing
			clock_gettime(CLOCK_MONOTONIC, &end_time);
			difftime = (end_time.tv_nsec - start_time.tv_nsec)/1000000.0 + (end_time.tv_sec - start_time.tv_sec)*1000.0;
			printf("Total elapse time measured by the process = %.3f ms\n", difftime);

			exit(0);

		}
	}

    MSG to_receive;
    printf("This is parent process.\n");

    close(fd[1]); //Only receive data
    //char readed[500] = "";
    while(read(fd[0], &to_receive, sizeof(MSG)) != 0) { // -1 for reserving space for terminating null-character
        //printf("Line %d: \n", to_receive.row_index);
        for(i=0;i<IMAGE_WIDTH;i++){
            pixels[to_receive.row_index * IMAGE_WIDTH + i] = to_receive.rowdata[i];
        }
    }
    printf("Done reading");
    close(fd[0]);


	struct rusage usage_children, usage_self;
	struct timespec now_time;
	int status, n = num_child;
	pid_t pid;

	//wait for all child process to end
	while(n>0){
		pid = wait(&status);
		//printf("Child with PID %ld exited with status 0x%x.\n", (long)pid, status);
		--n;
	}

	//calculate the processing time by parent process and children process
	printf("All Child processes have complete\n");
	getrusage(RUSAGE_CHILDREN, &usage_children);
	getrusage(RUSAGE_SELF, &usage_self);

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

