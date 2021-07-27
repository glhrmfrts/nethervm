#ifndef NETHERVM_NETHERVM_H
#define NETHERVM_NETHERVM_H

#include "types.h"

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

#endif