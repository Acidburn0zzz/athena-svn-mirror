#include "config.h"

#include "orbit-idl-c-backend.h"
#include <string.h>

static gboolean cc_output_tc_walker(IDL_tree_func_data *tfd, gpointer user_data);
static void cc_output_allocs(IDL_tree tree, OIDL_Run_Info *rinfo, OIDL_C_Info *ci);

static void cc_alloc_prep_sequence(IDL_tree tree, OIDL_Run_Info *rinfo, OIDL_C_Info *ci);
static void cc_typecode_prep_sequence(IDL_tree tree, OIDL_C_Info *ci);
static void cc_output_marshallers(OIDL_C_Info *ci);
static GSList *cc_small_build_interfaces (GSList *, IDL_tree tree);
static void cc_small_output_itypes (GSList *interfaces, OIDL_C_Info *ci);

typedef struct {
	IDL_tree tree;
	GSList  *methods; /* IDLN_OP_DCLs */
} Interface;

typedef struct {
	FILE     *of;
	IDL_tree cur_node; /* Current Interface */
	char     *cur_id;
	guint    parents;
} CCSmallInterfaceTraverseInfo;
 
static void
cc_output_typecodes(IDL_tree tree, OIDL_C_Info *ci)
{
  IDL_tree_walk2( tree, /*tfd*/0, IDL_WalkF_TypespecOnly,
		  /*pre*/ cc_output_tc_walker, /*post*/ cc_output_tc_walker, ci);
}

void
orbit_idl_output_c_common(OIDL_Output_Tree *tree, OIDL_Run_Info *rinfo, OIDL_C_Info *ci)
{
  fprintf (ci->fh, OIDL_C_WARNING);
  fprintf(ci->fh, "#include <string.h>\n");
  fprintf(ci->fh, "#define ORBIT2_STUBS_API\n");
  fprintf(ci->fh, "#define ORBIT_IDL_C_COMMON\n");
  fprintf(ci->fh, "#define %s_COMMON\n", ci->c_base_name);
  fprintf(ci->fh, "#include \"%s.h\"\n\n", ci->base_name);
  if (!rinfo->small)
    fprintf(ci->fh, "#include <orbit/GIOP/giop.h>\n");
  fprintf(ci->fh, "static const CORBA_unsigned_long ORBit_zero_int = 0;\n");

  cc_output_typecodes(tree->tree, ci);

  cc_output_allocs(tree->tree, rinfo, ci);

  if (!rinfo->small)
    cc_output_marshallers(ci);

  if (rinfo->small && rinfo->idata) {
    GSList *list;

    fprintf(ci->fh, "\n/* Interface type data */\n\n");
    list = cc_small_build_interfaces (NULL, tree->tree);
    cc_small_output_itypes (list, ci);
  }
}

static gboolean
cc_output_tc_walker(IDL_tree_func_data *tfd, gpointer user_data)
{
    OIDL_C_Info *ci = user_data;
    IDL_tree tree = tfd->tree;

#if 0
  if ( tree->declspec & IDLF_DECLSPEC_PIDL ) {
        g_warning ("Pruned pidl");
	return FALSE;	/* prune */
  }
#endif

  switch(IDL_NODE_TYPE(tree)) {
  case IDLN_CONST_DCL:
  case IDLN_ATTR_DCL:
  case IDLN_OP_DCL:
      return FALSE;	/* dont recurse into these */
  case IDLN_TYPE_SEQUENCE:
    cc_typecode_prep_sequence(tree, ci);
  case IDLN_INTERFACE: /* may need to be pre-order? */
  case IDLN_EXCEPT_DCL:
  case IDLN_TYPE_STRUCT:
  case IDLN_TYPE_UNION:
  case IDLN_TYPE_DCL:
  case IDLN_TYPE_ENUM:
  case IDLN_TYPE_FIXED:
    if ( tfd->step ) {
	/* do post-order */
        orbit_output_typecode(ci, tree);
    }
    break;
  default:
    break;
  }

  return TRUE;	/* continue walking */
}


/************************************************/
static void cc_alloc_prep(IDL_tree tree, OIDL_Run_Info *rinfo, OIDL_C_Info *ci);
static void cc_output_alloc_interface(IDL_tree tree, OIDL_Run_Info *rinfo, OIDL_C_Info *ci);
static void cc_output_alloc_struct(IDL_tree tree, OIDL_Run_Info *rinfo, OIDL_C_Info *ci);
static void cc_output_alloc_union(IDL_tree tree, OIDL_Run_Info *rinfo, OIDL_C_Info *ci);
static void cc_output_alloc_type_dcl(IDL_tree tree, OIDL_Run_Info *rinfo, OIDL_C_Info *ci);

