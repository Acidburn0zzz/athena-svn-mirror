#ifndef ORBIT_IDL3_TYPES_H
#define ORBIT_IDL3_TYPES_H 1

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <libIDL/IDL.h>
#include <orbit/util/orbit-util.h>
#include <orbit/orbit-config.h>

typedef struct _OIDL_Marshal_Context OIDL_Marshal_Context;

#define OUTPUT_NUM_PASSES 7

typedef struct {
  char *cpp_args;
  int debug_level;
  int idl_warn_level;
  int show_cpp_errors;
  int is_pidl;
  int do_skel_defs;	/* gen defs within the header file */

  enum { OUTPUT_STUBS=1<<0,
	 OUTPUT_SKELS=1<<1,
	 OUTPUT_COMMON=1<<2,
	 OUTPUT_HEADERS=1<<3,
	 OUTPUT_SKELIMPL=1<<4,
	 OUTPUT_IMODULE=1<<5,
	 OUTPUT_DEPS=1<<6 /* Make sure this is always the last pass or dep output will break. */
  } enabled_passes;

  char *output_formatter;

  char *output_language;
  char *input_filename;
  char *backend_directory;
  char *deps_file;
  char *header_guard_prefix;
  gboolean onlytop;
  gboolean small;
  gboolean small_stubs;
  gboolean small_skels;
  gboolean idata;

  IDL_ns ns; /* Use ns instead of namespace because that's a C++ reserved keyword */
} OIDL_Run_Info;

/* yadda yadda:
   Fixed length as in bulk-marshallable
   Fixed length as in a terminal allocation.

   Turn an IDL_LIST of params into a tree of marshalling info.
     Each node will need to give:
         Type (datum, loop, switch, string, complex)
	 Name
	 Subnodes (loop & switch only)
	 Dependencies

   Note for string_info.length_var, loop_info.loop_var, switch_info.discrim - these are all subnodes of the current node,
   not pointers to other unrelated nodes.

   dependencies is a list of pointers to unrelated nodes.
 */
typedef struct _OIDL_Marshal_Node OIDL_Marshal_Node;


/**
    LOOP is the (de)marshalling equivalent of IDL sequence, array, string,
    and wstring.  In all cases, there is a single underlying type ("subtype").
    The variable (parameter) consists of zero or more elements of the
    subtype.
**/

typedef enum {
  MARSHAL_DATUM,
  MARSHAL_LOOP,
  MARSHAL_SWITCH,
  MARSHAL_CASE,
  MARSHAL_COMPLEX,
  MARSHAL_CONST,
  MARSHAL_SET
} OIDL_Marshal_Node_Type;

typedef enum {
  MN_INOUT = 1<<0, /* Needs freeing before alloc */
  MN_NSROOT = 1<<1, /* Don't go to parents for variable naming */
  MN_NEED_TMPVAR = 1<<2, /* Need a temporary variable to hold this value */
  MN_NOMARSHAL = 1<<3, /* This is used by other vars, but not actually marshalled */
  MN_ISSEQ = 1<<4, /* for MARSHAL_LOOP only - we need to do foo._buffer before tacking on [v1] */
  MN_ISSTRING = 1<<5, /* for MARSHAL_LOOP only */
  MN_LOOPED = 1<<6, /* This variable is looped over */
  MN_COALESCABLE = 1<<7, /* You can coalesce multiple sequential instances of this type into one encode/decode operation */
  MN_ENDIAN_DEPENDANT = 1<<8,
  MN_DEMARSHAL_UPDATE_AFTER = 1<<9,
  MN_RECURSIVE_TOP = 1<<10,
  MN_TOPLEVEL = 1<<11, /* This is the toplevel thingie for the operation, so we can do special-case things for mem allocation */
  MN_PARAM_INOUT = 1<<12, /* For a node representing the top of an inout param, where we need to allocate slightly differently */
  MN_NEED_CURPTR_LOCAL = 1<<13,
  MN_NEED_CURPTR_RECVBUF = 1<<14,
  MN_ISSLICE = 1<<15,
  MN_WIDESTRING = 1<<16 /* variant of MN_ISSTRING - MN_ISSTRING will also be set */
} OIDL_Marshal_Node_Flags;

