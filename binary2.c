#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define STACK_SIZE          1000  // Default loop stack size
#define STACK_GROWTH_FACTOR 0.1   // How much stack to add when its full

void compile(char *asm_filename, char *src_filename);
char *string(char *str);
char *replace_extension(char *name, char *ext);

enum stage {
    COMPILE,   // Compile only
    ASSEMBLE,  // Compile and assemble only
    LINK       // Compile, assemble and link
};

struct info_t {
    char *pname;         // Process name
    char *ifilename;     // Input source code file name
    char *ofilename;     // Output file name
    enum stage ostage;   // Final stage that generates the output file
    char *arr_size;      // Memory allocated for the executable
} info;

int main(int argc, char **argv)
{
    int verbose = 0;               // 1 enables verbosity; 0 disables
    char *arr_size = "30000";      // Default size of array of cells
    char *asm_filename;            // File name for assembly code
    char *obj_filename;            // File name for object code
    char *exe_filename = "out";    // File name for executable code
    char *command;                 // Buffer for command line strings
    size_t i;                      // Counter
    size_t len;                    // Stores string lengths

    if ((info.pname = strrchr(argv[0], '/')) == NULL) {
        info.pname = argv[0];
    } else {
        info.pname++; // Address of the basename part in argv[0]
    }
    info.ifilename = NULL;
    info.ofilename = NULL;
    info.ostage = LINK; 
    info.arr_size = arr_size;

    if (argc < 2) {
    fprintf(stderr, "Usage: %s file.bin2\n", info.pname);
    exit(EXIT_FAILURE);
    }

    info.ifilename = argv[1];


    // Compiling

    // Determine name for assembly code filename
    if (info.ostage == COMPILE && info.ofilename != NULL) {
        asm_filename = string(info.ofilename);
    } else {
        asm_filename = replace_extension(info.ifilename, "s"); 
    }

    if (verbose) {
        printf("Compiling: compile(\"%s\", \"%s\")\n",
               asm_filename, info.ifilename);
    }
    compile(asm_filename, info.ifilename);

    // If compile only option was specified, exit
    if (info.ostage == COMPILE) {
        free(asm_filename);
        exit(EXIT_SUCCESS);
    }

    // Assembling 

    // Determine name for object code filename
    if (info.ostage == ASSEMBLE && info.ofilename != NULL) {
        obj_filename = string(info.ofilename);
    } else {
        obj_filename = replace_extension(info.ifilename, "o"); 
    }

    // as
    len = strlen("as -o") + strlen(asm_filename) +
          strlen(obj_filename) + 2;
    if ((command = malloc(len)) == NULL) {
        fprintf(stderr, "%s: Out of memory while assembling", info.pname);
    }
    sprintf(command, "as -o %s %s", obj_filename, asm_filename);

    // Assemble the asm code into its object file*/
    if (verbose) {
        printf("Assembling: %s\n", command);
    }
    system(command);
    free(command);

    // Assembly code file is not required after assembling
    unlink(asm_filename);
    free(asm_filename);

    // Link object file

    // Determine name for executable code filename
    if (info.ostage == LINK && info.ofilename != NULL) {
        exe_filename = info.ofilename;
    }

    // ld
    len = strlen("ld -o") + strlen(obj_filename) +
          strlen(exe_filename) + 2;
    if ((command = malloc(len)) == NULL) {
        fprintf(stderr, "%s: Out of memory while compiling", info.pname);
    }
    sprintf(command, "ld -o %s %s", exe_filename, obj_filename);

    // Link the object code to executable code
    if (verbose) {
        printf("Linking: %s\n", command);
    }
    system(command);
    free(command);

    // Object code file is not needed after linking
    unlink(obj_filename);
    free(obj_filename);
    
    exit(EXIT_SUCCESS);
}

char *string(char *str) {
    char *new_str;
    if ((new_str = malloc(strlen(str) + 1)) == NULL) {
        fprintf(stderr, "%s: Out of memory while allocating memory for "
                        "string: %s\n", info.pname, str);
        exit(1);
    }
    strcpy(new_str, str);
    return new_str;
}