static void
cc_output_allocs(IDL_tree tree, OIDL_Run_Info *rinfo, OIDL_C_Info *ci)
{
  if(!tree) return;

  if ( tree->declspec & IDLF_DECLSPEC_PIDL )
	return;

  switch(IDL_NODE_TYPE(tree)) {
  case IDLN_MODULE:
    cc_output_allocs(IDL_MODULE(tree).definition_list, rinfo, ci);
    break;
  case IDLN_LIST:
    {
      IDL_tree sub;
      for(sub = tree; sub; sub = IDL_LIST(sub).next)
	cc_output_allocs(IDL_LIST(sub).data, rinfo, ci);
    }
    break;
  case IDLN_INTERFACE:
    cc_output_alloc_interface(tree, rinfo, ci);
    break;
  case IDLN_EXCEPT_DCL:
  case IDLN_TYPE_STRUCT:
    if (!rinfo->small)
      cc_output_alloc_struct(tree, rinfo, ci);
    break;
  case IDLN_TYPE_UNION:
    if (!rinfo->small)
      cc_output_alloc_union(tree, rinfo, ci);
    break;
  case IDLN_TYPE_DCL:
    if (!rinfo->small)
      cc_output_alloc_type_dcl(tree, rinfo, ci);
    break;
  case IDLN_TYPE_SEQUENCE:
    if (!rinfo->small)
      cc_alloc_prep_sequence(tree, rinfo, ci);
    break;
  case IDLN_CASE_STMT:
    cc_output_allocs(IDL_CASE_STMT(tree).element_spec, rinfo, ci);
    break;
  case IDLN_MEMBER:
    cc_output_allocs(IDL_MEMBER(tree).type_spec, rinfo, ci);
    break;
  default:
    break;
  }
}

static void
cc_output_alloc_interface (IDL_tree       tree,
			   OIDL_Run_Info *rinfo,
			   OIDL_C_Info   *ci)
{
	char *id;

	cc_output_allocs (IDL_INTERFACE (tree).body, rinfo, ci);

	id = IDL_ns_ident_to_qstring (
		IDL_IDENT_TO_NS (IDL_INTERFACE (tree).ident), "_", 0);

	fprintf (ci->fh, "\n#ifndef ORBIT_IDL_C_IMODULE_%s\n",ci->c_base_name);
	fprintf (ci->fh, "CORBA_unsigned_long %s__classid = 0;\n", id);
	fprintf (ci->fh, "#endif\n");

	g_free (id);
}

static void
cc_output_alloc_struct(IDL_tree tree, OIDL_Run_Info *rinfo, OIDL_C_Info *ci)
{
  IDL_tree sub;
  char *tname;

  cc_alloc_prep(tree, rinfo, ci);
  cc_output_allocs(IDL_TYPE_STRUCT(tree).member_list, rinfo, ci);

  tname = orbit_cbe_get_typespec_str(tree);
  /* XXX if tname is fixed_length (doesnt contain any pointers),
   * then we should #define the __freekids func */
  fprintf(ci->fh, "gpointer %s__freekids(gpointer mem, gpointer dat)\n", tname);
  fprintf(ci->fh, "{\n");
  fprintf(ci->fh, "%s *var = mem;\n", tname);
  for(sub = IDL_TYPE_STRUCT(tree).member_list; sub; sub = IDL_LIST(sub).next) {
    IDL_tree memb, sub2, ttmp;
    char *ctmp;

    memb = IDL_LIST(sub).data;

    if(orbit_cbe_type_is_fixed_length(IDL_MEMBER(memb).type_spec))
      continue;

    ttmp = orbit_cbe_get_typespec(IDL_MEMBER(memb).type_spec);
    ctmp = orbit_cbe_get_typespec_str(IDL_MEMBER(memb).type_spec);
    for(sub2 = IDL_MEMBER(memb).dcls; sub2; sub2 = IDL_LIST(sub2).next)
      fprintf(ci->fh, "%s__freekids(&(var->%s), NULL);\n", ctmp, IDL_IDENT(IDL_LIST(sub2).data).str);
    g_free(ctmp);
  }

  fprintf(ci->fh, "return (gpointer)(var + 1);\n");
  fprintf(ci->fh, "}\n\n");

  if(IDL_TYPE_STRUCT(tree).member_list) {
    fprintf(ci->fh, "%s *%s__alloc(void)\n", tname, tname);
    fprintf(ci->fh, "{\n");
    fprintf(ci->fh, "%s *retval;\n", tname);
    fprintf(ci->fh, "retval = ORBit_alloc(sizeof(%s), 1, %s__freekids);\n", tname, tname);

    for(sub = IDL_TYPE_STRUCT(tree).member_list; sub; sub = IDL_LIST(sub).next) {
      IDL_tree memb, sub2;

      memb = IDL_LIST(sub).data;

      if(orbit_cbe_type_is_fixed_length(IDL_MEMBER(memb).type_spec))
	continue;

      for(sub2 = IDL_MEMBER(memb).dcls; sub2; sub2 = IDL_LIST(sub2).next)
	fprintf(ci->fh, "memset(&(retval->%s), '\\0', sizeof(retval->%s));\n",
		IDL_IDENT(IDL_LIST(sub2).data).str,
		IDL_IDENT(IDL_LIST(sub2).data).str);
    }

    fprintf(ci->fh, "return retval;\n");
    fprintf(ci->fh, "}\n");
  }

  g_free(tname);
}

