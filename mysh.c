#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<sys/wait.h>
#include<stdbool.h>
#define DELIM " \t\r\n\a"
bool background;
int bg_proc[400];
int bg_index;
void caught_error();
void standard(char *argv[]){
	char *ptr;
	if(strcmp(argv[0],"exit")== 0){
		for(int i=0;i<bg_index;i++)
			kill(bg_proc[i], SIGTERM);
	}
	else if(strcmp(argv[0],"cd") == 0 && argv[1] == NULL){
		 chdir(getenv("HOME"));
        }
	else if(strcmp(argv[0],"cd") == 0 && argv[1] != NULL){
                        if(chdir(argv[1])== -1)
				caught_error();
	}
	else if(strcmp(argv[0],"pwd") == 0) {
			if(argv[1] == NULL){
			 ptr = getcwd(NULL, 0);
			 printf("%s\n",ptr);
			 free(ptr);
			}
			else
			 caught_error();
	}
}

void caught_error(){
	char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));

}

int check_operation(char *argv[])
{	int i = 0;
	int flag=100;
	while(argv[i] != NULL){
		if(strcmp(argv[i], ">") == 0){
			if(flag == 1){
				flag = 10;
				i++;
				continue;
			}
		  flag = 0;
		}
		else if(strcmp(argv[i], "<") == 0){
			if(flag == 0){
				flag = 10;
				i++;
				continue;
			}
		  flag = 1;
		}
		else if(strcmp(argv[i], "|") == 0){
		  flag = 2;
		  break; 
		}
		i++;
 	}
	return flag;
} 

