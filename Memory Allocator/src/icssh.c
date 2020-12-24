
#include "icssh.h"
#include <readline/readline.h>
#include "helpers.h"

void sigchld_handler(int sig);

int main(int argc, char* argv[]) {
	int exec_result;
	int exit_status;
	pid_t pid;
	pid_t wait_result;
	char* line;
#ifndef DEBUG
	rl_outstream = fopen("/dev/null", "w");
#endif
	// Setup segmentation fault handler
	if (signal(SIGSEGV, sigsegv_handler) == SIG_ERR) {
		perror("Failed to set signal handler");
		exit(EXIT_FAILURE);
	}

	// Setup child signal handler
	if(signal(SIGCHLD, sigchld_handler) == SIG_ERR){
		perror("Failed to set signal handler");
		exit(EXIT_FAILURE);
	}

	node_t *listHead = NULL;
	List_t bgList;
	bgList.length = 0;
	bgList.head = listHead;
	bgList.comparator = compareBgentry;

	int listPos;
	pid_t tcpid;

    // print the prompt & wait for the user to enter commands string
	while ((line = readline(SHELL_PROMPT)) != NULL) {
        // MAGIC HAPPENS! Command string is parsed into a job struct
        // Will print out error message if command string is invalid
		job_info* job = validate_input(line);
        if (job == NULL) { // Command was empty string or invalid
			free(line);
			continue;
		}

        //Prints out the job linked list struture for debugging
        debug_print_job(job);		
		
		/* Reap child processes and print out BG_TERM statement. */
		if(deadChldAlarm && bgList.length > 0){
			listPos = 0;
			bgentry_t * temp;
			listHead = bgList.head;
			while(listHead){
				temp = (bgentry_t *)listHead->value;
				if(waitpid(temp->pid, NULL, WNOHANG) < 0){
					perror("wait error");
				}
				else if(waitpid(temp->pid, NULL, WNOHANG) == 0){
					listPos++;
					listHead = listHead->next;
				}
				else{
					fprintf(stdout, BG_TERM, temp->pid, temp->job->line);
					removeByIndex(&bgList, listPos);
					listHead = listHead->next;
				}
			}
			deadChldAlarm = 0;
		}


		// example built-in: exit
		if (strcmp(job->procs->cmd, "exit") == 0) {
			if(bgList.length > 0){
				listHead = bgList.head;
				bgentry_t * temp;
				while(listHead){
					 temp = (bgentry_t *)listHead->value;
					 fprintf(stdout, BG_TERM, temp->pid, temp->job->line);
					 kill(temp->pid, SIGKILL);
					 waitpid(temp->pid, &exit_status, 0);
					 listHead = listHead->next;
				}
			}
			// Terminating the shell
			free(line);
			free_job(job);
            validate_input(NULL);
            return 0;
		}

		// cd Command
		if (strcmp(job->procs->cmd, "cd") == 0) {
			char * buffer =  calloc(PATH_MAX,1);
			// If there is no specified directory path after command cd
			if(job->procs->argc == 1){
				chdir(getenv("HOME"));
				printf("%s\n", getcwd(buffer, PATH_MAX));
				free(buffer);
				continue;
			}
			// Check if specified dir path is valid
			if(chdir(job->procs->argv[1]) < 0){
				fprintf(stderr, DIR_ERR);
			}
			else{
				printf("%s\n", getcwd(buffer, PATH_MAX));
			}
			free(buffer);
			continue;
		}

		if (strcmp(job->procs->cmd, "estatus") == 0){
			if(WIFEXITED(exit_status))
				printf("%d\n", WEXITSTATUS(exit_status));
			continue;
		}

		if (strcmp(job->procs->cmd, "bglist") == 0){
			listHead = bgList.head;
			while(listHead){
				bgentry_t *p1 = (bgentry_t *)listHead->value;
				print_bgentry(p1);
				listHead = listHead->next;
			}
			continue;
		}

		// example of good error handling!
		if ((pid = fork()) < 0) {
			perror("fork error");
			exit(EXIT_FAILURE);
		}
		/* Child */
		if (pid == 0) {  //If zero, then it's the child process
			/* Redirection */
			redirectFds(job);

			/* Begin Piping */
			if(job->nproc > 1){
				
				int fd1[2];
				int fd2[2];
				if(pipe(fd1) < 0){
					perror("pipe error");
					exit(EXIT_FAILURE);
				}
				if(pipe(fd2) < 0){
					perror("pipe error");
					exit(EXIT_FAILURE);
				}

				/* First Grandchild */
				if((pid = fork()) < 0){
					perror("fork error");
					exit(EXIT_FAILURE);
				}
				if(pid == 0){
					close(fd1[0]);
					dup2(fd1[1], fileno(stdout));
					close(fd1[1]);
					close(fd2[0]);
					close(fd2[1]);
					exec_result = execvp(job->procs->cmd, job->procs->argv);
					if (exec_result < 0) {  //Error checking
						printf(EXEC_ERR, job->procs->cmd);
						exit(EXIT_FAILURE);
					}
					exit(EXIT_SUCCESS);
				}
				job->procs = job->procs->next_proc;

				/* Second Grandchild */
				if(job->nproc == 3){
					if((pid = fork()) < 0){
						perror("fork error");
						exit(EXIT_FAILURE);
					}
					if(pid == 0){
						dup2(fd1[0], fileno(stdin));
						dup2(fd2[1], fileno(stdout));
						close(fd1[0]);
						close(fd1[1]);
						close(fd2[0]);
						close(fd2[1]);
						exec_result = execvp(job->procs->cmd, job->procs->argv);
						if (exec_result < 0) {  //Error checking
							printf(EXEC_ERR, job->procs->cmd);
							exit(EXIT_FAILURE);
						}
						exit(EXIT_SUCCESS);
					}
					job->procs = job->procs->next_proc;
				}
							

				/* Child */
				if(job->nproc == 2)
					dup2(fd1[0], fileno(stdin));
				else if(job->nproc ==3)
					dup2(fd2[0], fileno(stdin));
				close(fd1[0]);
				close(fd1[1]);
				close(fd2[0]);
				close(fd2[1]);

				int i = 0;
				// for(;i < job->nproc; i++){
				// 	wait(&exit_status);
				// }
			}
			
			/* Child continued */
			/* When this process dies, pipe fds would automatically close */
			proc_info* proc = job->procs;
			exec_result = execvp(proc->cmd, proc->argv);
			if (exec_result < 0) {  //Error checking
				printf(EXEC_ERR, proc->cmd);
				exit(EXIT_FAILURE);
			}
			exit(EXIT_SUCCESS);
		}
		
		/* Parent */ 
		else {
			
			// As the parent, wait for the foreground job to finish
			if(job->bg == 1){
            	bgentry_t *bgInfo = malloc(sizeof(bgentry_t));
            	// job_info *copy = malloc(sizeof(job_info));
            	// copyJob(copy, job);
            	bgInfo->job = job;
            	bgInfo->pid = pid;
            	bgInfo->seconds = time(NULL);
				insertRear(&bgList, (void *)bgInfo);
				listHead = bgList.head;
			}
			else{	
				wait_result = waitpid(pid, &exit_status, 0);
				if (wait_result < 0) {
					printf(WAIT_ERR);
					exit(EXIT_FAILURE);
	
				}
				free_job(job);  // if a foreground job, we no longer need the data

			}
		}
		free(line);
	}

    // calling validate_input with NULL will free the memory it has allocated
    validate_input(NULL);

#ifndef DEBUG
	fclose(rl_outstream);
#endif
	return 0;
}

void sigchld_handler(int sig){
	int olderrno = errno;
	deadChldAlarm = 1;
	errno = olderrno;
}