static void
cc_output_alloc_union(IDL_tree tree, OIDL_Run_Info *rinfo, OIDL_C_Info *ci)
{
  IDL_tree sub;
  char *tname;
  gboolean hit_default = FALSE;

  cc_alloc_prep(tree, rinfo, ci);
  cc_output_allocs(IDL_TYPE_UNION(tree).switch_body, rinfo, ci);

  tname = orbit_cbe_get_typespec_str(tree);

  fprintf(ci->fh, "gpointer %s__freekids(gpointer mem, gpointer dat)\n", tname);
  fprintf(ci->fh, "{\n");

  fprintf(ci->fh, "%s *val = mem;\n", tname);

  fprintf(ci->fh, "switch(val->_d) {\n");
  for(sub = IDL_TYPE_UNION(tree).switch_body; sub; sub = IDL_LIST(sub).next) {
    IDL_tree cs, sub2, memb;

    cs = IDL_LIST(sub).data;

    if(IDL_CASE_STMT(cs).labels) {
      for(sub2 = IDL_CASE_STMT(cs).labels; sub2; sub2 = IDL_LIST(sub2).next) {
	if(IDL_LIST(sub2).data) {
	  fprintf(ci->fh, "case ");
	  orbit_cbe_write_const(ci->fh, IDL_LIST(sub2).data);
	  fprintf(ci->fh, ":\n");
	} else {
	  hit_default = TRUE;
	  fprintf(ci->fh, "default:\n");
	}
      }
    } else {
      hit_default = TRUE;
      fprintf(ci->fh, "default:\n");
    }

    memb = IDL_CASE_STMT(cs).element_spec;

    if(!orbit_cbe_type_is_fixed_length(IDL_MEMBER(memb).type_spec)) {
      char *ctmp;

      ctmp = orbit_cbe_get_typespec_str(IDL_MEMBER(memb).type_spec);
      fprintf(ci->fh, "%s__freekids(&(val->_u.%s), NULL);\n",
	      ctmp, IDL_IDENT(IDL_LIST(IDL_MEMBER(memb).dcls).data).str);
      g_free(ctmp);
    }

    fprintf(ci->fh, "break;\n");
  }
  if(!hit_default)
    fprintf(ci->fh, "default:\nbreak;\n");

  fprintf(ci->fh, "}\n");
  fprintf(ci->fh, "return (gpointer)(val + 1);\n");
  fprintf(ci->fh, "}\n");

  fprintf(ci->fh, "%s* %s__alloc(void)\n", tname, tname);
  fprintf(ci->fh, "{\n");
  fprintf(ci->fh, "%s *retval;\n", tname);
  fprintf(ci->fh, "retval = ORBit_alloc(sizeof(%s), 1, %s__freekids);\n",
	  tname, tname);
  if(!orbit_cbe_type_is_fixed_length(tree))
    fprintf(ci->fh, "memset(retval, '\\0', sizeof(%s));\n", tname);

  fprintf(ci->fh, "return retval;\n");
  fprintf(ci->fh, "}\n");
}


/**
   {node} is the IDLN_ARRAY (with corresponding new ident and size_list).
   {ts} is the element type of the array.
   Outputs the defintions of the __alloc() and __freekids() functions.
   Note that the __freekids() func is not used by the __alloc(); the
   __freekids() is only used when the array in embedded in a larger type.

   I think the whole mechanism may be slightly off, because it
   doesnt honour CORBA's recursive decomposition into slices.
**/
static void
cc_output_alloc_array(IDL_tree node, IDL_tree ts, 
  OIDL_Run_Info *rinfo, OIDL_C_Info *ci)
{
    int i, n;
    char *ts_name, *tname;
    gboolean fixlen;
    IDL_tree curitem, ttmp;

    ts_name = orbit_cbe_get_typespec_str(ts);
    fixlen = orbit_cbe_type_is_fixed_length(ts);

    tname = orbit_cbe_get_typespec_str(node);
    n = IDL_list_length(IDL_TYPE_ARRAY(node).size_list);

    fprintf(ci->fh, "gpointer %s__freekids(gpointer mem, gpointer dat)\n", tname);
    fprintf(ci->fh, "{\n");
    if(fixlen) {
	fprintf(ci->fh, "gpointer retval = ((guchar *)mem) + sizeof(%s);\n", tname);
    } else {
	fprintf(ci->fh, "gpointer retval = mem, slice = mem;\n");
	for(i = 0; i < n; i++) {
	  fprintf(ci->fh, "int n%d;\n", i);
	}

	for(i = 0, ttmp = IDL_TYPE_ARRAY(node).size_list; i < n; i++, ttmp = IDL_LIST(ttmp).next) {
	  fprintf(ci->fh, "for(n%d = 0; n%d < %" IDL_LL "d; n%d++) {\n",
		  i, i, IDL_INTEGER(IDL_LIST(ttmp).data).value, i);
	}
      
	fprintf(ci->fh, "retval = %s__freekids(&((%s_slice *)slice)", ts_name, tname);
	for(i = 0; i < n; i++)
	  fprintf(ci->fh, "[n%d]", i);
	fprintf(ci->fh, ", NULL);\n");
      
	for(i = 0; i < n; i++) {
	  fprintf(ci->fh, "}\n");
	}
    }
    fprintf(ci->fh, "return retval;\n");
    fprintf(ci->fh, "}\n\n");
    /* end-of __freekids */

    fprintf(ci->fh, "%s_slice* %s__alloc(void)\n", tname, tname);
    fprintf(ci->fh, "{\n");
    fprintf(ci->fh, "%s_slice *retval;\n", tname);
    fprintf(ci->fh, "  retval = ORBit_alloc(sizeof(%s), 1", ts_name);
    curitem = IDL_TYPE_ARRAY(node).size_list;
    for(; curitem; curitem = IDL_LIST(curitem).next)
      fprintf(ci->fh, "*%" IDL_LL "d", IDL_INTEGER(IDL_LIST(curitem).data).value);
    if(fixlen) {
      fprintf(ci->fh, ", NULL);\n");
    } else {
      fprintf(ci->fh, ", %s__freekids);\n", ts_name);
      /* WATCHOUT: the __freekids above is the underlying ts, not the new type! */
      fprintf(ci->fh, "memset(retval, '\\0', sizeof(%s));\n", tname);
    }
    fprintf(ci->fh, "return retval;\n");
    fprintf(ci->fh, "}\n");

    g_free(tname);
    g_free(ts_name);
}


