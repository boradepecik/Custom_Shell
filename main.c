#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>


//Node struct for a linked list for Alias, each have their name and the command string and a pointer to next alias.
typedef struct Node {
    char aliasName[1024];
    char command[1024];
    struct Node* next;
} Node;


//trims the leading and trailing blanks from the given input, splits the strings, creates a string array for the command.
char** createArgs(char* str) {
    char** args = malloc(1000 * sizeof(char*));
    if (args == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    char* token = strtok(str, " \t");
    int count = 0;
    
    while (token != NULL && count < 999) {
        args[count] = strdup(token); 
        if (args[count] == NULL) {
            perror("strdup");
            exit(EXIT_FAILURE);
        }
        token = strtok(NULL, " \t");
        count++;
    }

    
    args[count] = NULL; 
    
    if(count>=1) {
        if(args[count-1][strlen(args[count-1])-1]=='\n') {args[count-1][strlen(args[count-1])-1]='\0';}
        if(strlen(args[count-1])==0) {args[count-1] = NULL;}
    }
    
    

    return args;
}

int main() {

    //GET ALL ALIASES FROM THE FILE STORE IN A LINKED LIST
    FILE *aliasesFile;
    aliasesFile = fopen("aliases.txt", "a+");
    if (aliasesFile == NULL) {
        perror("Error opening aliases file");
        return 1;
    }
    char aliasName[1024];
    char command[1024];
    Node * head = NULL;
    Node * prev = NULL;
    Node * curr = NULL;
    while (fscanf(aliasesFile, "%s %[^\n] ", aliasName, command) == 2) {
        if(head==NULL) {
            head = (Node*)malloc(sizeof(Node));
            strcpy(head->aliasName,aliasName);
            strcpy(head->command,command);
            prev = head;
        }
        else {
            curr = (Node*)malloc(sizeof(Node));
            strcpy(curr->aliasName,aliasName);
            strcpy(curr->command,command);
            prev->next = curr;
            prev = curr;
            
        }
    }
    fclose(aliasesFile);

       
    

    //initialize necessary information such as process counts, last commands, path variable etc.
    
    char * PATH = getenv("PATH");
    char *paths[100];
    int path_number = 0;
    char *token = strtok(PATH, ":");
    while (token != NULL && path_number < 100) {
        paths[path_number++] = strdup(token); 
        token = strtok(NULL, ":");
    }

    int backprocess = 0 ;
    char lastCommand[1024] = "\n";
    char currentCommand[1024] = "\n";
    int currentProcesses = 0;

    //start the main while loop.
    while(1) {
        
        //get username and hostname in each iteration.
        char * username = getenv("USER");
        char * hostname = getenv("HOSTNAME");
        if(username==NULL) {username = "root";}
        
        char input[1024];
        char ** argslist;
        char pwd[100];
        getcwd(pwd, sizeof(pwd));
        
        
        printf("%s@%s %s --- ", username, hostname, pwd);
        
        //get input
        fgets(input,sizeof(input),stdin);
        
        //set the last element of the input null instead of \n
        if(input[strlen(input)-1]=='\n') input[strlen(input)-1] = '\0';

        //CHECK IF EMPTY STRING GIVEN, if so, just continue to the next iteration.
        if (strlen(input)==0) {
            
            int status;
            pid_t t = waitpid(-1,&status,WNOHANG);
            while(t>0) {
                printf("[%d]  Done\t %d\n", backprocess,t);
                backprocess--;
                currentProcesses--;
                t = waitpid(-1,&status,WNOHANG);
            }
            continue;
        }

        //save the current command to be input because it might change later
        strcpy(currentCommand,input);
        
        
        
        //call createArgs function, argslist is now a string array consisting of command words given with blankspaces.
        argslist = createArgs(input);
        

        //IF THE FIRST WORD IS ALIAS, ADD THE ALIAS NAME COMMAND TO THE FILE, ALSO CREATE A NEW NODE AND APPEND IT TO THE END OF THE LINKED LIST.
        if(strcmp(argslist[0],"alias")==0) {
            strcpy(lastCommand, currentCommand);
            FILE *aliasesFile;
            aliasesFile = fopen("aliases.txt", "a+");
            if (aliasesFile == NULL) {
                perror("Error opening aliases file");
                return 1;
            }
            char * alias_name = argslist[1];
            char final_alias_name[100];
            strcpy(final_alias_name,alias_name);
            int i = 3;
            while (argslist[i]!=NULL)
            {
                argslist[i-3] = argslist[i];                  
                i++;
            }
            if(argslist[3][0]=='"') memmove(argslist[0], argslist[0] + 1, strlen(argslist[0]));
            if(argslist[i-4][strlen(argslist[i-4])-1]=='"') argslist[i-4][strlen(argslist[i-4])-1] = '\0';
            argslist[i-3] = NULL;
            i = 0;
            char * command;
            command = malloc(1024);
            if (command == NULL) {
                perror("Memory allocation failed");
                return 1;
            }
            strcpy(command,argslist[0]);
            while (argslist[i]!=NULL)
            {
                strcat(alias_name," ");
                strcat(alias_name,argslist[i]);
                if(i!=0) {
                    strcat(command, argslist[i]);
                }
                strcat(command," ");
                i++;
            }
            if(strlen(command)>0) {
                command[strlen(command)-1]='\0';
            }
            
            strcat(alias_name,"\n");
            fprintf(aliasesFile,alias_name);
            fclose(aliasesFile);
            Node * newNode = (Node*)malloc(sizeof(Node));
            strcpy(newNode->aliasName,final_alias_name);

            strcpy(newNode->command,command);
            if(curr!=NULL) curr->next = newNode;
            else head = newNode;
            

            curr = newNode;
            free(command);
            
            continue;
        }


        //GIVEN A COMMAND CHECK IF THE FIRST WORD IS IN THE ALIAS LINKED LIST, IF SO, REPLACE THE ALIAS NAME WITH THE COMMAND AND CONTINUE EXECUTION.
        Node * node = head;
        char ** argslist2 = malloc(1000 * sizeof(char *));
        while(node!=NULL) {
            if(strcmp(node->aliasName, argslist[0])==0) {
                
                char args[1024];
                strcpy(args, node->command);
                
                char * temp = strtok(args, " ");
                int count = 0;
                while(temp!=NULL) {
                    argslist2[count] = temp;
                    count++;
                    temp = strtok(NULL," ");
                }
                int i = 1;
                while(argslist[i]!=NULL) {
                    argslist2[count] = argslist[i];
                    i++;
                    count++;
                }
                argslist2[count]==NULL;

                i = 0;
                while(argslist2[i]!=NULL) {
                    argslist[i] = argslist2[i];
                    i++;
                }
                argslist[i] = NULL;

                
                break;
            }
            node = node->next;
        }
        free(argslist2);
        
        
        //CHECK FOR REDIRECTIONS SIGNS > >> >>>
        int argCount = 0;
        int redirectProcess = 0;
        int red;
        for(int i = 0;argslist[i]!=NULL;i++) {
            if(strcmp(argslist[i],">")==0) {
                redirectProcess = 1;
                red = argCount+1;
            }
            else if(strcmp(argslist[i],">>")==0) {
                redirectProcess = 2;
                red = argCount+1;
            }
            else if(strcmp(argslist[i],">>>")==0) {
                redirectProcess = 3;
                red = argCount+1;
            }
            argCount+=1;
        }
        
        

        //CHECK FOR EXIT exit
        if(strcmp(argslist[0],"exit")==0) {
            break;
        }
        
        currentProcesses ++;

        int backgroundProcess = 0;
        
        //CHECK FOR BACKGROUND PROCESS &
        if(strcmp(argslist[argCount-1],"&")==0) {
            backgroundProcess=1;
            backprocess++;
            
            argslist[argCount-1]=NULL;
        }
        
        //IF THE FIRST WORD IS BELLO, WE FORK A NEW CHILD (WHICH WE WILL EXIT WHEN THE PROCESS ENDS), WE GET THE NECESSARY OUTPUTS
        //PRINT IF NO REDIRECTION, PRINT TO FILE IF THERE IS REDIRECTION SIGN. ALSO CHECK FOR BACKGROUND BELLO HERE
        if(strcmp(argslist[0],"bello")==0) {
            
            pid_t pid = fork();
            if(backgroundProcess&pid!=0) {printf("[%d] %d\n",backprocess,pid);}
            if(pid==0) {
                char output[1024];
                strcpy(output,username);
                strcat(output,"\n");
                strcat(output,hostname);
                strcat(output,"\n");
                if(strcmp(lastCommand,"\n")!=0) strcat(output,lastCommand);
                strcat(output,"\n");
                char *tty_name = ttyname(STDIN_FILENO);
                strcat(output,tty_name);
                strcat(output,"\n");
                strcat(output,getenv("SHELL"));
                strcat(output,"\n");
                strcat(output,getenv("HOME"));
                strcat(output,"\n");
                time_t currentTime;
                time(&currentTime);
                strcat(output,ctime(&currentTime));
                int length = snprintf( NULL, 0, "%d", currentProcesses );
                char* str = malloc( length + 1 );
                snprintf( str, length + 1, "%d", currentProcesses );
                strcat(output,str);
                strcat(output,"\n");
                
                if(redirectProcess==1 | redirectProcess == 2) {
                    FILE * outputFile;
                    char * fileName = argslist[red];
                    
                    outputFile = fopen(fileName, redirectProcess== 1 ? "w" : "a+");
                    if (outputFile == NULL) {
                        perror("open");
                        break;
                    }

                    fprintf(outputFile,output);
                    fclose(outputFile);
                }
                else if(redirectProcess==3) {
                    FILE * outputFile;
                    char * fileName = argslist[red];
                    
                    outputFile = fopen(fileName, "a+");
                    if (outputFile == NULL) {
                        perror("open");
                        break;
                    }
                    
                    
                    int start = 0;
                    int end = strlen(output) - 1;
                    
                    while (start < end) {
                        
                        char temp = output[start];
                        output[start] = output[end];
                        output[end] = temp;

                        ++start;
                        --end;
                    }

                    fprintf(outputFile,output);
                    fclose(outputFile);

                }
                else {
                    printf("%s",output);
                    
                }
                
                free(str);
                currentProcesses-=1;
                exit(EXIT_SUCCESS);
                
            }

            int status;
            pid_t t = waitpid(-1,&status,WNOHANG);
        

            while(t>0) {
                printf("[%d]  Done \t%d\n", backprocess,t);
                currentProcesses --;
                backprocess --;
                
                t = waitpid(-1,&status,WNOHANG);
            }
            if(!backgroundProcess) {
                pid_t t= waitpid(pid, &status, 0);
                currentProcesses --;
            }

            strcpy(lastCommand,currentCommand);
            
            
            continue;
        }

        strcpy(lastCommand,currentCommand);
        
        
        //FORK A NEW CHILD
        pid_t pid = fork();

        if(backgroundProcess&pid!=0) {printf("[%d] %d\n",backprocess,pid);}
        int s = -1;
        if(pid == 0) {
            
            int found = 0;
            for(int i =0; i<path_number; i++) {
                //TRY TO FIND IT IN THE PATH LIST, IF FOUND, A = 0 
                char * path_name = paths[i];
                strcat(path_name,"/");
                strcat(path_name,argslist[0]);
                int a = access(path_name,X_OK);
                
                if(a==0) {
                    
                    found = 1;
                    
                    if(redirectProcess==1 | redirectProcess==2) {
                        //IF REDIRECTPROCESS IS  > OR >> IT OPENS THE FILE WITH ACCORDING FLAGS AS A PIPE, 
                        //REDIRECTS THE STANDART OUTPUT TO THIS FILE. 
                        
                        int outputFile;
                        char * fileName = argslist[red];
                        int openFlags = redirectProcess==2 ? (O_WRONLY | O_CREAT | O_APPEND) : (O_WRONLY | O_CREAT | O_TRUNC);
                        outputFile = open(fileName, openFlags, 0644);
                        if (outputFile == -1) {
                            perror("open");
                            break;
                        }
                        int savedStdout = dup(STDOUT_FILENO);
                        
                        if (dup2(outputFile, STDOUT_FILENO) == -1) {
                            perror("dup2");
                            exit(EXIT_SUCCESS);
                        }

                        close(outputFile);
                        argslist[red-1] = NULL;
                        
                        execv(path_name,argslist);

                        dup2(savedStdout, STDOUT_FILENO);
                        

                        close(savedStdout);
                        
                        perror("execv");
                        exit(EXIT_SUCCESS);

                    }

                    else if(redirectProcess==3) {
                        //IF THE REDIRECTION IS >>> IT OPENS THE FILE, GETS THE OUTPUT WITH popen THEN REVERSES THE STRING
                        //AND APPENDS IT TO THE OPENED FILE.
                        FILE * outputFile;
                        char * fileName = argslist[red];
                        
                        outputFile = fopen(fileName, "a+");
                        if (outputFile == NULL) {
                            perror("open");
                            break;
                        }

                        argslist[red-1] = NULL;

                        for(int i = 1; argslist[i]!= NULL; i++) {
                            strcat(path_name, " ");
                            strcat(path_name,argslist[i]);
                        }
                        

                        char buf[1024];
                        int i = 0;
                        FILE *p = popen(path_name,"r");
                        if (p != NULL )
                        {
                            while (!feof(p) && (i < 99) )
                            {
                                fread(&buf[i++],1,1,p);
                            }
                            buf[i] = 0;
                            
                            int start = 0;
                            int end = i - 2;
                            
                            while (start < end) {
                                
                                char temp = buf[start];
                                buf[start] = buf[end];
                                buf[end] = temp;

                                ++start;
                                --end;
                            }

                            fprintf(outputFile,buf);
                            

                            pclose(p);
                            return 0;
                        }
                        else
                        {
                            perror("Error: ");
                            exit(EXIT_SUCCESS);
                        }
                        fclose(outputFile);



                    }                   
                
                    else {
                        //EXECUTE WITH THE GIVEN ARGUMENTS IF NO REDIRECTION IS GIVEN.
                        execv(path_name,argslist);
                        exit(EXIT_SUCCESS);
                        perror("Error");
                        exit(EXIT_SUCCESS);
                        break;
                    }
                    
                }
            }
            if(found == 0) {
                //IF NOT FOUND IN THE PATH LIST, GIVE ERROR
                perror("Error");
                exit(EXIT_SUCCESS);
            }
            
        }
        
        int status;
        
        // WAITPID
        pid_t t = waitpid(-1,&status,WNOHANG);
        

        while(t>0) {
            printf("[%d]  Done \t%d\n", backprocess,t);
            currentProcesses --;
            backprocess --;
            
            t = waitpid(-1,&status,WNOHANG);
        }
            
        //ALSO WAIT THE PID IF ITS NOT A BACKGROUND PROCESS
        if(!backgroundProcess) {
            pid_t t= waitpid(pid, &status, 0);
            currentProcesses --;
        }


        
        for (int i = 0; argslist[i] != NULL; i++) {
            //free(argslist[i]);
        }
        free(argslist);

        if(WTERMSIG(status)) {
            currentProcesses --;
        }
        

    }
    free(head);
   return 0;
}
