#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "nethervm/nethervm.h"

static NVM* vm;
static int counter = 0;

static void builtin_counter_increase(NVM* qcvm)
{
    int value = G_INT(OFS_PARM0);
    counter += value;
}

static void builtin_print(NVM* qcvm)
{
    const char* msg = G_STRING(OFS_PARM0);
    printf("%s\n", msg);
}

static void* alloc_callback(NVM* vm, void* oldptr, size_t size, const char* name)
{
    if (oldptr == NULL) {
        printf("Allocating %zu bytes for '%s'\n", size, name);
        return malloc(size);
    }
    else if (size > 0) {
        printf("Re-allocating %zu bytes for '%s'\n", size, name);
        return realloc(oldptr, size);
    }
    else {
        printf("Freeing %zu bytes for '%s'\n", size, name);
        free(oldptr);
        return NULL;
    }
}

static void print_callback(NVM* vm, const char* msg, bool debug)
{
    printf("%s", msg);
}

static void error_callback(NVM* vm, const char* msg)
{
    fprintf(stderr, "NVM error: %s\n", msg);
    exit(EXIT_FAILURE);
}

static bool ReadFile(const char* filename, char** data, size_t* size)
{
    FILE *f = fopen(filename, "rb");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);  /* same as rewind(f); */
    
    char* content = malloc(fsize + 1);
    fread(content, 1, fsize, f);
    fclose(f);

    content[fsize] = 0;

    *data = content;
    *size = fsize;
}

int main(int argc, char** argv)
{
    const char* progs_filename = "progs.dat";
    if (argc > 1)
    {
        progs_filename = argv[1];
    }

    char* progs_data = NULL;
    size_t progs_size = 0;
    if (!ReadFile(progs_filename, &progs_data, &progs_size)) return 1;

    vm = nvmCreateVM(alloc_callback, print_callback, error_callback, NULL);
    if (nvmLoadProgs(vm, progs_filename, progs_data, progs_size, true))
    {
        nvmAddExtBuiltin(vm, 0, "counter_increase", builtin_counter_increase);
        nvmAddExtBuiltin(vm, 0, "print", builtin_print);
        nvmExecuteFunction(vm, nvmFindFunction(vm, "test_main"));
    }
    nvmDestroyVM(vm);
    free(progs_data);

    //assert(counter == 2);
    return 0;
}