static void
cc_output_alloc_type_dcl(IDL_tree tree, OIDL_Run_Info *rinfo, OIDL_C_Info *ci)
{
  IDL_tree sub, ts, tts;
  gboolean fixlen;

  cc_alloc_prep(tree, rinfo, ci);
  ts = IDL_TYPE_DCL(tree).type_spec;
  tts = orbit_cbe_get_typespec(ts);

  if (IDL_NODE_TYPE(tts) == IDLN_INTERFACE
      || IDL_NODE_TYPE(tts) == IDLN_TYPE_OBJECT)
    return;

  cc_output_allocs(IDL_TYPE_DCL(tree).type_spec, rinfo, ci);

  fixlen = orbit_cbe_type_is_fixed_length(ts);

  for(sub = IDL_TYPE_DCL(tree).dcls; sub; sub = IDL_LIST(sub).next) {
    IDL_tree node;

    node = IDL_LIST(sub).data;

    switch(IDL_NODE_TYPE(node)) {
    case IDLN_IDENT:
      if(fixlen)
	continue;
      if(IDL_NODE_TYPE(tts) == IDLN_TYPE_STRING
	 || IDL_NODE_TYPE(tts) == IDLN_TYPE_WIDE_STRING)
	 continue;	/* why? - because strings don't need alloc functions */
      break;
    case IDLN_TYPE_ARRAY:
      cc_output_alloc_array(node, ts, rinfo, ci);
      break;
    default:
      g_assert_not_reached();
      break;
    }
  }
}


static void
cc_alloc_prep(IDL_tree tree, OIDL_Run_Info *rinfo, OIDL_C_Info *ci)
{
  switch(IDL_NODE_TYPE(tree)) {
  case IDLN_TYPE_SEQUENCE:
    cc_alloc_prep_sequence(tree, rinfo, ci);
    break;
  case IDLN_EXCEPT_DCL:
  case IDLN_TYPE_STRUCT:
    {
      IDL_tree sub;

      for(sub = IDL_TYPE_STRUCT(tree).member_list; sub; sub = IDL_LIST(sub).next) {
	cc_alloc_prep(IDL_MEMBER(IDL_LIST(sub).data).type_spec, rinfo, ci);
      }
    }
    break;
  case IDLN_TYPE_DCL:
    cc_alloc_prep(IDL_TYPE_DCL(tree).type_spec, rinfo, ci);
    break;
  case IDLN_TYPE_UNION:
    {
      IDL_tree sub;

      for(sub = IDL_TYPE_UNION(tree).switch_body; sub; sub = IDL_LIST(sub).next) {
	IDL_tree member;

	member = IDL_CASE_STMT(IDL_LIST(sub).data).element_spec;

	cc_alloc_prep(IDL_MEMBER(member).type_spec, rinfo, ci);
      }
    }
    break;
  case IDLN_TYPE_ARRAY:
    cc_alloc_prep(IDL_NODE_UP(tree), rinfo, ci);
    break;
  case IDLN_IDENT:
  case IDLN_LIST:
    cc_alloc_prep(IDL_NODE_UP(tree), rinfo, ci);
    break;
  default:
    break;
  }
}

static void
cc_typecode_prep_sequence(IDL_tree tree, OIDL_C_Info *ci)
{
  char *ctmp, *ctmp2;
  gboolean separate_defs;
  IDL_tree tts, fake_seq;
  gboolean fake_if = FALSE;

  tts = orbit_cbe_get_typespec(IDL_TYPE_SEQUENCE(tree).simple_type_spec);

  ctmp = orbit_cbe_get_typespec_str(IDL_TYPE_SEQUENCE(tree).simple_type_spec);
  if(IDL_NODE_TYPE(tts) == IDLN_INTERFACE) {
    ctmp2 = g_strdup("CORBA_Object");
    fake_if = TRUE;
  } else
    ctmp2 = orbit_cbe_get_typespec_str(tts);
  separate_defs = strcmp(ctmp, ctmp2);
  g_free(ctmp);
  ctmp = g_strdup_printf("CORBA_sequence_%s", ctmp2);

  if(separate_defs)
    {
      if(fake_if)
	tts = IDL_type_object_new();
      fake_seq = IDL_type_sequence_new(tts, NULL);
      IDL_NODE_UP(fake_seq) = IDL_NODE_UP(tree);

      cc_output_typecodes(fake_seq, ci);

      if(!fake_if)
	IDL_TYPE_SEQUENCE(fake_seq).simple_type_spec = NULL;
      IDL_tree_free(fake_seq);
    }
}

