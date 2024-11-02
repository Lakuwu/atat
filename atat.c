// gcc -o atat.exe atat.c -std=c2x -fcf-protection -Wall -Wextra -Wpedantic -g -fsanitize=undefined -fsanitize-trap=all -Og && atat file_a.txt output.txt
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define FILE_IMPLEMENTATION
#include "../uwulib/file.h"

#include "../uwulib/macro.h"

void usage(void) {
    puts("atat build " __DATE__ " " __TIME__);
    puts("Usage: atat input_file output_file");
}

void do_command(char *command, FILE *fp) {
    char *tok = strtok(command, " ");
    if(!tok) {
        WARN("Empty command");
        return;
    }
    
    if(!strcmp("insert", tok)) {
        tok = strtok(NULL, "\"");
        if(!tok) {
            ERROR("Could not find filename token");
            return;
        }
        size_t size = 0;
        char *buf = file_read(tok, &size);
        if(!buf) {
            ERRORF("Could not read file \"%s\" for insertion", tok);
            return;
        }
        if(size == 0) {
            WARN("Empty file");
            return;
        }
        fwrite(buf, 1, size, fp);
        free(buf);
    } else {
        WARNF("Unknown command \"%s\"", tok);
        return;
    }
}

int main(int argc, char **argv) {
    if(argc != 3) {
        usage();
        return EXIT_SUCCESS;
    }
    
    char *input_file = argv[1];
    char *output_file = argv[2];
    
    FILE *fp_out = fopen(output_file, "wb");
    if(!fp_out) {
        ERRORF("Could not open output file \"%s\"", output_file);
        return EXIT_FAILURE;
    }
    
    size_t size = 0;
    char *buf = file_read(input_file, &size);
    if(!buf) {
        ERRORF("Could not read input file \"%s\"", input_file);
        return EXIT_FAILURE;
    }
    if(size == 0) {
        WARN("Input file is empty, doing nothing");
        return EXIT_SUCCESS;
    }
    
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
                do_command(command, fp_out);
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

    free(buf);
    fclose(fp_out);
    
    return EXIT_SUCCESS;
}