void output_redirection(char *argv[],int n)
{
	int fd,ret,i=0,pid,k=0;
	char *argv1[20];
	while(argv[i] != NULL){
                if(strcmp(argv[i], ">") == 0){	
			break;
                }
		else{
			argv1[k] = strdup(argv[i]);
               	        k++;
		}
		i++;
	}
	argv1[k] = NULL;
	if(i == n || i == 0){
		caught_error();
                return;
	}
	if(argv[i+2] != NULL){
		 caught_error();
                 return;
	}
	fd = open(argv[i+1], O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
	if(fd == -1){
		 caught_error();
                 return;
	}
	fflush(stdout);
	pid = fork();
	if(pid == 0){
		ret = dup2(fd,STDOUT_FILENO);
		execvp(argv1[0], argv1);
		exit(0);
	}
	if(pid > 0){
		 if(background){
                        bg_proc[bg_index++] = pid;
                }
		if(!background)
			(void) wait(NULL);
	}
	for(int j=0;j<=k;j++)
                free(argv1[j]);
	close(fd);
}

void input_redirection(char *argv[],int n)
{	
	int out,i=0,pid,k=0;
	char *argv1[20];
	while(argv[i] != NULL){
		if(strcmp(argv[i],"<") == 0){
			break;
		}
		else{
			argv1[k] = strdup(argv[i]);
			k++;
		}
		i++;
	}
	argv1[k] = NULL;
	if(argv[i+2] != NULL){
		 caught_error();
                 return;
	}
	out = open(argv[i+1],O_RDONLY);
	if(out == -1){
		 caught_error();
                 return;
	}
	fflush(stdout);
	pid = fork();
	if(pid == 0){
		dup2(out,STDIN_FILENO);
		execvp(argv1[0], argv1);
	}
	if(pid > 0){
	        if(background) bg_proc[bg_index++] = pid;
		if(!background)
			(void) wait(NULL);
	}
	for(int j=0;j<=k;j++)
		free(argv1[j]);
	close(out);
}


void in_out(char *argv[])
{		
	char *command[20], *input, *output;
	int j=0;
	int i=0;
	while(strcmp(argv[i],"<") != 0 && strcmp(argv[i],">") != 0)
	{
		command[j] = strdup(argv[i]);
		j++;
		i++;
	}
	command[j] = NULL;
	int k = j;
	j=0;
	if(strcmp(argv[i],"<") == 0){ //input and then output
		input = strdup(argv[i+1]);
		if(strcmp(argv[i+2],">")!=0 || argv[i+3] ==NULL || argv[i+4] != NULL){
			caught_error();
                 	return;
		}
		else
			output = strdup(argv[i+3]);
		i = i+2;
	}

	else if(strcmp(argv[i],">") == 0){//output and then input		
        	output = strdup(argv[i+1]);
                if(strcmp(argv[i+2],"<")!=0 || argv[i+3] ==NULL || argv[i+4] != NULL){
                        caught_error();
                        return;
                }
		else
			input = strdup(argv[i+3]);

	}
	int fd = open(output, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
        if(fd == -1){
                 caught_error();
                 return;
        }
	int out = open(input,O_RDONLY);
        if(out == -1){
                 caught_error();
                 return;
        }
	fflush(stdout);
        int pid = fork();
        if(pid == 0){
	//	if(background) bg_proc[bg_index++] = getpid();
		dup2(fd, STDOUT_FILENO);
                dup2(out,STDIN_FILENO);
                execvp(command[0], command);
        }
        if(pid > 0){
	       if(background) bg_proc[bg_index++] = pid;
	       if(!background)
               	 (void) wait(NULL);
	}
	for(int p=0;p<=k;p++)
            free(command[p]);
	free(input);
	free(output);
        close(out);
	close(fd);
}

void _pipe(char *argv[])
{	
	char *argv1[20];
	char *argv2[20];
	int i=0,k=0;
	int pid1,pid2;
	int pipefd[2];
        while(strcmp(argv[i],"|") != 0){
		argv1[k] = strdup(argv[i]);
		i++; k++;
        }
	argv1[k] = NULL;
	free(argv1[k]);
	int p = k;
	if(i==0){
	   caught_error();
	   return;
	}
	else
	   i++;
	int prev = i;
	k = 0;
	while(argv[i]!=NULL){
		argv2[k] = strdup(argv[i]);
		i++; k++;
	}
	if(i == prev){
		 caught_error();
	         return;
	}
	argv2[k] = NULL;
	free(argv2[k]);
	int q = k;
	fflush(stdout);
	pid1 = fork();
	if (pid1 == 0) {
		pipe(pipefd);
		fflush(stdout);
		pid2 = fork();
		if (pid2 == 0) {
			close(pipefd[0]);
			dup2(pipefd[1], STDOUT_FILENO);
      			execvp(argv1[0], argv1);
	   		caught_error();
			exit(1);
   		}
   		else {
			close(pipefd[1]);
			dup2(pipefd[0], STDIN_FILENO);
        		execvp(argv2[0], argv2);
	   		caught_error();
			exit(1);
		}
	}
	else {
		(void)wait(NULL);
	}
	for(k=0;k<=p;k++)
		free(argv1[k]);
	for(k=0;k<=q;k++)
		free(argv2[k]);
}

int main(int argc, char *argv_main[])
{
	int counter = 0;
	char str[700];
	char *argv[20];
	char *token;
	int p,k;
	if (argc != 1) {
		caught_error();
		exit(1);
	}

	while(1){
		printf("mysh (%d)> ",++counter);
		fflush(stdout);
		background = false;
		fgets(str,700,stdin);
		if(strlen(str) == 1 && str[0] == '\n'){
			--counter;
			 continue;
		}
		if(strlen(str) > 128){
			caught_error();
				continue;
		}
		token = strtok(str,DELIM);
	        int i=0;
        	while(token!=NULL){
                	argv[i] = strdup(token);
                	i++;
                	token = strtok(NULL,DELIM);
        	}
		free(token);
		if(i!=0){
			argv[i] = NULL;
		//	free(argv[i]);
		}
		else {
			--counter;
			continue;
		}
		p = i;
		//built in commands
		if(strcmp(argv[0],"cd")== 0 || strcmp(argv[0],"exit") == 0 || strcmp(argv[0],"pwd") == 0){
			standard(argv);
			if(strcmp(argv[0],"exit") == 0){
				for(k=0;k<=p;k++)
                     		    free(argv[k]);
				exit(0);
			}
			else{
				for(k=0;k<=p;k++)
                                    free(argv[k]);
			}
			continue;
		}
		
		if(strcmp(argv[i-1],"&") == 0){
			 background = true;
			 free(argv[i-1]);
		     	argv[i-1] = NULL;
		}
		
		int flag =  check_operation(argv);
		//Output Redirection - flag = 0
		if(flag == 0)
		{
			output_redirection(argv,i);	
		}
                //Input Redirection - flag = 1
		else if(flag == 1)
		{
			input_redirection(argv,i);
		}
		//Combination of input and output
		else if(flag == 10)
		{
			in_out(argv);
		}
		//Pipe
		else if(flag == 2)
		{
			_pipe(argv);
		}
		else if(flag == 100){
			fflush(stdout);
                	int ret = fork();
                	int err;
                        	if(ret == 0){   
					err = execvp(argv[0],argv);
                                	if(err == -1){
                                        	caught_error();
						exit(0);
                                    	}
                          	}
                        	else if(ret > 0){
					if(background) bg_proc[bg_index++] = ret;
					if(!background)
                                		(void) wait(NULL);
                          	}
            	}
		 fflush(stdout);
		for(k=0;k<=p;k++)
                      free(argv[k]);

	}
}

