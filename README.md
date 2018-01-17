# Mandelbrot

 The Mandelbrot set is a famous example of a fractal in mathematics
 It is obtained from the iterative computation over the quadratic recurrence equation
 Mandelbrot set image is created by determining each sample point c in the complex plane goes to infinity or not


• Part 1 - Convert the sequential program to a multiprocessing program by using fork(), 
signal()/sigaction(), wait() and pipe() system functions to manage and coordinate all processes to 
achieve the task. For the multiprocessing program, we are going to use the Boss-Workers model to 
dynamically distribute the computation tasks to a pool of worker processes. 


• Part 2 – Convert the sequential program to a multithreaded program, which uses producer/consumer 
model for coordination between threads as well as distribution of computation tasks to idle worker 
threads. 
