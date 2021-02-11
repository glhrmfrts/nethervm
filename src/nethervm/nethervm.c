#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include "nethervm/nethervm.h"
#include "nethervm/types.h"

static const char* PR_GetString(NVM* qcvm, int ofs);

static void PR_SetEngineString(NVM* vm, const char* str);

static short LittleShort(short s)
{
    return s;
}

static long LittleLong(long l)
{
    return l;
}

static void Errorf(NVM* vm, const char* fmt, ...)
{
    char buffer[2048];
    va_list args;
    va_start(args, fmt);
    vsprintf_s(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    vm->error_callback(vm, buffer);
}

static void DPrintf(NVM* vm, const char* fmt, ...)
{
    char buffer[2048];
    va_list args;
    va_start(args, fmt);
    vsprintf_s(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    vm->print_callback(vm, buffer, true);
}

static void Printf(NVM* vm, const char* fmt, ...)
{
    char buffer[2048];
    va_list args;
    va_start(args, fmt);
    vsprintf_s(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    vm->print_callback(vm, buffer, false);
}

static edict_t *EDICT_NUM(NVM* qcvm, int n)
{
	if (n < 0 || n >= qcvm->max_edicts)
		Errorf (qcvm, "EDICT_NUM: bad number %i", n);
	return (edict_t *)((byte *)qcvm->edicts + (n)*qcvm->edict_size);
}

static int NUM_FOR_EDICT(NVM* qcvm, edict_t *e)
{
	int		b;

	b = (byte *)e - (byte *)qcvm->edicts;
	b = b / qcvm->edict_size;

	if (b < 0 || b >= qcvm->num_edicts)
		Errorf (qcvm, "NUM_FOR_EDICT: bad pointer");
	return b;
}

static dfunction_t *ED_FindFunction (NVM* qcvm, const char *fn_name)
{
	dfunction_t		*func;
	int				i;

	for (i = 0; i < qcvm->progs->numfunctions; i++)
	{
		func = &qcvm->functions[i];
		if ( !strcmp(PR_GetString(qcvm, func->s_name), fn_name) )
			return func;
	}
	return NULL;
}

/*
============
ED_GlobalAtOfs
============
*/
static ddef_t *ED_GlobalAtOfs (NVM* qcvm, int ofs)
{
	ddef_t		*def;
	int			i;

	for (i = 0; i < qcvm->progs->numglobaldefs; i++)
	{
		def = &qcvm->globaldefs[i];
		if (def->ofs == ofs)
			return def;
	}
	return NULL;
}

/*
============
ED_FieldAtOfs
============
*/
static ddef_t *ED_FieldAtOfs (NVM* qcvm, int ofs)
{
	ddef_t		*def;
	int			i;

	for (i = 0; i < qcvm->progs->numfielddefs; i++)
	{
		def = &qcvm->fielddefs[i];
		if (def->ofs == ofs)
			return def;
	}
	return NULL;
}

/*
============
PR_ValueString
(etype_t type, eval_t *val)

Returns a string describing *data in a type specific manner
=============
*/
static const char *PR_ValueString (NVM* qcvm, int type, eval_t *val)
{
	static char	line[512];
	ddef_t		*def;
	dfunction_t	*f;

	type &= ~DEF_SAVEGLOBAL;

	switch (type)
	{
	case ev_string:
		sprintf (line, "%s", PR_GetString(qcvm, val->string));
		break;
	case ev_entity:
		sprintf (line, "entity %i", NUM_FOR_EDICT(qcvm, PROG_TO_EDICT(val->edict)) );
		break;
	case ev_function:
		f = qcvm->functions + val->function;
		sprintf (line, "%s()", PR_GetString(qcvm, f->s_name));
		break;
	case ev_field:
		def = ED_FieldAtOfs ( qcvm, val->_int );
		sprintf (line, ".%s", PR_GetString(qcvm, def->s_name));
		break;
	case ev_void:
		sprintf (line, "void");
		break;
	case ev_float:
		sprintf (line, "%5.1f", val->_float);
		break;
	case ev_ext_integer:
		sprintf (line, "%i", val->_int);
		break;
	case ev_vector:
		sprintf (line, "'%5.1f %5.1f %5.1f'", val->vector[0], val->vector[1], val->vector[2]);
		break;
	case ev_pointer:
		sprintf (line, "pointer");
		break;
	default:
		sprintf (line, "bad type %i", type);
		break;
	}

	return line;
}

/*
============
PR_GlobalString

Returns a string with a description and the contents of a global,
padded to 20 field width
============
*/
static const char *PR_GlobalString (NVM* qcvm, int ofs)
{
	static char	line[512];
	const char	*s;
	int		i;
	ddef_t		*def;
	void		*val;

	val = (void *)&qcvm->globals[ofs];
	def = ED_GlobalAtOfs(qcvm, ofs);
	if (!def)
		sprintf (line,"%i(?)", ofs);
	else
	{
		s = PR_ValueString (qcvm, def->type, (eval_t *)val);
		sprintf (line,"%i(%s)%s", ofs, PR_GetString(qcvm, def->s_name), s);
	}

	i = strlen(line);
	for ( ; i < 20; i++)
		strcat (line, " ");
	strcat (line, " ");

	return line;
}

static const char *PR_GlobalStringNoContents (NVM* qcvm, int ofs)
{
	static char	line[512];
	int		i;
	ddef_t		*def;

	def = ED_GlobalAtOfs(qcvm, ofs);
	if (!def)
		sprintf (line,"%i(?)", ofs);
	else
		sprintf (line,"%i(%s)", ofs, PR_GetString(qcvm, def->s_name));

	i = strlen(line);
	for ( ; i < 20; i++)
		strcat (line, " ");
	strcat (line, " ");

	return line;
}

NVM* nvmCreateVM(AllocCallback acb, FreeCallback fcb, PrintCallback pcb, ErrorCallback ecb)
{
    NVM* vm = (NVM*)malloc(sizeof(NVM));
    memset(vm, 0, sizeof(NVM));
    vm->alloc_callback = acb;
    vm->free_callback = fcb;
    vm->print_callback = pcb;
    vm->error_callback = ecb;
    vm->auto_ext_builtin_number = MAX_BUILTINS - 1;
    return vm;
}

void nvmAddBuiltin(NVM* qcvm, int num, const char* name, BuiltinFunction builtin)
{
    if (num)
    {
        qcvm->builtins[num] = builtin;
    }
    else
    {
        //any #0 functions are remapped to their builtins here, so we don't have to tweak the VM in an obscure potentially-breaking way.
        for (int i = 0; i < (unsigned int)qcvm->progs->numfunctions; i++)
        {
            if (qcvm->functions[i].first_statement == 0 && qcvm->functions[i].s_name && !qcvm->functions[i].parm_start && !qcvm->functions[i].locals)
            {
                const char *vm_func_name = PR_GetString(qcvm, qcvm->functions[i].s_name);
                if (!strcmp(name, vm_func_name))
                {	//okay, map it
                    qcvm->functions[i].first_statement = -(qcvm->auto_ext_builtin_number);
                    qcvm->builtins[qcvm->auto_ext_builtin_number] = builtin;
                    break;
                }
            }
        }
    }
}

void nvmLoadBuiltins(NVM* qcvm, BuiltinFunction* builtins, size_t numbuiltins)
{
    memcpy(qcvm->builtins, builtins, numbuiltins*sizeof(qcvm->builtins[0]));
	qcvm->numbuiltins = numbuiltins;
}

bool nvmLoadProgs(NVM* qcvm, const char* filename, const char* data, size_t size, bool fatal)
{
    int			i;
	unsigned int u;

	//PR_ClearProgs(qcvm);	//just in case.

	qcvm->progs = (dprograms_t *)data;
	if (!qcvm->progs)
		return false;

	qcvm->progssize = size;

	// byte swap the header
	for (i = 0; i < (int) sizeof(*qcvm->progs) / 4; i++)
		((int *)qcvm->progs)[i] = LittleLong ( ((int *)qcvm->progs)[i] );

	if (qcvm->progs->version != PROG_VERSION)
	{
		if (fatal)
			Errorf (qcvm, "%s has wrong version number (%i should be %i)", filename, qcvm->progs->version, PROG_VERSION);
		else
		{
			Printf (qcvm, "%s ABI set not supported\n", filename);
			qcvm->progs = NULL;
			return false;
		}
	}

	DPrintf (qcvm, "%s occupies %iK.\n", filename, size/1024);

	qcvm->functions = (dfunction_t *)((byte *)qcvm->progs + qcvm->progs->ofs_functions);
	qcvm->strings = (char *)qcvm->progs + qcvm->progs->ofs_strings;
	if (qcvm->progs->ofs_strings + qcvm->progs->numstrings >= size)
		Errorf (qcvm, "%s strings go past end of file\n", filename);

	qcvm->globaldefs = (ddef_t *)((byte *)qcvm->progs + qcvm->progs->ofs_globaldefs);
	qcvm->fielddefs = (ddef_t *)((byte *)qcvm->progs + qcvm->progs->ofs_fielddefs);
	qcvm->statements = (dstatement_t *)((byte *)qcvm->progs + qcvm->progs->ofs_statements);

	qcvm->globals = (float *)((byte *)qcvm->progs + qcvm->progs->ofs_globals);
	//qcvm->global_struct = (globalvars_t*)qcvm->globals;

	qcvm->stringssize = qcvm->progs->numstrings;

	// byte swap the lumps
	for (i = 0; i < qcvm->progs->numstatements; i++)
	{
		qcvm->statements[i].op = LittleShort(qcvm->statements[i].op);
		qcvm->statements[i].a = LittleShort(qcvm->statements[i].a);
		qcvm->statements[i].b = LittleShort(qcvm->statements[i].b);
		qcvm->statements[i].c = LittleShort(qcvm->statements[i].c);
	}

	for (i = 0; i < qcvm->progs->numfunctions; i++)
	{
		qcvm->functions[i].first_statement = LittleLong (qcvm->functions[i].first_statement);
		qcvm->functions[i].parm_start = LittleLong (qcvm->functions[i].parm_start);
		qcvm->functions[i].s_name = LittleLong (qcvm->functions[i].s_name);
		qcvm->functions[i].s_file = LittleLong (qcvm->functions[i].s_file);
		qcvm->functions[i].numparms = LittleLong (qcvm->functions[i].numparms);
		qcvm->functions[i].locals = LittleLong (qcvm->functions[i].locals);
	}

	for (i = 0; i < qcvm->progs->numglobaldefs; i++)
	{
		qcvm->globaldefs[i].type = LittleShort (qcvm->globaldefs[i].type);
		qcvm->globaldefs[i].ofs = LittleShort (qcvm->globaldefs[i].ofs);
		qcvm->globaldefs[i].s_name = LittleLong (qcvm->globaldefs[i].s_name);
        //Con_Printf("globaldefs: %s - %d %d\n", qcvm->strings + qcvm->globaldefs[i].s_name, qcvm->globaldefs[i].type, qcvm->globaldefs[i].ofs);
	}

	//for (u = 0; u < sizeof(qcvm->extfields)/sizeof(int); u++)
		//((int*)&qcvm->extfields)[u] = -1;

	for (i = 0; i < qcvm->progs->numfielddefs; i++)
	{
		qcvm->fielddefs[i].type = LittleShort (qcvm->fielddefs[i].type);
		if (qcvm->fielddefs[i].type & DEF_SAVEGLOBAL)
			Errorf (qcvm, "PR_LoadProgs: pr_fielddefs[i].type & DEF_SAVEGLOBAL");
		qcvm->fielddefs[i].ofs = LittleShort (qcvm->fielddefs[i].ofs);
		qcvm->fielddefs[i].s_name = LittleLong (qcvm->fielddefs[i].s_name);
	}

	for (i = 0; i < qcvm->progs->numglobals; i++)
		((int *)qcvm->globals)[i] = LittleLong (((int *)qcvm->globals)[i]);

/*
	//spike: detect extended fields from progs
#define QCEXTFIELD(n,t) qcvm->extfields.n = ED_FindFieldOffset(#n);
	QCEXTFIELDS_ALL
	QCEXTFIELDS_GAME
	QCEXTFIELDS_CL
	QCEXTFIELDS_CS
	QCEXTFIELDS_SS
#undef QCEXTFIELD

	i = qcvm->progs->entityfields;
	if (qcvm->extfields.emiteffectnum < 0)
		qcvm->extfields.emiteffectnum = i++;
	if (qcvm->extfields.traileffectnum < 0)
		qcvm->extfields.traileffectnum = i++;*/

	qcvm->edict_size = i * 4 + sizeof(edict_t) - sizeof(entvars_t);
	// round off to next highest whole word address (esp for Alpha)
	// this ensures that pointers in the engine data area are always
	// properly aligned
	qcvm->edict_size += sizeof(void *) - 1;
	qcvm->edict_size &= ~(sizeof(void *) - 1);

	PR_SetEngineString(qcvm, "");
	//PR_EnableExtensions(qcvm, qcvm->globaldefs);

	return true;
}

int nvmFindFunction(NVM* vm, const char* name)
{
    dfunction_t* func = ED_FindFunction(vm, name);
    if (func)
        return func - vm->functions;
    else
        return -1;
}

static const char *pr_opnames[] =
{
	"DONE",

	"MUL_F",
	"MUL_V",
	"MUL_FV",
	"MUL_VF",

	"DIV",

	"ADD_F",
	"ADD_V",

	"SUB_F",
	"SUB_V",

	"EQ_F",
	"EQ_V",
	"EQ_S",
	"EQ_E",
	"EQ_FNC",

	"NE_F",
	"NE_V",
	"NE_S",
	"NE_E",
	"NE_FNC",

	"LE",
	"GE",
	"LT",
	"GT",

	"INDIRECT",
	"INDIRECT",
	"INDIRECT",
	"INDIRECT",
	"INDIRECT",
	"INDIRECT",

	"ADDRESS",

	"STORE_F",
	"STORE_V",
	"STORE_S",
	"STORE_ENT",
	"STORE_FLD",
	"STORE_FNC",

	"STOREP_F",
	"STOREP_V",
	"STOREP_S",
	"STOREP_ENT",
	"STOREP_FLD",
	"STOREP_FNC",

	"RETURN",

	"NOT_F",
	"NOT_V",
	"NOT_S",
	"NOT_ENT",
	"NOT_FNC",

	"IF",
	"IFNOT",

	"CALL0",
	"CALL1",
	"CALL2",
	"CALL3",
	"CALL4",
	"CALL5",
	"CALL6",
	"CALL7",
	"CALL8",

	"STATE",

	"GOTO",

	"AND",
	"OR",

	"BITAND",
	"BITOR"
};

/*
=================
PR_PrintStatement
=================
*/
void PR_PrintStatement (NVM* qcvm, dstatement_t *s)
{
	int	i;

	if ((unsigned int)s->op < sizeof(pr_opnames)/sizeof(pr_opnames[0]))
	{
		Printf(qcvm, "%s ", pr_opnames[s->op]);
		i = strlen(pr_opnames[s->op]);
		for ( ; i < 10; i++)
			Printf(qcvm, " ");
	}

	if (s->op == OP_IF || s->op == OP_IFNOT)
		Printf(qcvm, "%sbranch %i", PR_GlobalString(qcvm, s->a), s->b);
	else if (s->op == OP_GOTO)
	{
		Printf(qcvm, "branch %i", s->a);
	}
	else if ((unsigned int)(s->op-OP_STORE_F) < 6)
	{
		Printf(qcvm, "%s", PR_GlobalString(qcvm, s->a));
		Printf(qcvm, "%s", PR_GlobalStringNoContents(qcvm, s->b));
	}
	else
	{
		if (s->a)
			Printf(qcvm, "%s", PR_GlobalString(qcvm, s->a));
		if (s->b)
			Printf(qcvm, "%s", PR_GlobalString(qcvm, s->b));
		if (s->c)
			Printf(qcvm, "%s", PR_GlobalStringNoContents(qcvm, s->c));
	}
	Printf(qcvm, "\n");
}

/*
============
PR_StackTrace
============
*/
static void PR_StackTrace (NVM* qcvm)
{
	int		i;
	dfunction_t	*f;

	if (qcvm->depth == 0)
	{
		Printf(qcvm, "<NO STACK>\n");
		return;
	}

	qcvm->stack[qcvm->depth].f = qcvm->xfunction;
	for (i = qcvm->depth; i >= 0; i--)
	{
		f = qcvm->stack[i].f;
		if (!f)
		{
			Printf(qcvm, "<NO FUNCTION>\n");
		}
		else
		{
			Printf(qcvm, "%12s : %s\n", PR_GetString(qcvm, f->s_file), PR_GetString(qcvm, f->s_name));
		}
	}
}

/*
============
PR_RunError

Aborts the currently executing function
============
*/
void PR_RunError (NVM* qcvm, const char *error, ...)
{
	va_list	argptr;
	char	string[1024];

	va_start (argptr, error);
	vsnprintf (string, sizeof(string), error, argptr);
	va_end (argptr);

	PR_PrintStatement(qcvm, qcvm->statements + qcvm->xstatement);
	PR_StackTrace(qcvm);

	Printf(qcvm, "%s\n", string);

	qcvm->depth = 0;	// dump the stack so host_error can shutdown functions

	Errorf(qcvm, "Program error");
}

/*
====================
PR_EnterFunction

Returns the new program statement counter
====================
*/
static int PR_EnterFunction (NVM* qcvm, dfunction_t *f)
{
	int	i, j, c, o;

	qcvm->stack[qcvm->depth].s = qcvm->xstatement;
	qcvm->stack[qcvm->depth].f = qcvm->xfunction;
	qcvm->depth++;
	if (qcvm->depth >= MAX_STACK_DEPTH)
		PR_RunError(qcvm, "stack overflow");

	// save off any locals that the new function steps on
	c = f->locals;
	if (qcvm->localstack_used + c > LOCALSTACK_SIZE)
		PR_RunError(qcvm, "PR_ExecuteProgram: locals stack overflow\n");

	for (i = 0; i < c ; i++)
		qcvm->localstack[qcvm->localstack_used + i] = ((int *)qcvm->globals)[f->parm_start + i];
	qcvm->localstack_used += c;

	// copy parameters
	o = f->parm_start;
	for (i = 0; i < f->numparms; i++)
	{
		for (j = 0; j < f->parm_size[i]; j++)
		{
			((int *)qcvm->globals)[o] = ((int *)qcvm->globals)[OFS_PARM0 + i*3 + j];
			o++;
		}
	}

	qcvm->xfunction = f;
	return f->first_statement - 1;	// offset the s++
}

/*
====================
PR_LeaveFunction
====================
*/
static int PR_LeaveFunction (NVM* qcvm)
{
	int	i, c;

	if (qcvm->depth <= 0)
		Errorf(qcvm, "prog stack underflow");

	// Restore locals from the stack
	c = qcvm->xfunction->locals;
	qcvm->localstack_used -= c;
	if (qcvm->localstack_used < 0)
		PR_RunError(qcvm, "PR_ExecuteProgram: locals stack underflow");

	for (i = 0; i < c; i++)
		((int *)qcvm->globals)[qcvm->xfunction->parm_start + i] = qcvm->localstack[qcvm->localstack_used + i];

	// up stack
	qcvm->depth--;
	qcvm->xfunction = qcvm->stack[qcvm->depth].f;
	return qcvm->stack[qcvm->depth].s;
}

#define OPA ((eval_t *)&qcvm->globals[(unsigned short)st->a])
#define OPB ((eval_t *)&qcvm->globals[(unsigned short)st->b])
#define OPC ((eval_t *)&qcvm->globals[(unsigned short)st->c])

void nvmExecuteFunction(NVM* qcvm, func_t fnum)
{
	eval_t		*ptr;
	dstatement_t	*st;
	dfunction_t	*f, *newf;
	int profile, startprofile;
	edict_t		*ed;
	int		exitdepth;

	if (!fnum || fnum >= qcvm->progs->numfunctions)
	{
		// if (qcvm->global_struct->self) ED_Print (PROG_TO_EDICT(qcvm->global_struct->self));
		Errorf (qcvm, "PR_ExecuteProgram: NULL function");
	}

	f = &qcvm->functions[fnum];

	//FIXME: if this is a builtin, then we're going to crash.

	qcvm->trace = false;

// make a stack frame
	exitdepth = qcvm->depth;

	st = &qcvm->statements[PR_EnterFunction(qcvm, f)];
	startprofile = profile = 0;

    while (1)
    {
	st++;	/* next statement */

	if (++profile > 0x10000000)	//spike -- was decimal 100000
	{
		qcvm->xstatement = st - qcvm->statements;
		PR_RunError(qcvm, "runaway loop error");
	}

	if (qcvm->trace)
		PR_PrintStatement(qcvm, st);

	switch (st->op)
	{
	case OP_ADD_F:
		OPC->_float = OPA->_float + OPB->_float;
		break;
	case OP_ADD_V:
		OPC->vector[0] = OPA->vector[0] + OPB->vector[0];
		OPC->vector[1] = OPA->vector[1] + OPB->vector[1];
		OPC->vector[2] = OPA->vector[2] + OPB->vector[2];
		break;

	case OP_SUB_F:
		OPC->_float = OPA->_float - OPB->_float;
		break;
	case OP_SUB_V:
		OPC->vector[0] = OPA->vector[0] - OPB->vector[0];
		OPC->vector[1] = OPA->vector[1] - OPB->vector[1];
		OPC->vector[2] = OPA->vector[2] - OPB->vector[2];
		break;

	case OP_MUL_F:
		OPC->_float = OPA->_float * OPB->_float;
		break;
	case OP_MUL_V:
		OPC->_float = OPA->vector[0] * OPB->vector[0] +
			      OPA->vector[1] * OPB->vector[1] +
			      OPA->vector[2] * OPB->vector[2];
		break;
	case OP_MUL_FV:
		OPC->vector[0] = OPA->_float * OPB->vector[0];
		OPC->vector[1] = OPA->_float * OPB->vector[1];
		OPC->vector[2] = OPA->_float * OPB->vector[2];
		break;
	case OP_MUL_VF:
		OPC->vector[0] = OPB->_float * OPA->vector[0];
		OPC->vector[1] = OPB->_float * OPA->vector[1];
		OPC->vector[2] = OPB->_float * OPA->vector[2];
		break;

	case OP_DIV_F:
		OPC->_float = OPA->_float / OPB->_float;
		break;

	case OP_BITAND:
		OPC->_float = (int)OPA->_float & (int)OPB->_float;
		break;

	case OP_BITOR:
		OPC->_float = (int)OPA->_float | (int)OPB->_float;
		break;

	case OP_GE:
		OPC->_float = OPA->_float >= OPB->_float;
		break;
	case OP_LE:
		OPC->_float = OPA->_float <= OPB->_float;
		break;
	case OP_GT:
		OPC->_float = OPA->_float > OPB->_float;
		break;
	case OP_LT:
		OPC->_float = OPA->_float < OPB->_float;
		break;
	case OP_AND:
		OPC->_float = OPA->_float && OPB->_float;
		break;
	case OP_OR:
		OPC->_float = OPA->_float || OPB->_float;
		break;

	case OP_NOT_F:
		OPC->_float = !OPA->_float;
		break;
	case OP_NOT_V:
		OPC->_float = !OPA->vector[0] && !OPA->vector[1] && !OPA->vector[2];
		break;
	case OP_NOT_S:
		OPC->_float = !OPA->string || !*PR_GetString(qcvm, OPA->string);
		break;
	case OP_NOT_FNC:
		OPC->_float = !OPA->function;
		break;
	case OP_NOT_ENT:
		OPC->_float = (PROG_TO_EDICT(OPA->edict) == qcvm->edicts);
		break;

	case OP_EQ_F:
		OPC->_float = OPA->_float == OPB->_float;
		break;
	case OP_EQ_V:
		OPC->_float = (OPA->vector[0] == OPB->vector[0]) &&
			      (OPA->vector[1] == OPB->vector[1]) &&
			      (OPA->vector[2] == OPB->vector[2]);
		break;
	case OP_EQ_S:
		OPC->_float = !strcmp(PR_GetString(qcvm, OPA->string), PR_GetString(qcvm, OPB->string));
		break;
	case OP_EQ_E:
		OPC->_float = OPA->_int == OPB->_int;
		break;
	case OP_EQ_FNC:
		OPC->_float = OPA->function == OPB->function;
		break;

	case OP_NE_F:
		OPC->_float = OPA->_float != OPB->_float;
		break;
	case OP_NE_V:
		OPC->_float = (OPA->vector[0] != OPB->vector[0]) ||
			      (OPA->vector[1] != OPB->vector[1]) ||
			      (OPA->vector[2] != OPB->vector[2]);
		break;
	case OP_NE_S:
		OPC->_float = strcmp(PR_GetString(qcvm, OPA->string), PR_GetString(qcvm, OPB->string));
		break;
	case OP_NE_E:
		OPC->_float = OPA->_int != OPB->_int;
		break;
	case OP_NE_FNC:
		OPC->_float = OPA->function != OPB->function;
		break;

	case OP_STORE_F:
	case OP_STORE_ENT:
	case OP_STORE_FLD:	// integers
	case OP_STORE_S:
	case OP_STORE_FNC:	// pointers
		OPB->_int = OPA->_int;
		break;
	case OP_STORE_V:
		OPB->vector[0] = OPA->vector[0];
		OPB->vector[1] = OPA->vector[1];
		OPB->vector[2] = OPA->vector[2];
		break;

	case OP_STOREP_F:
	case OP_STOREP_ENT:
	case OP_STOREP_FLD:	// integers
	case OP_STOREP_S:
	case OP_STOREP_FNC:	// pointers
		ptr = (eval_t *)((byte *)qcvm->edicts + OPB->_int);
		ptr->_int = OPA->_int;
		break;
	case OP_STOREP_V:
		ptr = (eval_t *)((byte *)qcvm->edicts + OPB->_int);
		ptr->vector[0] = OPA->vector[0];
		ptr->vector[1] = OPA->vector[1];
		ptr->vector[2] = OPA->vector[2];
		break;

	case OP_ADDRESS:
		ed = PROG_TO_EDICT(OPA->edict);
#ifdef PARANOID
		NUM_FOR_EDICT(ed);	// Make sure it's in range
#endif
#if 0
		if (ed == (edict_t *)qcvm->edicts && sv.state == ss_active)
		{
			qcvm->xstatement = st - qcvm->statements;
			PR_RunError("assignment to world entity");
		}
#endif
		OPC->_int = (byte *)((int *)&ed->v + OPB->_int) - (byte *)qcvm->edicts;
		break;

	case OP_LOAD_F:
	case OP_LOAD_FLD:
	case OP_LOAD_ENT:
	case OP_LOAD_S:
	case OP_LOAD_FNC:
		ed = PROG_TO_EDICT(OPA->edict);
#ifdef PARANOID
		NUM_FOR_EDICT(ed);	// Make sure it's in range
#endif
		OPC->_int = ((eval_t *)((int *)&ed->v + OPB->_int))->_int;
		break;

	case OP_LOAD_V:
		ed = PROG_TO_EDICT(OPA->edict);
#ifdef PARANOID
		NUM_FOR_EDICT(ed);	// Make sure it's in range
#endif
		ptr = (eval_t *)((int *)&ed->v + OPB->_int);
		OPC->vector[0] = ptr->vector[0];
		OPC->vector[1] = ptr->vector[1];
		OPC->vector[2] = ptr->vector[2];
		break;

	case OP_IFNOT:
		if (!OPA->_int)
			st += st->b - 1;	/* -1 to offset the st++ */
		break;

	case OP_IF:
		if (OPA->_int)
			st += st->b - 1;	/* -1 to offset the st++ */
		break;

	case OP_GOTO:
		st += st->a - 1;		/* -1 to offset the st++ */
		break;

	case OP_CALL0:
	case OP_CALL1:
	case OP_CALL2:
	case OP_CALL3:
	case OP_CALL4:
	case OP_CALL5:
	case OP_CALL6:
	case OP_CALL7:
	case OP_CALL8:
		qcvm->xfunction->profile += profile - startprofile;
		startprofile = profile;
		qcvm->xstatement = st - qcvm->statements;
		qcvm->argc = st->op - OP_CALL0;
		if (!OPA->function)
			PR_RunError(qcvm, "NULL function");
		newf = &qcvm->functions[OPA->function];
		if (newf->first_statement < 0)
		{ // Built-in function
			int i = -newf->first_statement;
			if (i >= qcvm->numbuiltins)
				i = 0;	//just invoke the fixme builtin.
			qcvm->builtins[i](qcvm);
			break;
		}
		// Normal function
		st = &qcvm->statements[PR_EnterFunction(qcvm, newf)];
		break;

	case OP_DONE:
	case OP_RETURN:
		qcvm->xfunction->profile += profile - startprofile;
		startprofile = profile;
		qcvm->xstatement = st - qcvm->statements;
		qcvm->globals[OFS_RETURN] = qcvm->globals[(unsigned short)st->a];
		qcvm->globals[OFS_RETURN + 1] = qcvm->globals[(unsigned short)st->a + 1];
		qcvm->globals[OFS_RETURN + 2] = qcvm->globals[(unsigned short)st->a + 2];
		st = &qcvm->statements[PR_LeaveFunction(qcvm)];
		if (qcvm->depth == exitdepth)
		{ // Done
			return;
		}
		break;

	case OP_STATE:
		ed = PROG_TO_EDICT(qcvm->global_struct->self);
		ed->v.nextthink = qcvm->global_struct->time + 0.1;
		ed->v.frame = OPA->_float;
		ed->v.think = OPB->function;
		break;

	default:
		qcvm->xstatement = st - qcvm->statements;
		PR_RunError(qcvm, "Bad opcode %i", st->op);
	}
    }	/* end of while(1) loop */
}
#undef OPA
#undef OPB
#undef OPC
