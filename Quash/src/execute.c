/**
 * @file execute.c
 *
 * @brief Implements interface functions between Quash and the environment and
 * functions that interpret an execute commands.
 *
 * @note As you add things to this file you may want to change the method signature
 *
 * @author Jackson Schilmoeller
 */

#include "execute.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "quash.h"
#include "deque.h"

// Remove this and all expansion calls to it
/**
 * @brief Note calls to any function that requires implementation
 */
#define IMPLEMENT_ME()                                                  \
  fprintf(stderr, "IMPLEMENT ME: %s(line %d): %s()\n", __FILE__, __LINE__, __FUNCTION__);
  
//Define queues for Jobs and Processes
IMPLEMENT_DEQUE_STRUCT(pid_queue, pid_t);
IMPLEMENT_DEQUE(pid_queue, pid_t);  

//Struct for Job
typedef struct Job{
	int job_id;
	pid_queue pids;
}Job;

IMPLEMENT_DEQUE_STRUCT(job_queue, struct Job);
IMPLEMENT_DEQUE(job_queue, struct Job);

job_queue jobs;

static bool instatiateJobQueue = true;

//Declare actual Job queue
//job_queue* jobs;

//And Declare pid queue
//pid_queue pids;

//Pipe globals
static int pipes[2][2];
static int prevPipe = -1;
static int nextPipe = 0;

/***************************************************************************
 * Interface Functions
 ***************************************************************************/

// Return a string containing the current working directory.
char* get_current_directory(bool* should_free) {
  	// DONE: Get the current working directory. This will fix the prompt path.
  	char* cwd = malloc(sizeof(char)*1024);
  	*should_free = true;
  	return getcwd(cwd, 1024);
}

// Returns the value of an environment variable env_var
const char* lookup_env(const char* env_var) {
  	// DONE: Lookup environment variables. This is required for parser to be able
  	// to interpret variables from the command line and display the prompt
  	// correctly
  	return getenv(env_var);
}

// Check the status of background jobs
void check_jobs_bg_status() {
  // TODO: Check on the statuses of all processes belonging to all background
  // jobs. This function should remove jobs from the jobs queue once all
  // processes belonging to a job have completed.

  // TODO: Once jobs are implemented, uncomment and fill the following line
  // print_job_bg_complete(job_id, pid, cmd);
}

// Prints the job id number, the process id of the first process belonging to
// the Job, and the command string associated with this job
void print_job(int job_id, pid_t pid, const char* cmd) {
  printf("[%d]\t%8d\t%s\n", job_id, pid, cmd);
  fflush(stdout);
}

// Prints a start up message for background processes
void print_job_bg_start(int job_id, pid_t pid, const char* cmd) {
  printf("Background job started: ");
  print_job(job_id, pid, cmd);
}

// Prints a completion message followed by the print job
void print_job_bg_complete(int job_id, pid_t pid, const char* cmd) {
  printf("Completed: \t");
  print_job(job_id, pid, cmd);
}

/***************************************************************************
 * Functions to process commands
 ***************************************************************************/
// Run a program reachable by the path environment variable, relative path, or
// absolute path
void run_generic(GenericCommand cmd) {
  // Execute a program with a list of arguments. The `args` array is a NULL
  // terminated (last string is always NULL) list of strings. The first element
  // in the array is the executable
  char* exec = cmd.args[0];
  char** args = cmd.args;

  // DONE: Implement run generic
  execvp(exec, args);
  //If exec fails
  perror("ERROR: Failed to execute program");
}

// Print strings
void run_echo(EchoCommand cmd) {
  // Print an array of strings. The args array is a NULL terminated (last
  // string is always NULL) list of strings.
  char** str = cmd.args;

  // Traverse NULL terminated array and printf each
  int index = 0;
  while(NULL != str[index]){
  	printf("%s ", str[index]);
  	index++;
  }
  printf("\n");

  // Flush the buffer before returning
  fflush(stdout);
}

// Sets an environment variable
void run_export(ExportCommand cmd) {
  // Write an environment variable
  const char* env_var = cmd.env_var;
  const char* val = cmd.val;

  // DONE: Implement export.
  setenv(env_var, val, 1);
}

// Changes the current working directory
void run_cd(CDCommand cmd) {
	// Get the directory name
	const char* dir = cmd.dir;

	// Check if the directory is valid
	if (NULL == dir) {
		perror("ERROR: Failed to resolve path");
		return;
	}
	else if(!strcmp(dir, " ")){
		// if cd _ then navigate to $HOME
		chdir(getenv("HOME"));
	}
	else{
		char* oldpwd = getcwd(NULL, 1024);

		// DONE: Change directory
		chdir(dir);

		// DONE: Update the PWD environment variable to be the new current working
		// directory and optionally update OLD_PWD environment variable to be the old
		// working directory.

		setenv("PWD", dir, 1);
		setenv("OLDPWD", oldpwd, 1);

		free(oldpwd);
	}
}

// Sends a signal to all processes contained in a job
void run_kill(KillCommand cmd) {
  int signal = cmd.sig;
  int job_id = cmd.job;

  // TODO: Remove warning silencers
  (void) signal; // Silence unused variable warning
  (void) job_id; // Silence unused variable warning

  // TODO: Kill all processes associated with a background job
}


// Prints the current working directory to stdout
void run_pwd() {
	// DONE: Print the current working directory
	bool should_free;
	char* directory = get_current_directory(&should_free);
	printf("%s \n", directory);

	// Flush the buffer before returning
	fflush(stdout);

	if(should_free){
		free(directory);
	}
}

