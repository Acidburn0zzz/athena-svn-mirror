#include "config.h"

#include "orbit-idl-c-backend.h"

static void cc_output_tcs(IDL_tree tree, OIDL_Run_Info *rinfo, OIDL_C_Info *ci);
static void cc_output_allocs(IDL_tree tree, OIDL_Run_Info *rinfo, OIDL_C_Info *ci);
static void cc_tc_prep(IDL_tree tree, OIDL_C_Info *ci);

void
orbit_idl_output_c_common(OIDL_Output_Tree *tree, OIDL_Run_Info *rinfo, OIDL_C_Info *ci)
{
  fprintf(ci->fh, "/*\n"
  		   " * This file was generated by orbit-idl - DO NOT EDIT!\n"
		   " */\n\n");
  fprintf(ci->fh, "#include <string.h>\n");
  fprintf(ci->fh, "#include \"%s.h\"\n\n", ci->base_name);

  cc_output_tcs(tree->tree, rinfo, ci);
  cc_output_allocs(tree->tree, rinfo, ci);
}

static void
cc_output_tcs(IDL_tree tree, OIDL_Run_Info *rinfo, OIDL_C_Info *ci)
{
  if(!tree) return;

  switch(IDL_NODE_TYPE(tree)) {
  case IDLN_MODULE:
    cc_output_tcs(IDL_MODULE(tree).definition_list, rinfo, ci);
    break;
  case IDLN_LIST:
    {
      IDL_tree sub;
      for(sub = tree; sub; sub = IDL_LIST(sub).next) {
	cc_output_tcs(IDL_LIST(sub).data, rinfo, ci);
      }
    }
    break;
  case IDLN_INTERFACE:
    orbit_output_typecode(ci, tree);
    cc_output_tcs(IDL_INTERFACE(tree).body, rinfo, ci);
    break;
  case IDLN_TYPE_DCL:
  case IDLN_TYPE_STRUCT:
  case IDLN_EXCEPT_DCL:
  case IDLN_TYPE_UNION:
    cc_tc_prep(tree, ci);
  case IDLN_TYPE_ENUM:
  case IDLN_TYPE_FIXED:
    orbit_output_typecode(ci, tree);
    break;
  default:
    break;
  }
}

static void
cc_tc_prep(IDL_tree tree, OIDL_C_Info *ci)
{
  switch(IDL_NODE_TYPE(tree)) {
  case IDLN_TYPE_DCL:
    cc_tc_prep(IDL_TYPE_DCL(tree).type_spec, ci);
    break;
  case IDLN_TYPE_STRUCT:
  case IDLN_EXCEPT_DCL:
    {
      IDL_tree sub;

      for(sub = IDL_TYPE_STRUCT(tree).member_list; sub; sub = IDL_LIST(sub).next)
	cc_tc_prep(IDL_MEMBER(IDL_LIST(sub).data).type_spec, ci);
    }
    break;
  case IDLN_TYPE_UNION:
    {
      IDL_tree sub;

      for(sub = IDL_TYPE_UNION(tree).switch_body; sub; sub = IDL_LIST(sub).next) {
	IDL_tree member;

	member = IDL_CASE_STMT(IDL_LIST(sub).data).element_spec;

	cc_tc_prep(IDL_MEMBER(member).type_spec, ci);
      }
    }
    break;
  case IDLN_TYPE_SEQUENCE:
    orbit_output_typecode(ci, tree);
    break;
  default:
    break;
  }
}

/************************************************/
static void cc_alloc_prep(IDL_tree tree, OIDL_C_Info *ci);
static void cc_output_alloc_interface(IDL_tree tree, OIDL_Run_Info *rinfo, OIDL_C_Info *ci);
static void cc_output_alloc_struct(IDL_tree tree, OIDL_Run_Info *rinfo, OIDL_C_Info *ci);
static void cc_output_alloc_union(IDL_tree tree, OIDL_Run_Info *rinfo, OIDL_C_Info *ci);
static void cc_output_alloc_type_dcl(IDL_tree tree, OIDL_Run_Info *rinfo, OIDL_C_Info *ci);

static void
cc_output_allocs(IDL_tree tree, OIDL_Run_Info *rinfo, OIDL_C_Info *ci)
{
  if(!tree) return;

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
    cc_output_alloc_struct(tree, rinfo, ci);
    break;
  case IDLN_TYPE_UNION:
    cc_output_alloc_union(tree, rinfo, ci);
    break;
  case IDLN_TYPE_DCL:
    cc_output_alloc_type_dcl(tree, rinfo, ci);
    break;
  default:
    break;
  }
}

