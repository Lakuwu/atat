// gcc -o atat.exe atat.c -std=c23 -fcf-protection -Wall -Wextra -Wpedantic -g -fsanitize=undefined -fsanitize-trap=all -O0 && atat file_a.txt output.txt
// gcc -o atat.exe atat.c -std=c23 -fcf-protection -Wall -Wextra -Wpedantic -g -fsanitize=undefined -fsanitize-trap=all -O0 && gdb -q -ex run --args atat file_a.txt output.txt
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define FILE_IMPLEMENTATION
#include "../uwulib/file.h"

#include "../uwulib/macro.h"

#define FILE_DEPTH 32
char input_files[FILE_DEPTH][FILENAME_MAX];
int file_i;

int arg_count;
char **arg_str;
size_t *arg_len;

void push_file(const char* filename) {
    if(file_i < FILE_DEPTH) {
        strncpy(input_files[file_i++], filename, FILENAME_MAX);
    } else {
        file_i++;
    }
}

void pop_file() {
    if(file_i) file_i--;
}

void usage(void) {
    puts("atat build " __DATE__ " " __TIME__);
    puts("Usage: atat input_file output_file [arg0 arg1 ...]");
}

void process(FILE *fp_out, char *buf, size_t size);

void command_insert(FILE *fp_out, const char *filename) {
    uwu_entry;
    for(int i = 0; i < file_i; ++i) {
        if(!strncmp(input_files[i], filename, FILENAME_MAX)) {
            ERRORF("Duplicate filename \"%s\" found while processing. Are you sure you want an infinite loop?", filename);
            uwu_return;
        }
    }
    size_t size = 0;
    char *buf = file_read(filename, &size);
    if(!buf) {
        ERRORF("Could not read file \"%s\" for insertion", filename);
        uwu_return;
    }
    if(size == 0) {
        WARN("Empty file");
        uwu_return;
    }
    
    push_file(filename);
    process(fp_out, buf, size);
    pop_file();
    free(buf);
    uwu_return;
}

int get_arg_idx(const char *str) {
    uwu_entry;
    int idx;
    int ret = sscanf(str, "%d", &idx);
    if(ret < 0) {
        
        uwu_return -1;
    }
    if(idx < 0) {
        ERROR("arg index can't be negative");
        uwu_return -1;
    }
    if(idx >= arg_count) {
        ERRORF("not enough arguments given; need %d, have %d", idx + 1, arg_count);
        uwu_return -1;
    }
    uwu_return idx;
}

void do_command(FILE *fp_out, char *command) {
    uwu_entry;
    char *tok = strtok(command, " ");
    if(!tok) {
        WARN("Empty command");
        uwu_return;
    }
    
    enum {
        C_UNKNOWN,
        C_INSERT,
        C_ARG,
        C_INSERT_ARG,
    } cmd = C_UNKNOWN;
    
    if(!strcmp("insert", tok)) cmd = C_INSERT;
    else if(!strcmp("include", tok)) cmd = C_INSERT;
    else if(!strcmp("arg", tok)) cmd = C_ARG;
    else if(!strcmp("insert_arg", tok)) cmd = C_INSERT_ARG;
    else if(!strcmp("include_arg", tok)) cmd = C_INSERT_ARG;
    
    switch(cmd) {
    case C_INSERT: {
        tok = strtok(NULL, "\"");
        if(!tok) {
            ERROR("Could not find filename token");
            uwu_return;
        }
        command_insert(fp_out, tok);
        break;
    }
    case C_ARG: {
        tok = strtok(NULL, "");
        if(!tok) {
            ERROR("Missing arg index");
            uwu_return;
        }
        int idx = get_arg_idx(tok);
        if(idx >= 0) fwrite(arg_str[idx], 1, arg_len[idx], fp_out);
        break;
    }
    case C_INSERT_ARG: {
        tok = strtok(NULL, "");
        if(!tok) {
            ERROR("Missing arg index");
            uwu_return;
        }
        int idx = get_arg_idx(tok);
        if(idx >= 0) command_insert(fp_out, arg_str[idx]);
        break;
    }
    default: {
        WARNF("Unknown command \"%s\"", tok);
        uwu_return;
    }
    }
    uwu_return;
}

void process(FILE *fp_out, char *buf, size_t size) {
    uwu_entry;
    size_t i = 0;
    size_t write_begin = 0, write_end = 0;
    size_t command_begin = 0, command_end = 0;
    enum {
        S_DEFAULT,
        S_TRY_BEGIN,
        S_BEGIN,
        S_COMMAND,
        S_TRY_END,
        S_END,
    } state = S_DEFAULT;
    char *command = NULL;
    while(i < size) {
        switch(state) {
        
        case S_END: {
            state = S_DEFAULT;
            write_begin = i;
        } [[fallthrough]];
        
        case S_DEFAULT: {
            if(buf[i] == '@') state = S_TRY_BEGIN;
            break;
        }
        
        case S_TRY_BEGIN: {
            if(buf[i] == '@') {
                state = S_BEGIN;
                write_end = i - 1;
                
            } else {
                state = S_DEFAULT;
            }
            break;
        }
        
        case S_BEGIN: {
            state = S_COMMAND;
            command_begin = i;
        } [[fallthrough]];
        
        case S_COMMAND: {
            if(buf[i] == '@') state = S_TRY_END;
            break;
        }
        
        case S_TRY_END: {
            if(buf[i] == '@') {
                state = S_END;
                command_end = i - 1;
                fwrite(buf + write_begin, 1, write_end - write_begin, fp_out);
                command = buf + command_begin;
                *(buf + command_end) = '\0';
                do_command(fp_out, command);
            } else {
                state = S_COMMAND;
            }
            break;
        }
            
        }
        
        ++i;
    }
    if(state != S_END) {
        write_end = size;
        fwrite(buf + write_begin, 1, write_end - write_begin, fp_out);
    }
    uwu_return;
}

int main(int argc, char **argv) {
    uwu_entry;
    if(argc < 3) {
        usage();
        return EXIT_SUCCESS;
    }
    
    char *input_file = argv[1];
    char *output_file = argv[2];
    
    if(argc > 3) {
        arg_count = argc - 3;
        arg_len = calloc(arg_count, sizeof(size_t));
        arg_str = argv + 3;
        for(int i = 0; i < arg_count; ++i) {
            arg_len[i] = strlen(arg_str[i]);
        }
    }
    
    size_t size = 0;
    char *buf = file_read(input_file, &size);
    if(!buf) {
        ERRORF("Could not read input file \"%s\"", input_file);
        return EXIT_FAILURE;
    }
    
    FILE *fp_out = fopen(output_file, "wb");
    if(!fp_out) {
        ERRORF("Could not open output file \"%s\"", output_file);
        return EXIT_FAILURE;
    }
    
    push_file(input_file);
    process(fp_out, buf, size);

    free(buf);
    fclose(fp_out);
    
    return EXIT_SUCCESS;
}