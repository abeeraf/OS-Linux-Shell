#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <dirent.h>
#include <termios.h>
#include <unistd.h>
#include <pwd.h>
#include <limits.h>
#include <unistd.h>

char prompt_line[4096];
char input_user[4096];
static char* cur_direc;
static char* arguments[1000];
_Bool checker_array[2]={0,0};
char* commands[1000];

void init_shell_command_prompt() 
{ 
	char cwd[4096],host[64];
    	struct passwd *user_ID = getpwuid(getuid()); 

    	gethostname(host, sizeof(host));  
    	getcwd(cwd, sizeof(cwd)) ;

	strcpy(prompt_line,user_ID->pw_name);
	strcat(prompt_line,"@");
	strcat(prompt_line,host);
	strcat(prompt_line," ");
	strcat(prompt_line, cwd);
	strcat(prompt_line," > ");
}

void Avoid_Zombie_processes()// also used kill with exec
{
	// code to avoid zombie processes, uses the signal handler
	// code not called becuase it also messes up other library functions
	// however wait command used alternatively with kill signal
	////////////////////////////////////////////
	struct sigaction sigchld_action = {
	  .sa_handler = SIG_DFL,
	  .sa_flags = SA_NOCLDWAIT
	};
	sigaction(SIGCHLD, &sigchld_action, NULL);
	///////////////////////////////////////////
}

char* Remove_Edit_Spaces(char* str, int xx) 
{
	char *temp_storage;
	switch(xx){
	case (1): // remove spaces
	
		if (!(temp_storage = (char *) malloc(sizeof(str)*sizeof(char)))) //check empty
		return NULL;
		int y = 0;
	    	for(int x=0; str[x++];) 
	    	{
			if (str[x-1] != ' ')
			temp_storage[y++] = str[x-1];
		}
		temp_storage[y] = '\0';
		return temp_storage;
	
	case (2):// sep by space
	
		arguments[0] = strtok(str, " \n\t");int x;
		for( x = 1; (arguments[x] = strtok(NULL," \n\t")) != NULL; x++){}// skip next line 
	    	arguments[x] = NULL; //last value NULL
	}	
}

void close_Pipes(int pipe_fd, int checker_num_last, int* fd )
{
	if (checker_num_last)//closing pipes in order 
	close(fd[0]);

	if (pipe_fd)
	close(pipe_fd);

	close(fd[1]);
}

int run_prog(char* new_commands,int pipe_fd,_Bool tok_stat_first, int checker_num_last )
{
	int fd[2];
	pipe(fd);
	pid_t pid=fork();
	int std_in,std_out;

	if (pid > 0){//parent code	
		
		if(checker_array[0])	//background execution of programs
		{checker_array[0]=0;printf("PROCESS CREATED TO RUN BACKGROUND PROCESS \n PID: %d\n",pid);}
		else
		waitpid(pid,0,0);
	}
	if ( !pid ){// child code

		setenv("parent",getcwd(cur_direc, 1024),1); //adding environment var
		if ((!checker_num_last))//output redirection to terminal becuase background process
		{
			if (!tok_stat_first && pipe_fd){
			dup2 (pipe_fd, STDIN_FILENO);//read from pipe 
			dup2 (fd[1], STDOUT_FILENO); //sending to pipe
			}
			if (tok_stat_first && !pipe_fd)
			dup2 (fd[1], STDOUT_FILENO);//writing
		}
		else
		dup2 (pipe_fd, STDIN_FILENO);	
		_Bool inn,outt;

		char *output_re,*input_re;
		char *temp_storage[500];
	
		if (strchr(new_commands, '<') && (strchr(new_commands, '>'))) //searching characters 
        	{	
			inn = 1;//both operators
			outt = 1;
			temp_storage[0] = strtok(new_commands, "<");
			for(int x=1; (temp_storage[x] = strtok(NULL,">")); x++){}//

		    	input_re = Remove_Edit_Spaces(strdup(temp_storage[1]),1);
			output_re = Remove_Edit_Spaces(strdup(temp_storage[2]),1);
			Remove_Edit_Spaces(temp_storage[0],2);
		}
		else if (strchr(new_commands, '<'))//only '<' operator
		{
        		inn = 1;
			temp_storage[0] = strtok(new_commands, "<");
			for (int x = 1 ;(temp_storage[x] = strtok(NULL,"<")); x++){}

			input_re = Remove_Edit_Spaces(strdup(temp_storage[1]),1);
		    	Remove_Edit_Spaces(temp_storage[0],2);
			
		}
		else if (strchr(new_commands, '>'))
		{
        		outt = 1;
			temp_storage[0] = strtok(new_commands, ">");
			for (int x = 1 ;(temp_storage[x] = strtok(NULL,">")); x++){}

			output_re = Remove_Edit_Spaces(strdup(temp_storage[1]),1);
			Remove_Edit_Spaces(temp_storage[0],2);
			
		}
		if (outt) //using dup
        	{
            		std_out = open(output_re, O_CREAT | O_TRUNC | O_WRONLY, 0600); ////////on terminal
			dup2 (std_out, STDOUT_FILENO);
			close (std_out);
		}
            	if (inn) 
            	{
            		std_in = open(input_re, O_RDONLY, 0600);  
			dup2 (std_in, STDIN_FILENO);
			close (std_in);
		}
		
		if (execvp(arguments[0], arguments) < 0) //calling exec kill zombie processes
	 	{
			fprintf(stderr, "%s: Command not found\n",arguments[0]);// in case wrong command
			kill(getpid(),SIGTERM);//child terminated
		}
	exit(0);// wait and exit also avoids zombie processes 	
	}
	//now close the pipes
	close_Pipes(pipe_fd,checker_num_last,fd);
	return (fd[0]);
}

