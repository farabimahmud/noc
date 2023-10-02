#ifndef _CPU_O3_SPECIAL_INST_HH
#define _CPU_O3_SPECIAL_INST_HH

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdlib.h>

#define FILENAME_SZ 64
#define BUF_SZ 256
#define PC_CHARS 8

struct sp_pc_list {
    uint64_t pc;
    char* file;
    uint64_t total_lines;
};

struct sp_pc_list* 
read_sp_insts(char *filename)
{
    FILE *fp;
    struct sp_pc_list *pc_struct = NULL;
    fp = fopen(filename, "r");
    if (fp == NULL)
    {
        printf("Could not open file %s", filename);
        return NULL;
    }

    int num_lines = 0;
    char *prog_ctr = (char *) malloc(PC_CHARS);
    memset(prog_ctr, 0, PC_CHARS);
    char *temp_name = (char *) malloc(FILENAME_SZ);
    memset(temp_name, 0, FILENAME_SZ);

    for (char c = getc(fp); c != EOF; c = getc(fp))
    {
        if (c == '\n')
        {
            num_lines++;
        }
        //printf("%c", c);
    }
    fclose(fp);

    fp = fopen(filename, "r");
    if (fp == NULL)
    {
        printf("Could not open file %s", filename);
        return NULL;
    }

    pc_struct = (struct sp_pc_list*) malloc(num_lines * sizeof(struct sp_pc_list));
    memset(pc_struct, 0, num_lines * sizeof(struct sp_pc_list));

    num_lines = 0;    
    int pc_done = 0;
    int i = 0;

    int x = 0;
    for (char c = getc(fp); c != EOF; c = getc(fp))
    {
        if (c != ':' && !pc_done)
        {
            prog_ctr[i] = c;
            i++;
            continue;
        }
        else if (!pc_done)
        {
            pc_done = 1;
            pc_struct[num_lines].pc = strtoul(prog_ctr, NULL, 16);
            continue;
        }

        //printf("0x%" PRIx64 "\n", pc_struct[num_lines].pc);
        if (c != '\n')
        {
            temp_name[x] = c;
            x++;
            continue;
        }
        else
        {
            pc_done = 0;
        }
        pc_struct[num_lines].file = (char *) malloc(FILENAME_SZ);
        memset(pc_struct[num_lines].file, 0, FILENAME_SZ);
        strcpy(pc_struct[num_lines].file, temp_name);
        num_lines++;
        i = 0;
        x = 0;
        memset(temp_name, 0, FILENAME_SZ);
    }
    fclose(fp);
    free(temp_name);
    temp_name = NULL;
    free(prog_ctr);
    prog_ctr = NULL;
    pc_struct[0].total_lines = num_lines;
    return pc_struct;
}

#endif
