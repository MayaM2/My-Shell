/*
 * myshell.c
 *
 * prepare:
 * 1. Add SIGCHLD sigaction, so that child-proccesses won't
 * turn into zombies. This is done by defining a sigaction struct with SA_NOCLDWAIT flag.
 *
 * 2. Add SIGINT sigaction, so that parent proccess (which is the shell) won't terminate
 * upon SIGINT. This feature automatically applies to child processes.
 *
 *
 * process_arglist:
 * First run on arglist and check if it contains "|" char, if so mark flag pipeseen.
 * There are 3 options of action:
 * 1. pipeseen is 1, so create a pipe using pipe(),and then fork two children.
 * Use dup2() to set the stdin of the second part of the command, and stdout of the first
 * part of the command to the fd's created by pipe().
 * then execute using execvp.
 *
 * 2. & is in arglist[count-1]. Since srglist[count]=NULL, this means that the user's command
 * ends with & so we'll need to run the process in the background, using fork to create
 * the child process and then parent *not* waiting for him.
 *
 * 3. anyting else: no pipe and not on background. Create a child using fork and make
 * child run execvp on arglist. Wait for child since it's in the foreground.
 *
 *
 * finalize:
 * Do nothing and return. Since in "prepare" we did not allocate any dynamic memory,
 * there is nothing to free.
 *
 * A more explicit documentation given within the functions themselves (when needed).
 *
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

int prepare(void){ // set signal handling

	// don't turn children into zombies, to treat background child processes
	struct sigaction sigchld_action = {
	.sa_handler = SIG_DFL,
	.sa_flags   = SA_NOCLDWAIT
	};
	if(sigaction(SIGCHLD, &sigchld_action, NULL)<0){
		perror("Error: sigaction for SIGCHLD failed\n");
		return -1;
	}

	// ignoring SIGINT. will also pass to children since it is an ignoring disposition
	struct sigaction sigint_action_ign = {
	.sa_handler = SIG_IGN
	};
	if(sigaction(SIGINT, &sigint_action_ign, NULL)<0){
		perror("Error: sigaction for SIGINT failed\n");
		return -1;
	}
	return 0;
}

int process_arglist(int count, char** arglist){
	int pipeseen=0;
	int i=0, status;
	int pipefd[2];
	pid_t pid1, pid2;

	struct sigaction sigint_action_dft = {
	.sa_handler = SIG_DFL
	};

	for(i=0;i<count;i++){ // check if there is a pipe in the command. If so, set pipeseen.
		if(!strcmp(arglist[i],"|")){
			pipeseen=1;
			break; // if there's a pipe, int i holds it's index in arglist.
		}
	}

	if(pipeseen){  // case there is a pipe.
		if(pipe(pipefd)<0){ // now pipfd[2] contains fd's for I/O of pipe
			perror(strerror(errno));
			return 0;
		}
		if((pid1 = fork()) <0){ // parent forks the first child proccess
			perror(strerror(errno));
			exit(1);
		}
		if (pid1 == 0){ // first child is the second part of pipe in the user's command
			if(sigaction(SIGINT, &sigint_action_dft, NULL)<0){ // terminate upon SIGINT unlike parent
				perror(strerror(errno));
				exit(1);
			}
			close(pipefd[1]); // close unused half of pipe
			if(dup2(pipefd[0],0)<0){ // replace standard input with input fd given by pipe()
				perror(strerror(errno));
				exit(1);
			}
			close(pipefd[0]);

			/*
			 * execute second part of pipe:
			 * we are going to use the parts of arglist starting from index i+1
			 * because we know from the loop at the beginning that
			 * in index i of arglist there's the "|" char
			 */
			if (execvp(*(arglist+(i+1)),(arglist+i+1))<0){ //execute the process
				perror(strerror(errno));
				exit(1);
			}
		}
		else{ // parent
			if((pid2 = fork()) <0){ // parent will fork another child for the first part of the pipe in the use'rs command
				perror(strerror(errno));
				exit(1);
			}
			if(pid2==0){ // second child
				if(sigaction(SIGINT, &sigint_action_dft, NULL)<0){ // terminate upon SIGINT unlike parent
					perror(strerror(errno));
					exit(1);
				}

				/*
				 * change arglist for first part of pipe:
				 * we know from a previous loop that "|" symbol is in the i'th index
				 * so we will use the part of arglist untill that point by changing
				 * the i'th index to NULL, instead of unneeded "|" char.
				 */
				arglist[i] = NULL; // null-terminate the array for execvp()

				close(pipefd[0]); // close unused unput half of pipe
				if(dup2(pipefd[1],1)<0){ // replace standard output with output fd given by pipe()
					perror(strerror(errno));
					exit(1);
				}
				close(pipefd[1]);

				if(execvp(*arglist, arglist)<0){ // execute the process
					perror(strerror(errno));
					exit(1);
				}
			}
			else{ // parent
				close(pipefd[0]); // parent doesn't use fd's
				close(pipefd[1]);
				waitpid(pid1, &status, 0);
				waitpid(pid2, &status, 0); // wait for two children
				return 1;
			}
		}
	}

	else if(!strcmp(arglist[count-1],"&")){ // case it is a background process, (and no pipe)
		arglist[count-1]=NULL; // erase "&", Make array NULL terminated for execvp()

		if((pid1 = fork()) <0){ // fork a child process
			perror(strerror(errno));
			exit(1);
		}
		if(pid1==0){ // child
			if(execvp(*arglist,arglist)<0){
				perror(strerror(errno));
				exit(1);
			}
		}
		// Parent process should not wait at this case
		return 1;
	}

	else{ // case no pipe and not on background
		if((pid1 = fork())<0){ // fork a child process
			perror(strerror(errno));
			exit(1);
		}
		if(pid1==0){ // child
			sigaction(SIGINT, &sigint_action_dft, NULL); // terminate upon SIGINT unlike parent
			if(execvp(*arglist,arglist)<0){
				perror(strerror(errno));
				exit(1);
			}
		}
		else{ // Parent process should wait for child
			waitpid(pid1, &status, 0);
			return 1;
		}
	}
	return 1;
}

int finalize(void){ // nothing to release
	return 0;
}
