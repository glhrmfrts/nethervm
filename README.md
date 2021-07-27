# nethervm

Standalone QuakeC VM.

```c
/**
 * AllocCallback will be used in 3 different ways:
 * 
 * AllocCallback(vm, NULL, size, name) -> Allocate, should return pointer to new memory.
 * AllocCallback(vm, ptr, size, name) -> Reallocate, should reallocate memory at [ptr] and return pointer to new memory.
 * AllocCallback(vm, ptr, 0, name) -> Free memory at [ptr], should return NULL.
 * 
 * The 'vm' can be NULL for the first allocation, which allocates the NVM struct itself.
 */
typedef void*(*AllocCallback)(NVM* vm, void* ptr, size_t size, const char* name);

typedef void(*PrintCallback)(NVM* vm, const char* msg, bool debug);

typedef void(*ErrorCallback)(NVM* vm, const char* msg);

typedef void (*BuiltinFunction)(NVM* vm);

NVM* nvmCreateVM(AllocCallback acb, PrintCallback pcb, ErrorCallback ecb, void* user_data);

void nvmDestroyVM(NVM* vm);

void nvmAddExtBuiltin(NVM* qcvm, int num, const char* name, BuiltinFunction builtin);

void nvmLoadBuiltins(NVM* vm, BuiltinFunction* builtins, size_t num_builtins);

bool nvmLoadProgs(NVM* vm, const char* filename, const char* data, size_t size, bool fatal);

bool nvmAllocEdicts(NVM* vm, size_t count);

func_t nvmFindFunction(NVM* vm, const char* name);

global_t nvmFindGlobal(NVM* vm, const char* name);

field_t nvmFindField(NVM* vm, const char* name);

const char* nvmGetString(NVM* vm, int str_ofs);

void nvmExecuteFunction(NVM* vm, func_t func_ofs);
```