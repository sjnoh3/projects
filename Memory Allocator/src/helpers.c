#include "helpers.h"

// Your helper functions need to be here.


// void sigchld_handler(int sig){
// 	int olderrno = errno;
// 	deadChldAlarm = 1;
// 	errno = olderrno;
// }

// Sort by time first
// Sort by pid next
int compareBgentry(void *p1, void *p2){
	bgentry_t *firstBg = (bgentry_t *)p1;
	bgentry_t *secondBg = (bgentry_t *)p2;

	int comp_result = firstBg->seconds - secondBg->seconds;

	if(comp_result == 0){
		return firstBg->pid - secondBg->pid;
	}

	return comp_result;
}

void copyJob(job_info * copy, job_info *original){
	copy->line = malloc(sizeof(char)*(strlen(original->line)+1));
	if(original->in_file == NULL){
		copy->in_file = NULL;
	}
	else{
		copy->in_file = malloc(sizeof(char)*(strlen(original->in_file)+1));
		strcpy(copy->in_file, original->in_file);
	}

	if(original->out_file == NULL){
		copy->out_file == NULL;	
	}
	else{
		copy->out_file = malloc(sizeof(char)*(strlen(original->out_file)+1));
		strcpy(copy->out_file, original->out_file);
	}
	
	copy->procs = malloc(sizeof(proc_info *));
	strcpy(copy->line, original->line);
	copy->nproc = original->nproc;
	copy->bg = original->bg;

	copyProc(copy->procs, original->procs);
}

void copyProc(proc_info *copy, proc_info *original){
	if(original->next_proc == NULL){
		if(original->err_file == NULL){
			copy->err_file == NULL;	
		}
		else{
			copy->err_file = malloc(sizeof(char)*(strlen(original->err_file)+1));
			strcpy(copy->err_file, original->err_file);
		}
		copy->cmd = malloc(sizeof(char)*(strlen(original->cmd)+1));
		copy->argv = malloc(sizeof(char **)*(original->argc));
		copy->next_proc = malloc(sizeof(proc_info *));
		copy->argc = original->argc;
		int i = 0;
		for(; i < original->argc; i++){
			copy->argv[i] = malloc(sizeof(char)*(strlen(original->argv[i])+1));
			strcpy(copy->argv[i], original->argv[i]);
		}
		copy->next_proc = NULL;
		return;
	}
	else{
		if(original->err_file == NULL){
			copy->err_file == NULL;	
		}
		else{
			copy->err_file = malloc(sizeof(char)*(strlen(original->err_file)+1));	
		}
		proc_info * moreProc = malloc(sizeof(proc_info));
		copyProc(moreProc, original->next_proc);
		copy->cmd = malloc(sizeof(char)*(strlen(original->cmd)+1));
		copy->argv = malloc(sizeof(char **)*(original->argc));
		copy->next_proc = malloc(sizeof(proc_info *));
		copy->argc = original->argc;
		int i = 0;
		for(; i < original->argc; i++){
			char *argument = malloc(sizeof(char)*(strlen(original->argv[i])+1));
			strcpy(copy->argv[i], original->argv[i]);
		}
		copy->next_proc = moreProc;
	}	
}

void redirectFds(job_info * job){
	// 1 1 1
	if(job->in_file != NULL && job->out_file != NULL && job->procs->err_file != NULL){
		if(strcmp(job->in_file, job->out_file) != 0 | strcmp(job->in_file, job->procs->err_file) != 0 | strcmp(job->out_file, job->procs->err_file) != 0){
			redirectStdout(job);
			redirectStderr(job);
			redirectStdin(job);
			return;
		}
		fprintf(stderr, RD_ERR);
		exit(EXIT_FAILURE);
	}
	// 1 1 0
	else if(job->in_file != NULL && job->out_file != NULL && job->procs->err_file == NULL){
		if(strcmp(job->in_file, job->out_file) != 0){
			redirectStdout(job);
			redirectStdin(job);
			return;
		}
		fprintf(stderr, RD_ERR);
		exit(EXIT_FAILURE);
	}
	// 1 0 1
	else if(job->in_file != NULL && job->out_file == NULL && job->procs->err_file != NULL){
		if(strcmp(job->in_file, job->procs->err_file) != 0){
			redirectStderr(job);
			redirectStdin(job);
			return;
		}
		fprintf(stderr, RD_ERR);
		exit(EXIT_FAILURE);
	}
	// 1 0 0
	else if(job->in_file != NULL && job->out_file == NULL && job->procs->err_file == NULL){
			redirectStdin(job);
			return;
	}
	// 0 1 1
	else if(job->in_file == NULL && job->out_file != NULL && job->procs->err_file != NULL){
		if(strcmp(job->out_file, job->procs->err_file) != 0){
			redirectStdout(job);
			redirectStderr(job);
			return;
		}
		fprintf(stderr, RD_ERR);
		exit(EXIT_FAILURE);
	}
	// 0 1 0
	else if(job->in_file == NULL && job->out_file != NULL && job->procs->err_file == NULL){
		redirectStdout(job);
		return;
	}
	// 0 0 1
	else if(job->in_file == NULL && job->out_file == NULL && job->procs->err_file != NULL){
		redirectStderr(job);
		return;
	}
	// 0 0 0, Do nothing
	
}

void redirectStdin(job_info *job){
	int redir;
	redir = open(job->in_file, O_RDONLY, 00777);
	if(redir < 0){
		fprintf(stderr, RD_ERR);
		exit(EXIT_FAILURE);
	}
	dup2(redir, fileno(stdin));
	close(redir);
}

void redirectStdout(job_info *job){
	int redir;
	redir = open(job->out_file, O_WRONLY | O_CREAT | O_TRUNC, 00777);
	dup2(redir, fileno(stdout));
	close(redir);
}

void redirectStderr(job_info *job){
	int redir;
	redir = open(job->procs->err_file, O_WRONLY | O_CREAT | O_TRUNC, 00777);
	dup2(redir, fileno(stderr));
	close(redir);
}
