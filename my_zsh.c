#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "my_zsh.h"


int main(int argc, char** argv, char** envp) {

    if (argc > 1) {
        return 1;
    }
    (void)argv;

    //creates copy of the environment to use with env, setenv, unsetenv commands
    char** envp_copy = copy_envp(envp, 0);
    
    //creates a pointer that will store the address of a previous directory when cd is called
    char buf[500];
    getcwd(buf, 500);  
    char* prev_dir = malloc((my_strlen(buf) +1) * sizeof(char));
    my_strcpy(prev_dir, buf);    

    while (true) {

        write(1, "[my_zsh]> ", 10);
        
        //reads input from stdin
        char *buffer = NULL;
        size_t len = 0;
        ssize_t bytes_read = getline(&buffer, &len, stdin);

        //checks for read errors and EOF
        if (bytes_read < 0) {
            free(buffer);
            free_string_array(envp_copy);
            free(prev_dir);
            return 1;
        }

        //replaces '\n' with '\0'
        newline_to_endline(buffer);
        
        if (check_quote_marks(buffer) == -1) {
            write(2, "error: missing closing quote mark\n", 34);
            free(buffer);
            continue;
        }

        //removes extra spaces
        char* input = remove_extra_spaces(buffer);

        //edge case for empty input
        if (input[0] == '\0') {
            free(buffer);
            free(input);
            continue;
        }    

        //sets up string array struct that will be filled with strings from input
        str_array input_words;
        split_str(&input_words, input, ' ');

        //checks for exit or quit signal
        if (input_words.array[1] == NULL && (my_strcmp(input_words.array[0], "exit") == 0 || my_strcmp(input_words.array[0], "quit") == 0)) {
            free(buffer);
            free_string_array(envp_copy);
            free_string_array(input_words.array);
            free(input);
            free(prev_dir); 
            break;
        }
        
        //checks if input contains builtin function
        if (is_builtin(&input_words, 0) == 1){
            
            //runs builtin
            run_builtin(&input_words, &envp_copy, &prev_dir);

        //if it's not a builtin, checks if it's a binary
        } else {

            char* bin_path = NULL;
            int bin_path_malloc = 0;
            str_array path_array;

            //checks if the argument provided is a path
            if (input_words.array[0][0] == '/' || (input_words.array[0][0] == '.' && input_words.array[0][1] == '/')) {
                struct stat sb;
                if (stat(input_words.array[0], &sb) != -1) {
                    if (S_ISREG(sb.st_mode)) {
                        bin_path = input_words.array[0];
                    }
                }
            //if it's not a path, checks if it's a binary
            } else {
                //gets pointer to start of path string 
                char* path_str = find_path_start(envp_copy);
                    
                //splits path str into array of paths
                split_str(&path_array, path_str, ':');

                //checks paths to find if binary is located in one of them
                bin_path = binary_location(&path_array, input_words.array[0]);
                bin_path_malloc = 1;
            }
            
            //if it's not an executable or binary, print error message and restart loop
            if (bin_path == NULL) {
                write(2, "command not found: ", 19);
                write(2, input_words.array[0], my_strlen(input_words.array[0]));
                write(2, "\n", 1);
            } else {   
            //if it's an executable or binary, fork and execute binary
                pid_t pid;
                int status;               
                pid = fork();

                if (pid == 0) {
                    //child process runs     
                    if (execve(bin_path, input_words.array, envp_copy) == -1) {
                        perror("lsh");
                        exit(EXIT_FAILURE);
                    }
                } else {
                    //parent process waits for child to end and then continues
                    waitpid(pid, &status, 0);
                    //prints if the child program segfaults
                    if (WIFSIGNALED(status)) {
                        if (WTERMSIG(status) == SIGSEGV) {
                            write(2, "segmentation fault\n", 19);
                        }
                    }
                }
                
                if (bin_path_malloc == 1) {
                    free(bin_path);
                }
            }
            
            if (bin_path_malloc == 1) {
                free_string_array(path_array.array);
            }
            
        }

        free_string_array(input_words.array);
        free(input);
        free(buffer);     
    }
}


//////FUNCTIONS//////