/**
    Note that the __freekids is always a #define in the header.
**/
static void
cc_alloc_prep_sequence(IDL_tree tree, OIDL_Run_Info *rinfo, OIDL_C_Info *ci)
{
  char *ctmp, *ctmp2;
  gboolean elements_are_fixed, separate_defs;
  IDL_tree tts, fake_seq;
  gboolean fake_if = FALSE;

  tts = orbit_cbe_get_typespec(IDL_TYPE_SEQUENCE(tree).simple_type_spec);
  cc_output_allocs(IDL_TYPE_SEQUENCE(tree).simple_type_spec, rinfo, ci);

  ctmp = orbit_cbe_get_typespec_str(IDL_TYPE_SEQUENCE(tree).simple_type_spec);
  if(IDL_NODE_TYPE(tts) == IDLN_INTERFACE) {
    ctmp2 = g_strdup("CORBA_Object");
    fake_if = TRUE;
  } else
    ctmp2 = orbit_cbe_get_typespec_str(tts);
  separate_defs = strcmp(ctmp, ctmp2);
  g_free(ctmp);
  ctmp = g_strdup_printf("CORBA_sequence_%s", ctmp2);

  if(separate_defs)
    {
      if(fake_if)
	tts = IDL_type_object_new();
      fake_seq = IDL_type_sequence_new(tts, NULL);
      IDL_NODE_UP(fake_seq) = IDL_NODE_UP(tree);
      cc_alloc_prep_sequence(fake_seq, rinfo, ci);
      if(!fake_if)
	IDL_TYPE_SEQUENCE(fake_seq).simple_type_spec = NULL;
      IDL_tree_free(fake_seq);
    }

  fprintf(ci->fh, "#if ");
  orbit_cbe_id_cond_hack(ci->fh, "ORBIT_IMPL", ctmp, ci->c_base_name);
  fprintf(ci->fh, " && !defined(ORBIT_DEF_%s)", ctmp);
  fprintf(ci->fh, " && !defined(%s__alloc)\n", ctmp);
  fprintf(ci->fh, "#define ORBIT_DEF_%s 1\n\n", ctmp);
  fprintf(ci->fh, "%s *%s__alloc(void)\n", ctmp, ctmp);
  fprintf(ci->fh, "{\n");
  fprintf(ci->fh, "  %s *retval;\n", ctmp);
  fprintf(ci->fh, "  retval = ORBit_alloc(sizeof(%s), 1, %s__freekids);\n", 
  			ctmp, ctmp);

  fprintf(ci->fh, "  retval->_maximum = ");
  if(IDL_TYPE_SEQUENCE(tree).positive_int_const) {
    orbit_cbe_write_const(ci->fh, IDL_TYPE_SEQUENCE(tree).positive_int_const);
  } else
    fprintf(ci->fh, "0");
  fprintf(ci->fh, ";\n");

  fprintf(ci->fh, "  retval->_length = 0;\n");
  fprintf(ci->fh, "  retval->_buffer = NULL;\n");
  fprintf(ci->fh, "  retval->_release = CORBA_FALSE;\n");

  fprintf(ci->fh, "  return retval;\n}\n");

  elements_are_fixed = orbit_cbe_type_is_fixed_length(IDL_TYPE_SEQUENCE(tree).simple_type_spec);
  fprintf(ci->fh, "%s* %s_allocbuf(CORBA_unsigned_long len)\n", ctmp2, ctmp);
  fprintf(ci->fh, "{\n%s* retval = ", ctmp2);
  if(elements_are_fixed) {
      fprintf(ci->fh, "ORBit_alloc_simple(sizeof(%s)*len);\n", ctmp2);
  } else {
      fprintf(ci->fh, "ORBit_alloc(sizeof(%s), len, %s__freekids);\n", 
        ctmp2, ctmp2);
      fprintf(ci->fh, "memset(retval, '\\0', sizeof(%s)*len);\n", ctmp2);
  }
  fprintf(ci->fh, "return retval;\n");
  fprintf(ci->fh, "}\n");

  fprintf(ci->fh, "#endif\n\n");

  g_free(ctmp);
  g_free(ctmp2);
}

