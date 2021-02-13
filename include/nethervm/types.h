#ifndef NETHERVM_TYPES_H
#define NETHERVM_TYPES_H

#include "nethervm/pr_comp.h"
#include "nethervm/progdefs.h"

#define MAX_BUILTINS 1024
#define AREA_NODES 32

#define	NEXT_EDICT(e)		((edict_t *)( (byte *)e + qcvm->edict_size))

#define	EDICT_TO_PROG(e)	((byte *)e - (byte *)qcvm->edicts)
#define PROG_TO_EDICT(e)	((edict_t *)((byte *)qcvm->edicts + e))

#define	G_FLOAT(o)		(qcvm->globals[o])
#define	G_INT(o)		(*(int *)&qcvm->globals[o])
#define	G_EDICT(o)		((edict_t *)((byte *)qcvm->edicts+ *(int *)&qcvm->globals[o]))
#define G_EDICTNUM(o)		NUM_FOR_EDICT(G_EDICT(o))
#define	G_VECTOR(o)		(&qcvm->globals[o])
#define	G_STRING(o)		(nvmGetString(qcvm, *(string_t *)&qcvm->globals[o]))
#define	G_FUNCTION(o)		(*(func_t *)&qcvm->globals[o])

#define G_VECTORSET(r,x,y,z) do{G_FLOAT((r)+0) = x; G_FLOAT((r)+1) = y;G_FLOAT((r)+2) = z;}while(0)

#define	E_FLOAT(e,o)		(((float*)&e->v)[o])
#define	E_INT(e,o)		(*(int *)&((float*)&e->v)[o])
#define	E_VECTOR(e,o)		(&((float*)&e->v)[o])
#define	E_STRING(e,o)		(nvmGetString(qcvm, *(string_t *)&((float*)&e->v)[o]))

typedef struct NVM_s NVM;

typedef bool qboolean;

typedef char byte;

typedef void (*BuiltinFunction)(NVM* vm);

typedef void*(*AllocCallback)(NVM* vm, size_t size, const char* name);

typedef void(*FreeCallback)(NVM* vm, void* ptr);

typedef void(*PrintCallback)(NVM* vm, const char* msg, bool debug);

typedef void(*ErrorCallback)(NVM* vm, const char* msg);

typedef union eval_s
{
	string_t	string;
	float		_float;
	float		vector[3];
	func_t		function;
	int		_int;
	int		edict;
} eval_t;

#define	MAX_ENT_LEAFS	32
typedef struct edict_s
{
	qboolean	free;
	//link_t		area;			/* linked to a division node or leaf */

	unsigned int		num_leafs;
	int		leafnums[MAX_ENT_LEAFS];

	//entity_state_t	baseline;
	unsigned char	alpha;			/* johnfitz -- hack to support alpha since it's not part of entvars_t */
	qboolean	sendinterval;		/* johnfitz -- send time until nextthink to client for better lerp timing */
	qboolean	onladder;			/* spike -- content_ladder stuff */

	float		freetime;		/* sv.time when the object was freed */
	entvars_t	v;			/* C exported fields from progs */

	/* other fields from progs come immediately after */
} edict_t;

typedef struct
{
	int		s;
	dfunction_t	*f;
} prstack_t;

typedef struct areanode_s
{
	int		axis;		// -1 = leaf node
	float	dist;
	struct areanode_s	*children[2];
	//link_t	trigger_edicts;
	//link_t	solid_edicts;
} areanode_t;

typedef struct NVM_s
{
    dprograms_t	*progs;    
	dfunction_t	*functions;
	dstatement_t	*statements;
	float		*globals;	/* same as qcvm->global_struct */
	ddef_t		*fielddefs;	//yay reflection.

	int			edict_size;	/* in bytes */

	BuiltinFunction	builtins[MAX_BUILTINS];
	int			    numbuiltins;

	int			argc;

	qboolean	trace;
	dfunction_t	*xfunction;
	int			xstatement;

	unsigned short	progscrc;	//crc16 of the entire file
	unsigned int	progshash;	//folded file md4
	unsigned int	progssize;	//file size (bytes)

	//struct pr_extglobals_s extglobals;
	//struct pr_extfuncs_s extfuncs;
	//struct pr_extfields_s extfields;

	qboolean cursorforced;
	void *cursorhandle;	//video code.
	qboolean nogameaccess;	//simplecsqc isn't allowed to poke properties of the actual game (to prevent cheats when there's no restrictions on what it can access)

	//was static inside pr_edict
	char		*strings;
	int			stringssize;
	const char	**knownstrings;
	int			maxknownstrings;
	int			numknownstrings;
	int			freeknownstrings;
	ddef_t		*globaldefs;

	unsigned char *knownzone;
	size_t knownzonesize;

	//originally defined in pr_exec, but moved into the switchable qcvm struct
#define	MAX_STACK_DEPTH		1024 /*was 64*/	/* was 32 */
	prstack_t	stack[MAX_STACK_DEPTH];
	int			depth;

#define	LOCALSTACK_SIZE		16384 /* was 2048*/
	int			localstack[LOCALSTACK_SIZE];
	int			localstack_used;

	//originally part of the sv_state_t struct
	//FIXME: put worldmodel in here too.
	double		time;
	int			num_edicts;
	int			reserved_edicts;
	int			max_edicts;
	edict_t		*edicts;			// can NOT be array indexed, because edict_t is variable sized, but can be used to reference the world ent
	struct qmodel_s	*worldmodel;
	struct qmodel_s	*(*GetModel)(int modelindex);	//returns the model for the given index, or null.

	//originally from world.c
	areanode_t	areanodes[AREA_NODES];
	int			numareanodes;

    globalvars_t* global_struct;
    AllocCallback alloc_callback;
    FreeCallback  free_callback;
    ErrorCallback error_callback;
    PrintCallback print_callback;
    int auto_ext_builtin_number;
	void* user_data;
} NVM;

#endif