//customized strlen()
int my_strlen(char* str) {
    int i = 0;
    while (str[i] != '\0') {
        i++;
    }
    return i;
}

//customized version of strcmp(). returns 0 for equal strings, -1 for unequal strings
int my_strcmp (char* str1, char* str2) {
    int i = 0;
    while (str1[i] != '\0' || str2[i] != '\0') {
        if (str1[i] != str2[i]) {
            return -1;
        }
        i++;
    }
    if (str1[i] == '\0' && str2[i] == '\0') {
        return 0;
    }
    return -1;
}

//customized strcpy()
char* my_strcpy(char* dest, char* src) {
    int pos = 0;
    for (int i = 0; src[i] != '\0'; i++) {
        dest[i] = src[i];
        pos = i + 1;
    }
    dest[pos] = '\0';
    return dest;
}


//looks for '\n' char and replaces it with '\0'
void newline_to_endline(char* str) {
    int i = 0;
    while (str[i] != '\0') {
        if (str[i] == '\n') {
            str[i] = '\0';
            return;
        }
        i++;
    }
}

//copies the string while removing trailing spaces and extra spaces
char* remove_extra_spaces(char* str) {
    int len = my_strlen(str);
    char* result = malloc((len + 1) * sizeof(char));
    int pos = 0;

    for (int i = 0; i < len; i++) {
        if (str[i] != ' ' && str[i] != '"') {
            result[pos] = str[i];
            pos++;
        } else if (str[i] == '"') {
            result[pos] = str[i];
            pos++;
            i++;
            while (str[i] != '"') {
                result[pos] = str[i];
                pos++;
                i++;
            }
            result[pos] = str[i];
            pos++;
        } else if (str[i] == ' ') {
            if (i != 0 && i + 1 < len && pos > 0 && str[i + 1] != ' ') {
                result[pos] = str[i];
                pos++;
            }
        }
    }    
    result[pos] = '\0';

    //printf("%s\n", result);

    return result;
}

//splits input string into array of strings. accepts char on which to split
void split_str(str_array* input, char* str, char c) {
    //counts number of words (spaces + 1)
    int len = my_strlen(str);
    int words = 0;
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '"') {
            i++;
            while (str[i] != '"') {
                i++;
            }
        } else if (str[i] == c) {
            words++;
        }
    }
    words++;

    if (c == ' ') {
       // printf("words: %d\n", words);
    }

    input->size = words + 1; //+1 is necessary to NULL terminate the array
    input->array = malloc(input->size * sizeof(char*));
  
    //copies separate words into array
    int str_pos = 0;
    int i = 0;
    for (i = 0; i < input->size - 1; i++) {
        input->array[i] = malloc((len + 1) * sizeof(char));
        input->array[i][0] = '\0';
        for (int j = str_pos, k = 0; j < len; j++, k++) {
            if (str[j] != c && str[j] != '"') {
                input->array[i][k] = str[j];
                if (j == len - 1) {
                    input->array[i][k + 1] = '\0';
                }
            } else if (str[j] == '"') {
                j++;
                while (str[j] != '"') {
                    input->array[i][k] = str[j];
                    k++;
                    j++;
                }
                if (j + 1 == len || str[j + 1] == ' ') {
                    input->array[i][k] = '\0';
                } else {
                    k--;
                }

            } else if (str[j] == c) {
                input->array[i][k] = '\0';
                str_pos = j + 1;
                break;
            }
        }
    }

    //null-terminates the array (this will be necessary for execve())
    input->array[input->size - 1] = NULL;
}

//concatenates two strings with an extra char inserted between them
char* concat_with_insert(char* str1, char* str2, char c) {
    //size of result is number of chars in dir + number of chars in file + c + '\0'
    int len_str1 = my_strlen(str1);
    int len_str2 = my_strlen(str2);
    char* result = malloc((len_str1 + len_str2 + 2) * sizeof(char));

    //copies first part into result, copies separatos, then copies second part
    int pos = 0;
    for (int i = 0; i < len_str1; i++) {
        result[pos] = str1[i];
        pos++;
    }

    result[pos] = c;
    pos++;

    for (int i = 0; i < len_str2; i++) {
        result[pos] = str2[i];
        pos++;
    }
    result[pos] = '\0';

    return result;
}



