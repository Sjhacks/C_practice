#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include<stdbool.h>

int cmdIndex = 1;

struct history
{
    char *arg;
    struct history *next;
};

struct history *head = NULL;
struct history *curr = NULL;

//this creates the list that will store the history
struct history* create_list(char *arg)
{
    struct history *ptr = (struct history*)malloc(sizeof(struct history));
    if(NULL == ptr)
    {
        printf("History creation failed\n");
        return NULL;
    }
    ptr->arg = arg;
    ptr->next = NULL;

    head = curr = ptr;
    return ptr;
}

// adds the command to the history
struct history* add_to_hist(char *arg)
{
    if(NULL == head)
    {
        return (create_list(arg));
    }
    struct history *ptr = (struct history*)malloc(sizeof(struct history));
    
    cmdIndex++;
    
    ptr->arg = arg;
    ptr->next = head;
    head = ptr;
    
    return ptr;
}

//searches for the command in the history
struct history* search(char a, struct history **prev)
{
    struct history *ptr = head;
    struct history *tmp = NULL;
    bool found = false;
    int i;
    
    if(ptr != NULL){
        for (i = 0; i<10; i++){
            if(ptr->arg[0] == a){
                found = true;
                break;
            }
            else{
                tmp = ptr;
                ptr = ptr->next;
                if(ptr == NULL){
                    break;
                }
            }
        }
        if(found == true){
            return ptr;
        }
        else{
            return NULL;
        }
    }
}

//displays the history
void print_hist(void)
{
    struct history *ptr = head;
    int i = 0;
    int j = cmdIndex; 

    for (i=0; i<10; i++)
    {
        printf("[%d][%s] \n", j, ptr->arg);
        ptr = ptr->next;
        j--;
        if (ptr == NULL){
            break;
        }
    }
    
    return;
}


int getcmd(char *prompt, char *args[], int *background)
{
    int length, i = 0;
    char *token, *loc;
    char *line = NULL;
    size_t linecap = 0;

    printf("%s", prompt);
    length = getline(&line, &linecap, stdin);

    if (length <= 0) {
        exit(-1);
    }
    
	add_to_hist(line);
    
    // Check if background is specified..
    if ((loc = index(line, '&')) != NULL) {
        *background = 1;
        *loc = ' ';
    } else
        *background = 0;

    while ((token = strsep(&line, " \t\n")) != NULL) {
        int j = 0;
        for (j = 0; j < strlen(token); j++)
            if (token[j] <= 32)
                token[j] = '\0';
        if (strlen(token) > 0)
            args[i++] = token;
    }

    return i;
}


int main()
{
    char *args[20];
    int bg;
    int i =0;
	struct history *ptr = NULL;
    
	while(1) {
		bg = 0;
		
		printf(">");
		
        int cnt = getcmd("\n>>  ", args, &bg);
        for (i = 0; i < cnt; i++)
            printf("\nArg[%d] = %s", i, args[i]);

        if (bg)
            printf("\nBackground enabled..\n");
        else
            printf("\nBackground not enabled \n");

        printf("\n\n");
		
		int pid = fork();
		//if pid>0 this is the parent and will wait for child if bg is enabled
		if (pid > 0){
			
			if(bg == 0){
			    int status = 0;
			    while (wait(&status) != pid);
		    }
		}
		else if (pid == 0){
		    if( strcmp(args[0],"history") == 0){ /*  Print History if history was entered */
                print_hist();
				exit(0);
            }
			//changes the working directory
			else if(strcmp(args[0],"cd") == 0){
				if(chdir(args[1]) < 0 ){
					printf("directory not found");
				}else{
					printf("directory changed");
				}
				exit(0);
			}
			//prints the working directory
			else if(strcmp(args[0],"pwd") == 0){
				char* cwd;
				char buff[10000];
				
				cwd = getcwd( buff, 10000 );
				if( cwd != NULL ) {
					printf( "The current working directory is %s.\n", cwd );
				}
				exit(0);
			}
			//searches the command list for the matching command and executes it
			else if(strcmp(args[0],"r") == 0){
                ptr = search(args[1][0], NULL);
				if (ptr != NULL){
					int cnt = getcmd(ptr->arg, args, &bg);
					if (execvp(args[0], args) < 0) {
						printf("Error: exec failed\n");
						exit(1);
					}
					exit(0);
				}
			//closes the shell 
			}else if(strcmp(args[0],"exit()") == 0){
                exit(0);
			}
			//executes the command
			else{
		        if (execvp(args[0], args) < 0) {
                    printf("Error: exec failed\n");
                    exit(1);
                }
		        exit(0);
		    }
		    
		}
		else{
		    printf("fork error/n");
		}
	    	
	}
}
