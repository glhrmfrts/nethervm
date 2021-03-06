#ifndef NETHERVM_NETHERVM_H
#define NETHERVM_NETHERVM_H

#include "types.h"

NVM* nvmCreateVM(AllocCallback acb, PrintCallback pcb, ErrorCallback ecb);

void nvmDestroyVM(NVM* vm);

void nvmAddExtBuiltin(NVM* qcvm, int num, const char* name, BuiltinFunction builtin);

void nvmLoadBuiltins(NVM* vm, BuiltinFunction* builtins, size_t num_builtins);

bool nvmLoadProgs(NVM* vm, const char* filename, const char* data, size_t size, bool fatal);

bool nvmAllocEdicts(NVM* vm, size_t count);

int nvmFindFunction(NVM* vm, const char* name);

int nvmFindGlobal(NVM* vm, const char* name);

int nvmFindField(NVM* vm, const char* name);

const char* nvmGetString(NVM* vm, int str_ofs);

void nvmExecuteFunction(NVM* vm, int func_ofs);

#endif