//checks if input contains builtin command. yes = 1, no = -1
int is_builtin(str_array* input, int n) {
    char* builtins[] = {"echo", "cd", "setenv", "unsetenv", "env", "exit", "pwd", "which", "quit", NULL};

    int i = 0;
    while (builtins[i] != NULL) {
        if (my_strcmp(input->array[n], builtins[i]) == 0) {
            return 1;
        }
        i++;
    }
    return -1;
}

//determines which builtin command was called
void run_builtin(str_array* input, char*** envp, char** path) {
    if (my_strcmp(input->array[0], "exit") == 0 && input->array[1] != NULL) {
        write(2, "exit: wrong syntax\n", 20);
        return;
    }

    if (my_strcmp(input->array[0], "quit") == 0 && input->array[1] != NULL) {
        write(2, "quit: wrong syntax\n", 20);
        return;
    }
    
    if (my_strcmp(input->array[0], "pwd") == 0) {
        do_pwd(input);
        return;
    }

    if (my_strcmp(input->array[0], "echo") == 0) {
        do_echo(input, *envp);
        return;
    }

    if (my_strcmp(input->array[0], "cd") == 0) {
        do_cd(path, input);
        return;
    }

    if (my_strcmp(input->array[0], "env") == 0) {
        do_env(*envp, input);
        return;
    }

    if (my_strcmp(input->array[0], "setenv") == 0) {
        do_setenv(input, envp);
        return;
    }

    if (my_strcmp(input->array[0], "unsetenv") == 0) {
        do_unsetenv(*envp, input);
        return;
    }

    if (my_strcmp(input->array[0], "which") == 0) {
        do_which(input, *envp);
        return;
    }

}

//sets the value of an environment variable
void do_setenv(str_array* input, char*** envp) {
    
    //checks if number of args is correct
    if (input->array[1] == NULL || input->array[2] != NULL) {
        write(2, "setenv: wrong syntax\n", 21);
        return;
    } 

    //checks if formatting is correct: KEY=VALUE
    int found = 0;
    for (int i = 0; i < my_strlen(input->array[1]); i++) {
        if (input->array[1][i] == '=' && i != 0) {
            found++;
        }
    }
    if (found != 1) {
        write(2, "setenv: wrong syntax\n", 21);
        return;
    }

    //splits var and value into separate strings
    str_array key_value;
    split_str(&key_value, input->array[1], '=');

    //looks for variable in envp array
    int i = 0;
    char* match = NULL;
    while ((*envp)[i] != NULL) {
        match = find_env_var((*envp)[i], key_value.array[0]); 
        if (match != NULL) {
            break;
        }
        i++;
    }
    
    //declares new string that will replace the existing string
    char* new = NULL;

    //checks if value has been provided, if value is empty, adds nothing after '='
    if (key_value.array[1] != NULL) {
        new = concat_with_insert(key_value.array[0], key_value.array[1], '=');
    } else {
        new = concat_with_insert(key_value.array[0], "", '=');
    }

    //if variable exists, replaces string with variable with the new string
    if (match != NULL) {
        free((*envp)[i]);
        (*envp)[i] = new;
    //if variable doesn't exist, appends new string to the end of envp
    //to do this a new copy of envp has to be made to allocate an extra space for the new string
    } else if (match == NULL) {
        char** envp_copy = copy_envp(*envp, 2);

        int k = 0;
        while ((*envp)[k] != NULL) {
            free((*envp)[k]);
            k++;
        }
        free(*envp);

        *envp = envp_copy;
        
        int j = 0;
        while ((*envp)[j] != NULL) {
            j++;
        }
        (*envp)[j] = new;
        (*envp)[j + 1] = NULL;
    }

    free_string_array(key_value.array);
}