// Prints all background jobs currently in the job list to stdout
void run_jobs() {
	// TODO: Print background jobs

	// Flush the buffer before returning
	fflush(stdout);
}

/***************************************************************************
 * Functions for command resolution and process setup
 ***************************************************************************/

/**
 * @brief A dispatch function to resolve the correct @a Command variant
 * function for child processes.
 *
 * This version of the function is tailored to commands that should be run in
 * the child process of a fork.
 *
 * @param cmd The Command to try to run
 *
 * @sa Command
 */
void child_run_command(Command cmd) {
	CommandType type = get_command_type(cmd);

	switch (type) {
	case GENERIC:
		run_generic(cmd.generic);
		break;

	case ECHO:
		run_echo(cmd.echo);
		break;

	case PWD:
		run_pwd();
		break;

	case JOBS:
		run_jobs();
		break;

	case EXPORT:
	case CD:
	case KILL:
	case EXIT:
	case EOC:
		break;

	default:
		fprintf(stderr, "Unknown command type: %d\n", type);
	}
}

/**
 * @brief A dispatch function to resolve the correct @a Command variant
 * function for the quash process.
 *
 * This version of the function is tailored to commands that should be run in
 * the parent process (quash).
 *
 * @param cmd The Command to try to run
 *
 * @sa Command
 */
void parent_run_command(Command cmd) {
	CommandType type = get_command_type(cmd);

	switch (type) {
	case EXPORT:
		run_export(cmd.export);
		break;

	case CD:
		run_cd(cmd.cd);
		break;

	case KILL:
		run_kill(cmd.kill);
		break;

	case GENERIC:
	case ECHO:
	case PWD:
	case JOBS:
	case EXIT:
	case EOC:
		break;

	default:
		fprintf(stderr, "Unknown command type: %d\n", type);
	}
}

/**
 * @brief Creates one new process centered around the @a Command in the @a
 * CommandHolder setting up redirects and pipes where needed
 *
 * @note Processes are not the same as jobs. A single job can have multiple
 * processes running under it. This function creates a process that is part of a
 * larger job.
 *
 * @note Not all commands should be run in the child process. A few need to
 * change the quash process in some way
 *
 * @param holder The CommandHolder to try to run
 *
 * @sa Command CommandHolder
 */
void create_process(CommandHolder holder, Job *job) {
	// Read the flags field from the parser
	bool p_in  = holder.flags & PIPE_IN;
	bool p_out = holder.flags & PIPE_OUT;
	bool r_in  = holder.flags & REDIRECT_IN;
	bool r_out = holder.flags & REDIRECT_OUT;
	bool r_app = holder.flags & REDIRECT_APPEND; // This can only be true if r_out
		                                       // is true
	// DONE: Setup pipes, redirects, and new process
	if(p_out){
		pipe(pipes[nextPipe]);
	}
	
	pid_t pid = fork();

	if(0 == pid){ //Child process
		if(r_in){
			//redirect input
			FILE* input = fopen(holder.redirect_in, "r");
			dup2(fileno(input), STDIN_FILENO);
			fclose(input);
		}
		if(r_out){
			//redirect output
			if(r_app){
				//append
				FILE* output = fopen(holder.redirect_out, "a");
				dup2(fileno(output), STDOUT_FILENO);
				fclose(output);
			}
			else{
				//overwrite
				FILE* output = fopen(holder.redirect_out, "w");
				dup2(fileno(output), STDOUT_FILENO);
				fclose(output);
			}
		}
		if(p_out){
			//pipe out to next process
			dup2(pipes[nextPipe][1], STDOUT_FILENO);
			close(pipes[nextPipe][1]);
		}
		if(p_in){
			//pipe in from previous process
			dup2(pipes[prevPipe][0], STDIN_FILENO);
			close(pipes[prevPipe][0]);
		}
		child_run_command(holder.cmd);
		exit(0);
	}
	else{

		if(p_in){
			close(pipes[prevPipe][0]);
		}
		if(p_out){
			close(pipes[nextPipe][1]);
		}
		nextPipe = (nextPipe + 1) % 2;
		prevPipe = (prevPipe + 1) % 2;
		push_front_pid_queue(&job->pids, pid);
		parent_run_command(holder.cmd);
	}
}

// Run a list of commands
void run_script(CommandHolder* holders) {
	if(instatiateJobQueue){
		jobs = new_job_queue(1);
		instatiateJobQueue = false;
	}
	
	if (holders == NULL){
		return;
	}

	check_jobs_bg_status();

	if (get_command_holder_type(holders[0]) == EXIT &&
	  	get_command_holder_type(holders[1]) == EOC) {
		end_main_loop();
		return;
	}

	CommandType type;
	Job job;
	job.pids = new_pid_queue(1);

	// Run all commands in the `holder` array
	for (int i = 0; (type = get_command_holder_type(holders[i])) != EOC; ++i){
		create_process(holders[i], &job);
	}

	if (!(holders[0].flags & BACKGROUND)) {
		// Not a background Job
		// DONE: Wait for all processes under the job to complete
		while(!is_empty_pid_queue(&job.pids)){
			pid_t tempPid = pop_front_pid_queue(&job.pids);
			int wait;
			waitpid(tempPid, &wait, 0);
		}
	}
	else {
		// A background job.
		// TODO: Push the new job to the job queue

		// TODO: Once jobs are implemented, uncomment and fill the following line
		// print_job_bg_start(job_id, pid, cmd);
	}
	
	destroy_pid_queue(&job.pids);
}
