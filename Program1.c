#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <string.h>

#define MAX_WORKERS 10
#define MAX_JOB_TYPES 5


typedef struct {
    int worker_type;
    int process_id;
    int pipe_fd[2];
} Worker;

Worker workers[MAX_WORKERS];
int worker_count = 0;


void worker_process(int worker_type, int read_fd) {
    while (1) {
        int job_duration;
        if (read(read_fd, &job_duration, sizeof(job_duration)) > 0) {
            printf("Worker %d (PID: %d) processing job for %d seconds...\n", worker_type, getpid(), job_duration);
            sleep(job_duration);
            printf("Worker %d (PID: %d) finished job.\n", worker_type, getpid());
        }
    }
}


void create_workers(int worker_type, int count) {
    for (int i = 0; i < count; i++) {
        int pipe_fds[2];
        pipe(pipe_fds);
        
        pid_t pid = fork();
        if (pid == 0) {
            
            close(pipe_fds[1]); 
            worker_process(worker_type, pipe_fds[0]);
            exit(0);
        } else {
            
            close(pipe_fds[0]); 
            workers[worker_count++] = (Worker){worker_type, pid, {pipe_fds[0], pipe_fds[1]}};
        }
    }
}

int main() {
    
    for (int i = 0; i < MAX_JOB_TYPES; i++) {
        int worker_type, count;
        scanf("%d %d", &worker_type, &count);
        create_workers(worker_type, count);
    }
    
    fd_set write_fds;
    int max_fd = 0;
    
    
    while (1) {
        int job_type, job_duration;
        if (scanf("%d %d", &job_type, &job_duration) != 2) break;
        
        
        FD_ZERO(&write_fds);
        for (int i = 0; i < worker_count; i++) {
            if (workers[i].worker_type == job_type) {
                FD_SET(workers[i].pipe_fd[1], &write_fds);
                if (workers[i].pipe_fd[1] > max_fd) {
                    max_fd = workers[i].pipe_fd[1];
                }
            }
        }
        
        
        if (select(max_fd + 1, NULL, &write_fds, NULL, NULL) > 0) {
            for (int i = 0; i < worker_count; i++) {
                if (FD_ISSET(workers[i].pipe_fd[1], &write_fds)) {
                    write(workers[i].pipe_fd[1], &job_duration, sizeof(job_duration));
                    break;
                }
            }
        }
    }
    
    
    for (int i = 0; i < worker_count; i++) {
        close(workers[i].pipe_fd[1]);
        waitpid(workers[i].process_id, NULL, 0);
    }
    
    return 0;
}