void do_echo(str_array* input, char** envp) {
    //returns empty line if there are no arguments
    if (input->array[1] == NULL) {
        write(1, "\n", 1);
        return;
    }
    
    //special case to print out value assigned to environment variable
    if (input->array[1][0] == '$' && input->array[2] == NULL) {
        int i = 0;
        char* match = NULL;
        while (envp[i] != NULL) {
            match = find_env_var(envp[i], &input->array[1][1]);
            if (match != NULL) {
                write(1, match, my_strlen(match));
                write(1, "\n", 1);
                return;
            } 
            i++;
        }
        if (match == NULL) {
            write(1, "\n", 1);
            return;
        }
    }    

    //general case for all strings: prints out whatever was written after "echo"
    int i = 1;
    while (input->array[i] != NULL) {
        if (i > 1) {
            write(1, " ", 1);
        }
        write(1, input->array[i], my_strlen(input->array[i]));
        i++;
    }
    write(1,"\n", 1);
}

//prints out all environment variables and their values
void do_env(char** envp, str_array* input) {
    if (input->array[1] != NULL) {
        write(2, "env: wrong syntax\n", 18);
        return;
    }

    int i = 0;
    while (envp[i] != NULL) {
        write(1, envp[i], my_strlen(envp[i]));
        write(1, "\n", 1);
        i++;
    }
}

//removes environment variable and its values from envp array
void do_unsetenv(char** envp, str_array* input) {
    //check if there's exactly one variable
    if (input->array[1] == NULL || input->array[2] != NULL) {
        write(2, "unsetenv: wrong syntax\n", 23);
        return;
    }

    //looks for variable in array
    int i = 0;
    char* match = NULL;
    while (envp[i] != NULL) {
        match = find_env_var(envp[i], input->array[1]); 
        if (match != NULL) {
            break;
        }
        i++;
    }
    
    if (match == NULL) {
        return;
    }

    //frees the string with corresponding variable
    free(envp[i]);

    //loops through the following strings and shifts them one space to the left
    int j = i;
    int k = i + 1;
    while (envp[k] != NULL) {
        envp[j] = envp[k];
        j++;
        k++;
    }
    envp[j] = NULL;
}

//changes directory. receives a pointer to a pointer (char**) because the whole string may need to be freed and malloc'ed
void do_cd(char** prev_dir, str_array* input) {
    
    //checks that cd has one argument
    if (input->array[1] == NULL || input->array[2] != NULL) {
        write(2, "cd: wrong syntax\n", 17);
        return;
    }
    
    //"cd -" = change directory to previous location
    if (my_strcmp(input->array[1], "-") == 0) {
        //saves current path
        char buf[500];
        getcwd(buf, 500);
        char* new_dir = malloc((my_strlen(buf) +1) * sizeof(char));
        my_strcpy(new_dir, buf);       
        
        //changes directory to previous path
        int chdir_result = chdir(*prev_dir);
        if (chdir_result == -1) {
            write(2, "cd: no such file or directory: ", 31);
            write(2, input->array[1], my_strlen(input->array[1]));
            write(2, "\n", 1);
            free(new_dir);
        
        //if successful, saves new value for previous path
        } else {
            write(1, *prev_dir, my_strlen(*prev_dir));
            write(1, "\n", 1);
            free(*prev_dir);
            *prev_dir = new_dir;
        }

    //for regular cd calls with path, saves current directory, checks if path exists, if yes, changes directory
    } else {
        //saves current path
        char buf[500];
        getcwd(buf, 500);
        char* new_dir = malloc((my_strlen(buf) +1) * sizeof(char));
        my_strcpy(new_dir, buf);

        //changes directory to provided path
        int chdir_result = chdir(input->array[1]);
        if (chdir_result == -1) {
            write(2, "cd: no such file or directory: ", 31);
            write(2, input->array[1], my_strlen(input->array[1]));
            write(2, "\n", 1);
            free(new_dir);
        
        //if successful, saves new value for previous path
        } else {
            free(*prev_dir);
            *prev_dir = new_dir;
        }
    }
}

//prints out current directory
void do_pwd(str_array* input) {
    //checks that there are no arguments
    if (input->array[1] != NULL) {
        write(2, "pwd: wrong syntax\n", 18);
        return;
    }
    
    //prints out current location
    char buf[500];
    char* pwd_result = getcwd(buf, 500);
    write(1, pwd_result, my_strlen(pwd_result));
    write(1, "\n", 1);
}