/**
    When demarshalling a message (both stub and skel), each of the
    parameters must be extracted into local memory. This local memory
    may be of the following types:
    	Auto		The variable is declared explicitly as a
			C automatic variable, e.g. "int x;",
			and thus the value of the variable is stored
			on the stack.
    	Alloca		A pointer variable is declared on the stack,
			and space for the variable value is
			allocated using alloca().
	Msg		A pointer variable is declared on the stack,
			and is set to point "into" the recieved message.
    	Heap		A pointer variable is declared on the stack,
			and space for the variable value is allocated
			allocated using some varient of CORBA_alloc,
			and it must be freed using CORBA_free.
	
    In addition, the following two are defined:
    	Virtual		This is a collection of variables, and memory
			for the collection is not allocated together.
	Within		Memory for this variable is allocated within
			its parent. (e.g., this is used for simple members
			of a structure).
**/

typedef enum {
    MW_Null = 1<<0,
    MW_Auto = 1<<1,
    MW_Alloca = 1<<2,
    MW_Msg = 1<<3,
    MW_Heap = 1<<4,
    MW_Any = (MW_Null|MW_Auto|MW_Alloca|MW_Msg|MW_Heap)
} OIDL_Marshal_Where;

typedef struct {
  int len;
  char *mult_const;
  OIDL_Marshal_Node *mult_expr;
} OIDL_Marshal_Length;

struct _OIDL_Marshal_Node {
  OIDL_Marshal_Node *up;
  char *name;
  IDL_tree tree;
  OIDL_Marshal_Node_Type type;
  GSList *dependencies;
  union {
    struct {
      OIDL_Marshal_Node *loop_var, *length_var;
      OIDL_Marshal_Node *contents;
    } loop_info;
    struct {
      OIDL_Marshal_Node *discrim;
      GSList *cases;
    } switch_info;
    struct {
      GSList *labels;
      OIDL_Marshal_Node *contents;
    } case_info;
    struct {
      guint32 datum_size;
      gboolean needs_bswap;
    } datum_info;
    struct {
      OIDL_Marshal_Node *amount;
    } update_info;
    struct {
      glong amount;
    } const_info;
    struct {
      enum {
	CX_CORBA_FIXED, 
	CX_CORBA_ANY, 
	CX_CORBA_OBJECT, 
	CX_CORBA_TYPECODE, 
	CX_CORBA_CONTEXT,
	CX_MARSHAL_METHOD,
	CX_NATIVE
      } type;
      int context_item_count;
    } complex_info;
    struct {
      GSList *subnodes;
    } set_info;
  } u;
  OIDL_Marshal_Node_Flags flags;
  OIDL_Marshal_Where where;
  guint8 arch_head_align, arch_tail_align;
  guint8 iiop_head_align, iiop_tail_align;
  gint8 nptrs, use_count;
  OIDL_Marshal_Length *pre, *post; /* This condition must be met before/after (pre/post) this node is marshalled */
  char *marshal_error_exit;
};

/* Handling an IDLN_ATTR_DCL:
   foreach(node->simple_declarations) {
       turn node->data into a OIDL_Attr_Info.
       Process op1 & op2.
   }
*/
typedef struct {
  IDL_tree op1, op2;
} OIDL_Attr_Info;
typedef struct {
  OIDL_Marshal_Node *in_stubs, *out_stubs, *in_skels, *out_skels;
  int counter;
} OIDL_Op_Info;
typedef struct {
  OIDL_Marshal_Node *marshal, *demarshal;
} OIDL_Except_Info;

typedef struct _OIDL_Output_Tree OIDL_Output_Tree;

typedef struct {
  const char *name;
  void (*op_output)(OIDL_Output_Tree *tree, OIDL_Run_Info *rinfo);
} OIDL_Backend_Info;

#endif