static void
build_marshal_funcs(gpointer key, gpointer value, gpointer data)
{
  OIDL_C_Info *ci = data;
  IDL_tree tree = key;
  OIDL_Type_Marshal_Info *tmi = value;
  OIDL_Marshal_Node *node;
  OIDL_Populate_Info pi;
  char *ctmp;

  pi.ctxt = ci->ctxt;

  if(IDL_NODE_TYPE(tree) == IDLN_TYPE_SEQUENCE)
    {
      char *ctmp2;
      ctmp2 = orbit_cbe_get_typespec_str(orbit_cbe_get_typespec(IDL_TYPE_SEQUENCE(tree).simple_type_spec));
      ctmp = g_strdup_printf("CORBA_sequence_%s", ctmp2);
      g_free(ctmp2);
    }
  else
    ctmp = orbit_cbe_get_typespec_str(tree);

  if((tmi->avail_mtype & MARSHAL_FUNC) ||
     (tmi->avail_dmtype & MARSHAL_FUNC)) {
    fprintf(ci->fh, "#if ");
    orbit_cbe_id_cond_hack(ci->fh, "MARSHAL_IMPL", ctmp, ci->c_base_name);
    fprintf(ci->fh, " && !defined(ORBIT_MARSHAL_%s)\n", ctmp);
    fprintf(ci->fh, "#define ORBIT_MARSHAL_%s 1\n\n", ctmp);
    
    if(tmi->avail_mtype & MARSHAL_FUNC)
      {
        pi.flags = PI_BUILD_FUNC;
	pi.where = MW_Null|MW_Heap;
	node = marshal_populate(tree, NULL, &pi);
	orbit_idl_do_node_passes(node, FALSE);
	
	node->name = "_ORBIT_val";
	node->nptrs = oidl_param_numptrs(tree, DATA_IN);
	
	fprintf(ci->fh, "void %s_marshal(GIOPSendBuffer *_ORBIT_send_buffer, ", ctmp);
	orbit_cbe_write_param_typespec_raw(ci->fh, tree, DATA_IN);
	fprintf(ci->fh, " _ORBIT_val, CORBA_Environment *ev)\n{\n");
	orbit_cbe_alloc_tmpvars(node, ci);
	c_marshalling_generate(node, ci, FALSE);
	fprintf(ci->fh, "}\n");
      }

    if(tmi->avail_dmtype & MARSHAL_FUNC)
      {
        pi.flags = PI_BUILD_FUNC;
	pi.where = MW_Null;
	node = marshal_populate(tree, NULL, &pi);
	node->name = "_ORBIT_val";
	node->nptrs = oidl_param_numptrs(tree, DATA_IN);
	
	orbit_idl_do_node_passes(node, TRUE);
	
	fprintf(ci->fh, "gboolean\n%s_demarshal(GIOPRecvBuffer *_ORBIT_recv_buffer, ", ctmp);
	orbit_cbe_write_param_typespec_raw(ci->fh, tree, DATA_INOUT);
	fprintf(ci->fh, " _ORBIT_val, CORBA_boolean do_dup, CORBA_Environment *ev)\n{\n");
	fprintf(ci->fh, "register guchar *_ORBIT_curptr;\n");
	fprintf(ci->fh, "register guchar *_ORBIT_buf_end = _ORBIT_recv_buffer->end;\n");
	orbit_cbe_alloc_tmpvars(node, ci);
	c_demarshalling_generate(node, ci, FALSE, TRUE);
	fprintf(ci->fh, "return FALSE;");
	fprintf(ci->fh, "_ORBIT_demarshal_error:\nreturn TRUE;\n");
	fprintf(ci->fh, "}\n");
      }
    fprintf(ci->fh, "#endif\n\n");
  }
  g_free(ctmp);
}

static void
cc_output_marshallers(OIDL_C_Info *ci)
{
  g_hash_table_foreach(ci->ctxt->type_marshal_info, build_marshal_funcs, ci);
}

/************************************************/

static void
cc_small_output_iargs (FILE *of, const char *method, IDL_tree tree)
{
	IDL_tree sub;
	int      arg_count = 0;

	/* Build a list of IArgs */
	for (sub = IDL_OP_DCL (tree).parameter_dcls; sub;
	     sub = IDL_LIST (sub).next) {
		IDL_tree parm;
		char    *tc;

		if (!arg_count)
			fprintf (of, "static ORBit_IArg %s__arginfo [] = {\n", method);

		parm = IDL_LIST(sub).data;

		fprintf (of, "\t{ ");

		/* TypeCode tc */
		tc = orbit_cbe_get_typecode_name (
			IDL_PARAM_DCL (parm).param_type_spec);
		if (!tc) {
			g_warning ("Can't get typecode");
			tc = g_strdup ("NULL /* no typecode */");
		}
		fprintf (of, "%s, ", tc);

		/* IArgFlag flags */
		switch (IDL_PARAM_DCL (parm).attr) {
		case IDL_PARAM_IN:
			fprintf (of, " ORBit_I_ARG_IN ");
			break;
		case IDL_PARAM_OUT:
			fprintf (of, " ORBit_I_ARG_OUT ");
			break;
		case IDL_PARAM_INOUT:
			fprintf (of, " ORBit_I_ARG_INOUT ");
			break;
		}

		if (orbit_cbe_type_is_fixed_length (
			IDL_PARAM_DCL (parm).param_type_spec))
			fprintf (of, "| ORBit_I_COMMON_FIXED_SIZE");

		else if (IDL_PARAM_DCL(parm).attr == IDL_PARAM_OUT) {

			IDL_tree ts = orbit_cbe_get_typespec (
				IDL_PARAM_DCL (parm).param_type_spec);

			switch(IDL_NODE_TYPE (ts)) {
			case IDLN_TYPE_STRUCT:
			case IDLN_TYPE_UNION:
			case IDLN_TYPE_ARRAY:
/*				fprintf (of, "| ORBIT_I_ARG_FIXED");*/
				break;
			default:
				break;
			};
		}

		fprintf (of, ", ");

		/* string name */
		fprintf (of, "\"%s\"", IDL_IDENT (IDL_PARAM_DCL (
			IDL_LIST (sub).data).simple_declarator).str);

		fprintf (of, " }%s\n", IDL_LIST (sub).next ? "," : "");

		g_free (tc);
		arg_count++;
	}

	if (arg_count)
		fprintf (of, "};\n");
}