//checks if function is builtin or binary
void do_which(str_array* input, char** envp_copy) {
    //loop through args 1 till NULL ("which" can take multiple arguments)
    int i = 1;
    while (input->array[i] != NULL) {
        //checks if it's a builtin
        if (is_builtin(input, i) == 1){          
            write(1, input->array[i], my_strlen(input->array[i]));
            write(1, ": shell built-in command\n", 25);
        
        //if it's not a builtin, checks if it's an executable file or binary
        } else {

            char* bin_path = NULL;
            int bin_path_malloc = 0;
            str_array path_array;

            //if string starts with ./ or /, checks if file with such path exists
            if (input->array[i][0] == '/' || (input->array[i][0] == '.' && input->array[i][1] == '/')) {
                struct stat sb;
                if (stat(input->array[i], &sb) != -1) {
                    if (S_ISREG(sb.st_mode)) {
                        bin_path = input->array[i];
                    }
                }
            } else {
                //gets pointer to start of path string 
                char* path_str = find_path_start(envp_copy);
                    
                //splits path str into array of paths
                split_str(&path_array, path_str, ':');

                bin_path = binary_location(&path_array, input->array[i]);
                bin_path_malloc = 1;
            }
            
            //if it's not an executable file or binary, print error message and restart loop
            if (bin_path == NULL) {
                write(1, input->array[i], my_strlen(input->array[i]));
                write(1, " not found\n", 11);
            //else print out path of executable
            } else {
                write(1, bin_path, my_strlen(bin_path));
                write(1, "\n", 1);
            }

            if (bin_path_malloc == 1) {
                free_string_array(path_array.array);    
                free(bin_path);
            }
        }
        i++;
    }
}

//returns pointer to start of PATH variables
char* find_path_start(char** envp) {
    char* result = NULL;
    int i = 0;
    while (envp[i] != NULL) {
        result = find_env_var(envp[i], "PATH");
        if (result != NULL) {
            break;
        }
        i++;
    }

    return result;
}

//checks if start of envp str matches var, returns pointer to start of values upon success
char* find_env_var(char* str, char* var) {
    int i = 0;
    while (str[i] != '\0' && var[i] != '\0') {
        if (str[i] != var[i]) {
            return NULL;
        }
        i++;
    }

    if (str[i] == '=' && var[i] == '\0') {
        return &str[i + 1];
    } else {
        return NULL;
    }
}

void free_string_array(char** array) {
    int i = 0;
    while (array[i] != NULL) {
        free(array[i]);
        i++;
    }
    free(array);
}

//looks for the directory where binary function is located
char* binary_location(str_array *path_array, char* binary) {
    for (int i = 0; i < path_array->size; i++) {
        //opens directory
        DIR *directory = opendir(path_array->array[i]);
        if (directory == NULL) {
            continue;
        }

        //loops through entries of directory
        struct dirent *entry = readdir(directory);
        while (entry != NULL) {
            //if file with name of binary is found, returns concatenated path to binary
            if (my_strcmp(entry->d_name, binary) == 0) {
                char* result = concat_with_insert(path_array->array[i], binary, '/');
                closedir(directory);
                return result;
            }
            entry = readdir(directory);
        }
        closedir(directory);
    }
    return NULL;
}

//function that copies the environment for further modifications
//receives an int that can be used to allocate extra space in the array, e.g. to be able to run setenv which may add an extra string
char** copy_envp(char** envp, int extra) {
    //counts number of variables
    int count = 0;
    while (envp[count] != NULL) {
        count++;
    }

    char** result = malloc((count + 1 + extra) * sizeof(char*));
    
    //copies variables
    int i = 0;
    while (envp[i] != NULL) {
        result[i] = malloc((my_strlen(envp[i]) + 1) * sizeof(char));
        my_strcpy(result[i], envp[i]);
        i++;
    }

    result[i] = NULL;
    return result;
}

//checks for correct use of quote marks
int check_quote_marks(char* str) {
    int open_quote = 0;
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '"') {
            open_quote = !open_quote;
        }
    }
    if (open_quote != 0) {
        return -1;
    }
    return 0;
}