static void
cc_output_alloc_interface(IDL_tree tree, OIDL_Run_Info *rinfo, OIDL_C_Info *ci)
{
  char *id;

  cc_output_allocs(IDL_INTERFACE(tree).body, rinfo, ci);

  id = IDL_ns_ident_to_qstring(IDL_IDENT_TO_NS(IDL_INTERFACE(tree).ident), "_", 0);

  fprintf(ci->fh, "CORBA_unsigned_long %s__classid = 0;\n", id);
  g_free(id);
}

static void
cc_output_alloc_struct(IDL_tree tree, OIDL_Run_Info *rinfo, OIDL_C_Info *ci)
{
  IDL_tree sub;
  char *tname;

  cc_alloc_prep(tree, ci);

  tname = orbit_cbe_get_typename(tree);
  fprintf(ci->fh, "gpointer %s__free(gpointer mem, gpointer dat, CORBA_boolean free_strings)\n", tname);
  fprintf(ci->fh, "{\n");
  fprintf(ci->fh, "%s *var = mem;\n", tname);
  for(sub = IDL_TYPE_STRUCT(tree).member_list; sub; sub = IDL_LIST(sub).next) {
    IDL_tree memb, sub2, ttmp;
    char *ctmp;

    memb = IDL_LIST(sub).data;

    if(orbit_cbe_type_is_fixed_length(IDL_MEMBER(memb).type_spec))
      continue;

    ttmp = orbit_cbe_get_typespec(IDL_MEMBER(memb).type_spec);
    if(IDL_NODE_TYPE(ttmp) == IDLN_TYPE_STRING)
      fprintf(ci->fh, "if(free_strings) {\n");
    else
      fprintf(ci->fh, "{\n");

    ctmp = orbit_cbe_get_typename(IDL_MEMBER(memb).type_spec);
    for(sub2 = IDL_MEMBER(memb).dcls; sub2; sub2 = IDL_LIST(sub2).next)
      fprintf(ci->fh, "%s__free(&(var->%s), NULL, free_strings);\n", ctmp, IDL_IDENT(IDL_LIST(sub2).data).str);
    g_free(ctmp);
    
    fprintf(ci->fh, "}\n");
  }

  fprintf(ci->fh, "return (gpointer)(var + 1);\n");
  fprintf(ci->fh, "}\n\n");

  if(IDL_TYPE_STRUCT(tree).member_list) {
    fprintf(ci->fh, "%s *%s__alloc(void)\n", tname, tname);
    fprintf(ci->fh, "{\n");
    fprintf(ci->fh, "%s *retval;\n", tname);
    fprintf(ci->fh, "retval = ORBit_alloc(sizeof(%s), (ORBit_free_childvals)%s__free, GUINT_TO_POINTER(1));\n", tname, tname);

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

  cc_alloc_prep(tree, ci);

  tname = orbit_cbe_get_typename(tree);

  fprintf(ci->fh, "gpointer %s__free(gpointer mem, gpointer dat, CORBA_boolean free_strings)\n", tname);
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

      ctmp = orbit_cbe_get_typename(IDL_MEMBER(memb).type_spec);
      fprintf(ci->fh, "%s__free(&(val->_u.%s), NULL, free_strings);\n",
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
  fprintf(ci->fh, "retval = ORBit_alloc(sizeof(%s), (ORBit_free_childvals)%s__free, GUINT_TO_POINTER(1));\n",
	  tname, tname);
  if(!orbit_cbe_type_is_fixed_length(tree))
    fprintf(ci->fh, "memset(retval, '\\0', sizeof(%s));\n", tname);

  fprintf(ci->fh, "return retval;\n");
  fprintf(ci->fh, "}\n");
}

static void
cc_output_alloc_type_dcl(IDL_tree tree, OIDL_Run_Info *rinfo, OIDL_C_Info *ci)
{
  IDL_tree sub, ts, tts;
  int i, n;
  char *ctmp;
  gboolean fixlen;

  cc_alloc_prep(tree, ci);
  ts = IDL_TYPE_DCL(tree).type_spec;
  tts = orbit_cbe_get_typespec(ts);

  if (IDL_NODE_TYPE(tts) == IDLN_INTERFACE
      || IDL_NODE_TYPE(tts) == IDLN_TYPE_OBJECT)
    return;

  ctmp = orbit_cbe_get_typename(ts);

  fixlen = orbit_cbe_type_is_fixed_length(ts);

  for(sub = IDL_TYPE_DCL(tree).dcls; sub; sub = IDL_LIST(sub).next) {
    IDL_tree node, ident, ttmp;
    char *tname;

    node = IDL_LIST(sub).data;

    switch(IDL_NODE_TYPE(node)) {
    case IDLN_IDENT:
      if(fixlen)
	continue;
      ident = node;
      if(IDL_NODE_TYPE(tts) == IDLN_TYPE_STRING
	 || IDL_NODE_TYPE(tts) == IDLN_TYPE_WIDE_STRING) continue;
      break;
    case IDLN_TYPE_ARRAY:
      ident = IDL_TYPE_ARRAY(node).ident;
      break;
    default:
      g_assert_not_reached();
      break;
    }

    tname = orbit_cbe_get_typename(node);

    fprintf(ci->fh, "gpointer %s__free(gpointer mem, gpointer dat, CORBA_boolean free_strings)\n", tname);
    fprintf(ci->fh, "{\n");

    switch(IDL_NODE_TYPE(node)) {
    case IDLN_IDENT:
      fprintf(ci->fh, "return %s__free(mem, dat, free_strings);\n", ctmp);
      break;
    case IDLN_TYPE_ARRAY:
      n = IDL_list_length(IDL_TYPE_ARRAY(node).size_list);

      if(fixlen) {
	fprintf(ci->fh, "gpointer retval = ((guchar *)mem) + sizeof(%s);\n", tname);
      } else {
	fprintf(ci->fh, "gpointer retval = mem;\n");
	for(i = 0; i < n; i++) {
	  fprintf(ci->fh, "int n%d;\n", i);
	}

	for(i = 0, ttmp = IDL_TYPE_ARRAY(node).size_list; i < n; i++, ttmp = IDL_LIST(ttmp).next) {
	  fprintf(ci->fh, "for(n%d = 0; n%d < %" IDL_LL "d; n%d++) {\n",
		  i, i, IDL_INTEGER(IDL_LIST(ttmp).data).value, i);
	}
      
	fprintf(ci->fh, "retval = %s__free(&((%s_slice *)retval)", ctmp, tname);
	for(i = 0; i < n; i++)
	  fprintf(ci->fh, "[n%d]", i);
	fprintf(ci->fh, ", NULL, free_strings);\n");
      
	for(i = 0; i < n; i++) {
	  fprintf(ci->fh, "}\n");
	}
      }

      fprintf(ci->fh, "return retval;\n");

      break;
    default:
      break;
    }
    fprintf(ci->fh, "}\n\n");

    fprintf(ci->fh, "%s%s %s__alloc(void)\n", tname, (IDL_NODE_TYPE(node) == IDLN_TYPE_ARRAY)?"_slice*":"*", tname);
    fprintf(ci->fh, "{\n");

    if(IDL_NODE_TYPE(node) == IDLN_TYPE_ARRAY) {

      fprintf(ci->fh, "%s_slice *retval;\n", tname);
      fprintf(ci->fh, "  retval = ORBit_alloc(sizeof(%s), (ORBit_free_childvals)", tname);

      if(fixlen)
	fprintf(ci->fh, "NULL, NULL);\n");
      else {
	IDL_tree curitem;

	curitem = IDL_TYPE_ARRAY(node).size_list;
	fprintf(ci->fh, "%s__free, GUINT_TO_POINTER(%" IDL_LL "d", tname, IDL_INTEGER(IDL_LIST(curitem).data).value);
	for(; curitem; curitem = IDL_LIST(curitem).next)
	  fprintf(ci->fh, "*%" IDL_LL "d", IDL_INTEGER(IDL_LIST(curitem).data).value);
	fprintf(ci->fh, "));\n");

	curitem = IDL_TYPE_ARRAY(node).size_list;
	fprintf(ci->fh, "memset(retval, '\\0', sizeof(%s_slice) * %" IDL_LL "d);\n", tname,
		IDL_INTEGER(IDL_LIST(curitem).data).value);
      }

      fprintf(ci->fh, "return retval;\n");
    } else {
      fprintf(ci->fh, "return %s__alloc();\n", ctmp);
    }

    fprintf(ci->fh, "}\n");

    g_free(tname);
  }

  g_free(ctmp);
}

static void cc_alloc_prep_sequence(IDL_tree tree, OIDL_C_Info *ci);

static void
cc_alloc_prep(IDL_tree tree, OIDL_C_Info *ci)
{
  switch(IDL_NODE_TYPE(tree)) {
  case IDLN_TYPE_SEQUENCE:
    cc_alloc_prep_sequence(tree, ci);
    break;
  case IDLN_EXCEPT_DCL:
  case IDLN_TYPE_STRUCT:
    {
      IDL_tree sub;

      for(sub = IDL_TYPE_STRUCT(tree).member_list; sub; sub = IDL_LIST(sub).next) {
	cc_alloc_prep(IDL_MEMBER(IDL_LIST(sub).data).type_spec, ci);
      }
    }
    break;
  case IDLN_TYPE_DCL:
    cc_alloc_prep(IDL_TYPE_DCL(tree).type_spec, ci);
    break;
  case IDLN_TYPE_UNION:
    {
      IDL_tree sub;

      for(sub = IDL_TYPE_UNION(tree).switch_body; sub; sub = IDL_LIST(sub).next) {
	IDL_tree member;

	member = IDL_CASE_STMT(IDL_LIST(sub).data).element_spec;

	cc_alloc_prep(IDL_MEMBER(member).type_spec, ci);
      }
    }
    break;
  case IDLN_TYPE_ARRAY:
    cc_alloc_prep(IDL_NODE_UP(tree), ci);
    break;
  case IDLN_IDENT:
  case IDLN_LIST:
    cc_alloc_prep(IDL_NODE_UP(tree), ci);
    break;
  default:
    break;
  }
}

static void
cc_alloc_prep_sequence(IDL_tree tree, OIDL_C_Info *ci)
{
  char *ctmp, *ctmp2;
  gboolean elements_are_fixed;

  ctmp = orbit_cbe_get_typename(tree);

  fprintf(ci->fh, "#if ");
  orbit_cbe_id_cond_hack(ci->fh, "ORBIT_IMPL", ctmp, ci->c_base_name);
  fprintf(ci->fh, " && !defined(ORBIT_DEF_%s)\n", ctmp);
  fprintf(ci->fh, "#define ORBIT_DEF_%s 1\n\n", ctmp);
  fprintf(ci->fh, "gpointer %s__free(gpointer mem, gpointer dat, CORBA_boolean free_strings)\n", ctmp);
  fprintf(ci->fh, "{\n");
  fprintf(ci->fh, "  %s* val = mem;\n", ctmp);
  fprintf(ci->fh, "  if(val->_release)");
  fprintf(ci->fh, "    ORBit_free(val->_buffer, free_strings);\n");
  fprintf(ci->fh, "  return (gpointer)(val + 1);\n");
  fprintf(ci->fh, "}\n\n");

  fprintf(ci->fh, "%s *%s__alloc(void)\n", ctmp, ctmp);
  fprintf(ci->fh, "{\n");
  fprintf(ci->fh, "  %s *retval;\n", ctmp);
  fprintf(ci->fh, "  retval = ORBit_alloc(sizeof(%s), (ORBit_free_childvals)%s__free, GUINT_TO_POINTER(1));\n", ctmp, ctmp);

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

  orbit_cbe_write_typespec(ci->fh, IDL_TYPE_SEQUENCE(tree).simple_type_spec);
  fprintf(ci->fh, "* %s_allocbuf(CORBA_unsigned_long len)\n", ctmp);
  fprintf(ci->fh, "{\n");
  orbit_cbe_write_typespec(ci->fh, IDL_TYPE_SEQUENCE(tree).simple_type_spec);
  fprintf(ci->fh, "* retval = ORBit_alloc(sizeof(");
  orbit_cbe_write_typespec(ci->fh, IDL_TYPE_SEQUENCE(tree).simple_type_spec);
  fprintf(ci->fh, ")*len, (ORBit_free_childvals)");
  elements_are_fixed = orbit_cbe_type_is_fixed_length(IDL_TYPE_SEQUENCE(tree).simple_type_spec);
  if(elements_are_fixed)
    fprintf(ci->fh, "NULL");
  else {
    ctmp2 = orbit_cbe_get_typename(IDL_TYPE_SEQUENCE(tree).simple_type_spec);
    fprintf(ci->fh, "%s__free", ctmp2);
    g_free(ctmp2);
  }
  fprintf(ci->fh, ", GUINT_TO_POINTER(len));\n");
  if(!elements_are_fixed) {
    fprintf(ci->fh, "memset(retval, '\\0', sizeof(");
    orbit_cbe_write_typespec(ci->fh, IDL_TYPE_SEQUENCE(tree).simple_type_spec);
    fprintf(ci->fh, ")*len);\n");
  }
  fprintf(ci->fh, "return retval;\n");

  fprintf(ci->fh, "}\n");

  fprintf(ci->fh, "#endif\n\n");

  g_free(ctmp);
}