static void
cc_small_output_contexts (FILE *of, const char *method, IDL_tree tree)
{
	/* Build a list of contest names */
	if (IDL_OP_DCL (tree).context_expr) {
		IDL_tree curitem;

		fprintf (of, "/* Exceptions */\n");
		fprintf (of, "static CORBA_string %s__contextinfo [] = {\n",
			 method);

		for (curitem = IDL_OP_DCL (tree).context_expr; curitem;
		     curitem = IDL_LIST (curitem).next) {
			fprintf (of, "\"%s\"%c", 
				 IDL_STRING (IDL_LIST (curitem).data).value,
				 IDL_LIST (curitem).next ? ',' : ' ');
		}

		fprintf (of, "};\n");
	}
}

static void
cc_small_output_exceptinfo (FILE *of, const char *method, IDL_tree tree)
{
	/* Build a list of exception typecodes */
	if (IDL_OP_DCL (tree).raises_expr) {
		IDL_tree curitem;

		fprintf (of, "/* Exceptions */\n");
		fprintf (of, "static CORBA_TypeCode %s__exceptinfo [] = {\n",
			 method);
		
		for (curitem = IDL_OP_DCL (tree).raises_expr; curitem;
		     curitem = IDL_LIST(curitem).next) {
			char *type_id;
			IDL_tree curnode = IDL_LIST(curitem).data;
			
			type_id = orbit_cbe_get_typecode_name (curnode);
			fprintf (of, "\t%s,\n", type_id);
			g_free (type_id);
		}
		fprintf (of, "\tNULL\n};\n");
	}
}

static void
cc_small_output_method_bits (IDL_tree tree, const char *id, OIDL_C_Info *ci)
{
	OIDL_Op_Info *oi;
	FILE         *of = ci->fh;
	char         *fullname;

	fullname = g_strconcat (id, "_", IDL_IDENT (
		IDL_OP_DCL (tree).ident).str, NULL);

	oi = tree->data;
	g_assert (oi);

	cc_small_output_iargs (of, fullname, tree);

	cc_small_output_contexts (of, fullname, tree);

	cc_small_output_exceptinfo (of, fullname, tree);

	g_free (fullname);
}

static void
cc_small_output_method (FILE *of, IDL_tree tree, const char *id)
{
	int arg_count;
	int except_count;
	int context_count;
	const char *method;
	char       *fullname;

	fullname = g_strconcat (id, "_", IDL_IDENT (
		IDL_OP_DCL (tree).ident).str, NULL);

	arg_count = IDL_list_length (IDL_OP_DCL (tree).parameter_dcls);
	except_count = IDL_list_length (IDL_OP_DCL (tree).raises_expr);
	context_count = IDL_list_length (IDL_OP_DCL (tree).context_expr);
	
	fprintf (of, "\t{\n");

	/* IArgs arguments */
	if (arg_count)
		fprintf (of, "\t\t{ %d, %d, %s__arginfo, FALSE },\n",
			 arg_count, arg_count, fullname);
	else
		fprintf (of, "\t\t{ 0, 0, NULL, FALSE },\n");

	/* IContexts contexts */
	if (context_count)
		fprintf (of, "\t\t{ %d, %d, %s__contextinfo, FALSE },\n",
			 context_count, context_count, fullname);
	else
		fprintf (of, "\t\t{ 0, 0, NULL, FALSE },\n");
		
	/* ITypes exceptions */
	if (IDL_OP_DCL (tree).raises_expr)
		fprintf (of, "\t\t{ %d, %d, %s__exceptinfo, FALSE },\n",
			 except_count, except_count, fullname);
	else
		fprintf (of, "\t\t{ 0, 0, NULL, FALSE },\n");

	/* TypeCode ret */
	if (IDL_OP_DCL (tree).op_type_spec) {
		char *type_id;

		type_id = orbit_cbe_get_typespec_str (
			IDL_OP_DCL (tree).op_type_spec);
		fprintf (of, "\t\tTC_%s, ", type_id);
		g_free (type_id);
	} else
		fprintf (of, "TC_void, ");

	/* string name, long name_len */
	method = IDL_IDENT (IDL_OP_DCL (tree).ident).str;
	fprintf (of, "\"%s\", %d,\n", method, strlen (method));

	/* IMethodFlags flags */
	fprintf (of, "\t\t0");

	if (IDL_OP_DCL(tree).f_oneway)
		fprintf (of, " | ORBit_I_METHOD_1_WAY");

/* FIXME: re-scan for no_out */
/*	if (no_out)
	fprintf (of, " | ORBit_I_METHOD_NO_OUT");*/

	if (IDL_OP_DCL (tree).op_type_spec &&
	    orbit_cbe_type_is_fixed_length (
		    IDL_OP_DCL (tree).op_type_spec))
		fprintf (of, "| ORBit_I_COMMON_FIXED_SIZE");

	if (IDL_OP_DCL(tree).context_expr)
		fprintf (of, "| ORBit_I_METHOD_HAS_CONTEXT");

	fprintf (of, "\n}\n");

	g_free (fullname);
}

