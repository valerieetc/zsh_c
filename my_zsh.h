#ifndef MY_ZSH_H
#define MY_ZSH_H 

typedef struct str_array {
    char** array;
    int size;
} str_array;


//<string.h> function versions
int my_strlen(char* str);
int my_strcmp(char* str1, char* str2);
char* my_strcpy(char* dest, char* src);

//string manipulation functions
void newline_to_endline(char* str);
char* remove_extra_spaces(char* str);
void split_str(str_array* input, char* str, char c);
char* concat_with_insert(char* str1, char* str2, char c);

//functions to check and run builtins
int is_builtin(str_array* input, int n);
void run_builtin(str_array* input, char*** envp, char** path);
void do_setenv(str_array* input, char*** envp);
void do_echo(str_array* input, char** envp);
void do_env(char** envp, str_array* input);
void do_unsetenv(char** envp, str_array* input);
void do_cd(char** prev_dir, str_array* input);
void do_pwd(str_array* input);
void do_which(str_array* input, char** envp_copy);

//misc
char* binary_location(str_array *path_array, char* binary);
char** copy_envp(char** envp, int extra);
void free_string_array(char** array);
char* find_path_start(char** envp);
char* find_env_var(char* str, char* var);
int check_quote_marks(char* str);


#endif