void ls_command(char * directory)
{
	struct dirent **file_or_direc_name; 
      	int check_dir=0;
	
	if (directory == NULL) //Directory Does Not exist
	goto here;

	DIR *path_direc = opendir (directory);

    	if (path_direc)
    	{ 
        	(void) closedir (path_direc);
		return;
    	}
	
	here:
	if(directory)
	check_dir=scandir(directory,&file_or_direc_name,NULL,alphasort);
	else
	check_dir=scandir(".",&file_or_direc_name,NULL,alphasort);// home
	 
  	while (check_dir--) //printing file names 
        { 
                printf("%s\n",file_or_direc_name[check_dir]->d_name);
                free(file_or_direc_name[check_dir]); 
        }
        free(file_or_direc_name); //deallocating memeory
}

void command_Environ(int indicator)
{
	char* variable_name=arguments[1];
	char* value="";
	if(arguments[2])
	value=arguments[2];

	if(!variable_name)
        {printf("%s","Not enought arguments provided in declaration\n");return;}

	switch(indicator){

	case(1)://unset
	
		if(getenv(variable_name))//need varibale and value not given
		{
			unsetenv(variable_name);
			printf("%s", "Variable Erased\n");
		}
            	else
            	printf("%s", "Variable does not Exist\n");break;
      	
	case(2):// setenv

		if(getenv(variable_name))//if variable exists already in environment varibale otherwise new variable declared
		printf("%s", "Variable Overwritten\n");
            	else
            	printf("%s", "Variable Created\n");
            	setenv(variable_name, value, 1);
    	}	
}

int set_descriptor(int count)
{
	int file_desc,std_out;
	file_desc = open(arguments[count+1], O_CREAT | O_TRUNC | O_WRONLY, 0600);// opening file
	std_out = dup(STDOUT_FILENO); 	//standard output duped	
	dup2(file_desc, STDOUT_FILENO); // terminal output; 
	close(file_desc);
}