char *replace_extension(char *name, char *ext) {
    char *dot = strrchr(name, '.');
    char *new_name;
    size_t len = dot == NULL ? strlen(name) : dot - name;

    if ((new_name = malloc(len + strlen(ext) + 2)) == NULL) {
        fprintf(stderr, "%s: Out of memory while changing extension of "
                        "%s to %s\n", info.pname, name, ext);
        exit(1);
    }

    strncpy(new_name, name, len);
    new_name[len] = '\0';
    strcat(new_name, ".");
    strcat(new_name, ext);
    return new_name;
}

void compile(char *asm_filename, char *src_filename) {

    FILE *src;                      // Source code file
    FILE *as;                       // Assembly code file
    size_t *stack;                  // Loop stack
    size_t top = 0;                 // Next free location in stack
    size_t stack_size = STACK_SIZE; // Stack size
    size_t loop = 0;                // Used to generate loop labels
    int c;

    // Open source file
    if ((src = fopen(src_filename, "r")) == NULL) {
        fprintf(stderr, "%s: %s: Could not read file\n",
                info.pname, src_filename);
        exit(EXIT_FAILURE);
    }

    // Open assembly file
    if ((as = fopen(asm_filename, "w")) == NULL) {
        fprintf(stderr, "%s: %s: Could not write file\n",
                info.pname, asm_filename);
        exit(EXIT_FAILURE);
    }

    // Create loop stack
    if ((stack = malloc(stack_size * sizeof *stack)) == NULL) {
        fprintf(stderr, "%s: Out of memory while creating loop stack "
                        "of size %lu\n", info.pname, stack_size);
        exit(EXIT_FAILURE);
    }

    /* Write assembly code */
    fprintf(as, ".section .bss\n");
    fprintf(as, "\t.lcomm buffer %s\n", info.arr_size);
    fprintf(as, ".section .text\n");
    fprintf(as, ".globl _start\n");
    fprintf(as, "_start:\n");
    fprintf(as, "\tmov $buffer, %%edi\n");
    while ((c = fgetc(src)) != EOF) {
        switch (c) {
         case '0':
            fprintf(as, "\tinc %%edi\n"); 
            break;
        case '1':
            fprintf(as, "\tdec %%edi\n");
            break;
        case '2':
            fprintf(as, "\tincb (%%edi)\n");
            break;
        case '3':
            fprintf(as, "\tdecb (%%edi)\n");
            break;
        case '4':
            fprintf(as, "\tmovl $3, %%eax\n");
            fprintf(as, "\tmovl $0, %%ebx\n");
            fprintf(as, "\tmovl %%edi, %%ecx\n");
            fprintf(as, "\tmovl $1, %%edx\n");
            fprintf(as, "\tint $0x80\n");
            break;
        case '5':
            fprintf(as, "\tmovl $4, %%eax\n");
            fprintf(as, "\tmovl $1, %%ebx\n");
            fprintf(as, "\tmovl %%edi, %%ecx\n");
            fprintf(as, "\tmovl $1, %%edx\n");
            fprintf(as, "\tint $0x80\n");
            break;
        case '6':
            if (top == stack_size) {
                stack_size *= 1 + STACK_GROWTH_FACTOR;
                if ((stack = realloc(stack,
                                     sizeof *stack * stack_size)) == NULL) {
                    fprintf(stderr, "%s: Out of memory while increasing "
                                    "loop stack to size: %lu\n",
                            info.pname, stack_size);
                    exit(EXIT_FAILURE);
                }
            }
            stack[top++] = ++loop;
            fprintf(as, "\tcmpb $0, (%%edi)\n");
            fprintf(as, "\tjz .LE%u\n", loop);
            fprintf(as, ".LB%u:\n", loop);
            break;
        case '7':
            fprintf(as, "\tcmpb $0, (%%edi)\n");
            fprintf(as, "\tjnz .LB%u\n", stack[--top]);
            fprintf(as, ".LE%u:\n", stack[top]);
            break;
        }
    }
    fprintf(as, "movl $1, %%eax\n");
    fprintf(as, "movl $0, %%ebx\n");
    fprintf(as, "int $0x80\n");

    fclose(as);
    fclose(src);
}