static void
cc_small_output_base_itypes(IDL_tree node, CCSmallInterfaceTraverseInfo *iti)
{
	if (iti->cur_node == node)
		return;

	fprintf (iti->of, "\"%s\",\n",
		 IDL_IDENT(IDL_INTERFACE(node).ident).repo_id);

	iti->parents++;
}

static void
cc_small_output_itypes (GSList *list, OIDL_C_Info *ci)
{
	GSList *l;
	FILE   *of = ci->fh;

	for (l = list; l; l = l->next) {
		CCSmallInterfaceTraverseInfo iti;
		Interface *i = l->data;
		char      *id;
		GSList    *m;

		id = IDL_ns_ident_to_qstring (IDL_IDENT_TO_NS (
			IDL_INTERFACE (i->tree).ident), "_", 0);

		for (m = i->methods; m; m = m->next)
			cc_small_output_method_bits (m->data, id, ci);

		if (i->methods) {
			fprintf (of, "\n#ifdef ORBIT_IDL_C_IMODULE_%s\n",
				 ci->c_base_name);
			fprintf (of, "static\n");
			fprintf (of, "#endif\n");

			fprintf (of, "ORBit_IMethod %s__imethods [] = {\n", id);

			if (!(m = i->methods))
				fprintf (of, "{{0}}");

			else for (; m; m = m->next) {
				cc_small_output_method (of, m->data, id);
				if (m->next)
					fprintf(of, ", ");
			}

			fprintf (of, "};");
		}

		fprintf (of, "static CORBA_string %s__base_itypes[] = {\n", id);

		iti.of = of;
		iti.cur_node = i->tree;
		iti.cur_id = id;
		iti.parents = 0;
		IDL_tree_traverse_parents(i->tree, (GFunc)cc_small_output_base_itypes, &iti);

		fprintf (of, "\"IDL:omg.org/CORBA/Object:1.0\"\n};");

		fprintf (of, "\n#ifdef ORBIT_IDL_C_IMODULE_%s\n",
			 ci->c_base_name);
		fprintf (of, "static\n");
		fprintf (of, "#endif\n");
		fprintf (of, "ORBit_IInterface %s__iinterface = {\n", id);
		fprintf (of, "TC_%s,", id);
		fprintf (of, "{%d, %d, %s__imethods, FALSE},\n",
			 g_slist_length (i->methods),
			 g_slist_length (i->methods), id);

		fprintf (of, "{%d, %d, %s__base_itypes, FALSE}\n", 
			 iti.parents + 1, iti.parents + 1, id);

		fprintf (of, "};\n\n");

		g_free (id);
	}

	for (l = list; l; l = l->next) {
		g_slist_free (((Interface *)l->data)->methods);
		g_free (l->data);
	}

	g_slist_free (list);
}

static GSList *
cc_small_build_interfaces (GSList *list, IDL_tree tree)
{
	if (!tree)
		return list;

	switch (IDL_NODE_TYPE (tree)) {
	case IDLN_MODULE:
		list = cc_small_build_interfaces (
			list, IDL_MODULE (tree).definition_list);
		break;
	case IDLN_LIST: {
		IDL_tree sub;
		for (sub = tree; sub; sub = IDL_LIST (sub).next)
			list = cc_small_build_interfaces (
				list, IDL_LIST (sub).data);
		break;
	}
	case IDLN_ATTR_DCL: {
		IDL_tree curitem;
      
		for (curitem = IDL_ATTR_DCL (tree).simple_declarations;
		     curitem; curitem = IDL_LIST (curitem).next) {
			OIDL_Attr_Info *ai = IDL_LIST (curitem).data->data;
	
			list = cc_small_build_interfaces (list, ai->op1);
			if (ai->op2)
				list = cc_small_build_interfaces (list, ai->op2);
		}
		break;
	}
	case IDLN_INTERFACE: {
		Interface *i = g_new0 (Interface, 1);

		i->tree = tree;

		list = g_slist_append (list, i);

		list = cc_small_build_interfaces (list, IDL_INTERFACE(tree).body);

		break;
	}
	case IDLN_OP_DCL: {
		Interface *i;

		g_return_val_if_fail (list != NULL, NULL);

		i = ( g_slist_last(list) )->data;
		i->methods = g_slist_append (i->methods, tree);
		break;
	}
	case IDLN_EXCEPT_DCL:
		break;
	default:
		break;
	}

	return list;
}