int execute_commands(char* commands,int checker_num_last, int pipe_fd , _Bool tok_stat_first)
{	
	
	char* temp_commands = strdup(commands);
	Remove_Edit_Spaces(commands,2);
	_Bool if_controller=0;
	_Bool set_background=0;
	int count=0;

	int std_out;

	extern char** environ;
	char **envi;
	while (arguments[count])
	{
		if (!strcmp(arguments[count],"&") || (!strcmp(arguments[count],">")) || (!strcmp(arguments[count],"<")))
		break;
		count++;
	}
	//Now start checking the commands
	if (arguments[0])
	{
		if((!strcmp(arguments[0],"exit")))//if only exit command
		exit(0);
		if (!strcmp(arguments[0],"clear")) 
		printf("\x1B[2J\x1B[H");
		if (!strcmp(arguments[0],"cd") && !checker_array[0]) // this won't work in the background
                {	if_controller=1;
			if (!arguments[1]) 
			chdir(getenv("HOME")); 
			else
			{ 
				if (chdir(arguments[1]) == -1) 
				printf(" %s: no such directory\n", arguments[1]);
			}
			init_shell_command_prompt();
			
                }

	if (!checker_array[1])
	{

		if (!checker_array[0])
		{
			if (!(strcmp(arguments[0],"pwd")))
			{	if_controller=1;
				if(arguments[count])
				{
				     	if ((!strcmp(arguments[count],">")) && (arguments[count+1]))
				     	{
						std_out=set_descriptor(count);
						printf("%s\n", getcwd(cur_direc, 1024));
						dup2(std_out, STDOUT_FILENO);
				 	}
		     		}
		     		else
		    		printf("%s\n", getcwd(cur_direc, 1024));
		    		set_background=1;
			}
			else if(!(strcmp(arguments[0],"ls")))
			{	if_controller=1;
				if(arguments[count])//otherwise
				{
					if (!(strcmp(arguments[count],">")) && (arguments[count+1]))
			     		{
						std_out=set_descriptor(count);
						if(arguments[1] && !count)
			    			{
							if(arguments[1][0]!='-')// -ls command here 
							ls_command(arguments[1]);
							else{
								dup2(std_out, STDOUT_FILENO);// going back to previous stdout
								return (run_prog(temp_commands,pipe_fd, tok_stat_first,checker_num_last ));
							}
						}
						else
						ls_command(NULL);
						dup2(std_out, STDOUT_FILENO);
					}
					
				}
				else
				{
					if(arguments[1]){	
						if(arguments[1][0]!='-')
			    			ls_command(arguments[1]);
						else
						return (run_prog(temp_commands,pipe_fd, tok_stat_first,checker_num_last ));
					}
					else
		       			ls_command(NULL);
				}
				set_background=1;
			}
		}
			if (!(strcmp(arguments[0],"environ")))//environ
	       		{	if_controller=1;
		    		if (arguments[count])
		    		{
	    				if (!(strcmp(arguments[count],">")) && arguments[count+1])
	    				{
						std_out=set_descriptor(count);
						for(envi = environ; *envi != 0; envi++)//built in fucntion + from internet
        					printf("%s\n", *envi);
						dup2(std_out, STDOUT_FILENO);
					}
				}
				else{
				for(envi = environ; *envi != 0; envi++)//built in fucntion + from internet
        			printf("%s\n", *envi);
				}
				set_background=1;
	       	 	}
	}
   	    	if (!strcmp(arguments[0],"setenv"))
   	    	{	
			if_controller=1;
            		command_Environ(2);
            		set_background=1;
           	} 
	    	if (!strcmp(arguments[0],"unsetenv"))
	    	{	
			if_controller=1;
			set_background=1;
            		command_Environ(1);
            		
        	}
		if(set_background)
		checker_array[1]=0;
        	if(!if_controller)
        	return (run_prog(temp_commands,pipe_fd, tok_stat_first,checker_num_last ));	        

	}
}

void Assess_input()
{
	_Bool tok_stat=1; //check if its the first command in a cascade 
	for (int x = 0 ; x < 500 ; x++) //500 assumed to be max input user length
	{ 	
		if(input_user[x]=='|') //checking commands b/w pipes
		checker_array[1]=1;
		if(input_user[x]=='&')	//checking background commands 
        	{
			input_user[x]=NULL;
			if(input_user[x-1]==" ")// if there is space it should be NULL for pwd& etc to be run
			input_user[x-1]=NULL;
			checker_array[0]=1;
		}
		
	}
	commands[0] = strtok(input_user, "|");// split the commands separted by '|'
	int x=1;
	while (commands[x] = strtok(NULL, "|"))
	x++;
	commands[x] = NULL; //last value should be NULL

	int pipe_fd=0;
	int i=0;
	int last=0;
	while (i < x)
	{
		if(i == x-1)// to show last command
		last=1;	
	
		pipe_fd=execute_commands(commands[i],last,pipe_fd,tok_stat); 
		i++;
		if(tok_stat)
		tok_stat=0;
	}
}

int main(int argc, char *argv[]) {

	signal(SIGINT, SIG_IGN);
	init_shell_command_prompt();
	setenv("shell",getcwd(cur_direc, 1024),1);
	while(1)	// infinte loop 
    	{
		printf("%s",prompt_line);
		here:
		memset(input_user, '\0', 4096 );// clear buffer
		fgets(input_user, 4096, stdin);
		if(!strcmp(input_user, " "))	//ignore starting spaces like in linux
		goto here;
		if(!strcmp(input_user, "\n"))	//if enter new line again like in linux
		continue;
		Assess_input();
	}
	exit(0); // exit normally	
}


