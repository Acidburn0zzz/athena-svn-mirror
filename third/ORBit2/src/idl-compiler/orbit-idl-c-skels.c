#include "config.h"
#include <string.h>
#include "orbit-idl-c-backend.h"

typedef struct {
  OIDL_C_Info *ci;
  GSList *oplist;
  gint curlevel;
  gboolean small;
} CBESkelInterfaceTraverseInfo;

typedef struct {
  char *iface_id;
  char *opname;
  IDL_tree op;
  int      idx;
} CBESkelOpInfo;

static void ck_output_skels(IDL_tree tree, OIDL_Run_Info *rinfo, OIDL_C_Info *ci, int *idx);
static void ck_output_poastuff(IDL_tree tree, OIDL_Run_Info *rinfo, OIDL_C_Info *ci);

void
orbit_idl_output_c_skeletons(OIDL_Output_Tree *tree, OIDL_Run_Info *rinfo, OIDL_C_Info *ci)
{
  fprintf (ci->fh, OIDL_C_WARNING);
  fprintf(ci->fh, "#include <string.h>\n");
  fprintf(ci->fh, "#define ORBIT2_STUBS_API\n");
  fprintf(ci->fh, "#include \"%s.h\"\n\n", ci->base_name);

  ck_output_skels(tree->tree, rinfo, ci, NULL);
  ck_output_poastuff(tree->tree, rinfo, ci);
}

static void ck_output_skel(IDL_tree tree, OIDL_C_Info *ci);
static void ck_output_small_skel(IDL_tree tree, OIDL_C_Info *ci, int *idx);
static void ck_output_except(IDL_tree tree, OIDL_C_Info *ci);

static void
ck_output_skels(IDL_tree tree, OIDL_Run_Info *rinfo, OIDL_C_Info *ci, int *idx)
{
  if ( !tree || (tree->declspec & IDLF_DECLSPEC_PIDL) )
    return;


  switch(IDL_NODE_TYPE(tree)) {
  case IDLN_MODULE:
    ck_output_skels(IDL_MODULE(tree).definition_list, rinfo, ci, idx);
    break;
  case IDLN_LIST:
    {
      IDL_tree sub;
      for(sub = tree; sub; sub = IDL_LIST(sub).next) {
	ck_output_skels(IDL_LIST(sub).data, rinfo, ci, idx);
      }
    }
    break;
  case IDLN_ATTR_DCL:
    {
      OIDL_Attr_Info *ai = tree->data;

      IDL_tree curitem;
      
      for(curitem = IDL_ATTR_DCL(tree).simple_declarations; curitem; curitem = IDL_LIST(curitem).next) {
	ai = IDL_LIST(curitem).data->data;
	
	ck_output_skels(ai->op1, rinfo, ci, idx);
	if(ai->op2)
	  ck_output_skels(ai->op2, rinfo, ci, idx);
      }
    }
    break;
  case IDLN_INTERFACE: {
    int real_idx = 0;  
    ck_output_skels(IDL_INTERFACE(tree).body, rinfo, ci, &real_idx);
    break;
  }
  case IDLN_OP_DCL:
    if (rinfo->small_skels)
      ck_output_small_skel(tree, ci, idx);
    else
      ck_output_skel(tree, ci);
    break;
  case IDLN_EXCEPT_DCL:
    if (!rinfo->small_skels)
      ck_output_except(tree, ci);
    break;
  default:
    break;
  }
}


static void
cbe_print_var_dcl(FILE *of, IDL_tree tree)
{
    IDL_tree 		rawts = NULL /* Quiet gcc */, ts;
    IDL_ParamRole	role = 0 /* Quiet gcc */;
    gchar		*id = NULL /* Quiet gcc */, *ts_str;
    gboolean		isSlice;
    int			n;

    if (IDL_NODE_TYPE(tree) == IDLN_OP_DCL) {
	rawts = IDL_OP_DCL(tree).op_type_spec;
	role = DATA_RETURN;
	id = "_ORBIT_retval";
    } else if (IDL_NODE_TYPE(tree) == IDLN_PARAM_DCL) {
    	rawts = IDL_PARAM_DCL(tree).param_type_spec;
    	role = oidl_attr_to_paramrole(IDL_PARAM_DCL(tree).attr);
    	id = IDL_IDENT(IDL_PARAM_DCL(tree).simple_declarator).str;
    } else
      g_error("Unexpected tree node");

    ts = orbit_cbe_get_typespec( rawts );
    ts_str = orbit_cbe_get_typespec_str(ts);
    n = oidl_param_info(ts, role, &isSlice);

    if ( IDL_NODE_TYPE(ts)==IDLN_TYPE_ARRAY )
      {
	if ( isSlice==0 ) {
	  g_assert( n==0 );
	  fprintf(of, "%s %s;\n", ts_str, id);
	} else {
	  if ( n==1 ) {
	    fprintf(of, "%s_slice *%s;\n", ts_str, id);
	  } else {
	    g_assert( n==2 );
	    fprintf(of, "%s_slice *_ref_%s; %s_slice **%s = &_ref_%s;\n", ts_str, id, ts_str, id, id);
	  }
	}
      }
    else
      {
	switch(n)
	  {
	  case 0:
	    fprintf(of, "%s %s;\n", ts_str, id);
	    break;
	  case 1:
	    if ( role == DATA_RETURN)
	      fprintf(of, "%s *%s;\n", ts_str, id);
	    else
	      fprintf(of, "%s _val_%s; %s *%s = &_val_%s;\n", ts_str, id, ts_str, id, id);
	    break;
	  case 2:
	    fprintf(of, "%s *_ref_%s; %s **%s = &_ref_%s;\n", ts_str, id, ts_str, id, id);
	    break; 
	  default:
	    g_assert_not_reached();
	    break;
	  }
      }
    g_free(ts_str);
}

static void
cbe_skel_op_dcl_print_call_param(IDL_tree tree, OIDL_C_Info *ci)
{
  gchar *id = IDL_IDENT(IDL_PARAM_DCL(tree).simple_declarator).str;
  IDL_tree ts;

  ts = orbit_cbe_get_typespec(IDL_PARAM_DCL(tree).param_type_spec);
  if ( IDL_NODE_TYPE(ts)==IDLN_TYPE_ARRAY 
       && IDL_PARAM_DCL(tree).attr == IDL_PARAM_IN) {
    gchar *ts_str = orbit_cbe_get_typespec_str(ts);
    fprintf(ci->fh, "(const %s_slice*)", ts_str);
    g_free(ts_str);
  }
  fprintf(ci->fh, "%s", id);
}

static void cbe_skel_op_params_free(IDL_tree tree, OIDL_C_Info *ci);
static void cbe_skel_op_dcl_print_call_param(IDL_tree tree, OIDL_C_Info *ci);

static void
ck_output_skel(IDL_tree tree, OIDL_C_Info *ci)
{
  char *opname, *ifname;
  IDL_tree intf, curitem;
  OIDL_Op_Info *oi;

  intf = IDL_get_parent_node(tree, IDLN_INTERFACE, NULL);

  opname = IDL_ns_ident_to_qstring(IDL_IDENT_TO_NS(IDL_OP_DCL(tree).ident), "_", 0);
  ifname = IDL_ns_ident_to_qstring(IDL_IDENT_TO_NS(IDL_INTERFACE(intf).ident), "_", 0);

  fprintf(ci->fh, "void _ORBIT_skel_%s(POA_%s * _ORBIT_servant, GIOPRecvBuffer *_ORBIT_recv_buffer, CORBA_Environment *ev, ",
          opname, ifname);
  orbit_cbe_op_write_proto(ci->fh, tree, "_impl_", TRUE);
  fprintf(ci->fh, ")\n");
  fprintf(ci->fh, "{\n");
  fprintf(ci->fh, "register guchar *_ORBIT_buf_end G_GNUC_UNUSED = _ORBIT_recv_buffer->end;\n");

  oi = tree->data;

  if(IDL_OP_DCL(tree).op_type_spec)
    cbe_print_var_dcl(ci->fh, tree);
  for(curitem = IDL_OP_DCL(tree).parameter_dcls; curitem; curitem = IDL_LIST(curitem).next) {
    IDL_tree param = IDL_LIST(curitem).data;
    cbe_print_var_dcl(ci->fh, param);
  }

  if(IDL_OP_DCL(tree).context_expr)
    fprintf(ci->fh, "struct CORBA_Context_type _ctx;\n");

  if(oi->in_skels) {
    fprintf(ci->fh, "{ /* demarshalling */\n");
    fprintf(ci->fh, "register guchar *_ORBIT_curptr G_GNUC_UNUSED;\n");
    
    orbit_cbe_alloc_tmpvars(oi->in_skels, ci);
    c_demarshalling_generate(oi->in_skels, ci, TRUE, FALSE);
    
    fprintf(ci->fh, "}\n");
  }

  if(IDL_OP_DCL(tree).op_type_spec)
    fprintf(ci->fh, "_ORBIT_retval = ");
  fprintf(ci->fh, "_impl_%s(_ORBIT_servant, ", IDL_IDENT(IDL_OP_DCL(tree).ident).str);
  for(curitem = IDL_OP_DCL(tree).parameter_dcls; curitem; curitem = IDL_LIST(curitem).next) {
    cbe_skel_op_dcl_print_call_param(IDL_LIST(curitem).data, ci);
    fprintf(ci->fh, ", ");
  }
  if(IDL_OP_DCL(tree).context_expr)
    fprintf(ci->fh, "&_ctx, ");
  fprintf(ci->fh, "ev);\n");

  if(!IDL_OP_DCL(tree).f_oneway) {
    fprintf(ci->fh, "{ /* marshalling */\n");
    fprintf(ci->fh, "register GIOPSendBuffer *_ORBIT_send_buffer;\n");

    fprintf(ci->fh, "_ORBIT_send_buffer = giop_send_buffer_use_reply(_ORBIT_recv_buffer->connection->giop_version,"
	    "giop_recv_buffer_get_request_id(_ORBIT_recv_buffer), ev->_major);\n");

    fprintf(ci->fh, "if(_ORBIT_send_buffer) {\n");
    fprintf(ci->fh, "if (ev->_major == CORBA_NO_EXCEPTION) {\n");
    orbit_cbe_alloc_tmpvars(oi->out_skels, ci);

    c_marshalling_generate(oi->out_skels, ci, TRUE);

    if(IDL_OP_DCL(tree).raises_expr) {
      IDL_tree curitem;
      
      fprintf(ci->fh, "} else if (ev->_major == CORBA_USER_EXCEPTION) { \n");
      fprintf(ci->fh, "static const ORBit_exception_marshal_info _ORBIT_user_exceptions[] = { ");
      for(curitem = IDL_OP_DCL(tree).raises_expr; curitem;
	  curitem = IDL_LIST(curitem).next) {
	char *id;
	IDL_tree curnode = IDL_LIST(curitem).data;
	
	id = orbit_cbe_get_typespec_str(curnode);
	fprintf(ci->fh, "{(const CORBA_TypeCode)&TC_%s_struct, (gpointer)_ORBIT_%s_marshal},",
		id, id);
	g_free(id);
      }

      fprintf(ci->fh, "{CORBA_OBJECT_NIL, NULL}};\n");
      fprintf(ci->fh, "ORBit_send_user_exception(_ORBIT_send_buffer, ev, _ORBIT_user_exceptions);\n");
    }

    fprintf(ci->fh, "} else\n");
    fprintf(ci->fh, "ORBit_send_system_exception(_ORBIT_send_buffer, ev);\n");

    fprintf(ci->fh, "giop_send_buffer_write(_ORBIT_send_buffer, _ORBIT_recv_buffer->connection);\n");
    fprintf(ci->fh, "giop_send_buffer_unuse(_ORBIT_send_buffer);\n");
    fprintf(ci->fh, "}\n");

    fprintf(ci->fh, "}\n");
  }

  cbe_skel_op_params_free(tree, ci);

  fprintf(ci->fh, "}\n");

  g_free(opname);
  g_free(ifname);
}

static void
ck_output_small_skel(IDL_tree tree, OIDL_C_Info *ci, int *idx)
{
  char *opname, *ifname;
  IDL_tree intf;
  gboolean has_args, has_retval;

  g_return_if_fail (idx != NULL);

  intf = IDL_get_parent_node(tree, IDLN_INTERFACE, NULL);
  has_args   = IDL_OP_DCL(tree).parameter_dcls != NULL;
  has_retval = IDL_OP_DCL(tree).op_type_spec != NULL;

  opname = IDL_ns_ident_to_qstring(IDL_IDENT_TO_NS(IDL_OP_DCL(tree).ident), "_", 0);
  ifname = IDL_ns_ident_to_qstring(IDL_IDENT_TO_NS(IDL_INTERFACE(intf).ident), "_", 0);

  fprintf(ci->fh, "void _ORBIT_skel_small_%s("
	  "POA_%s             *_o_servant, "
	  "gpointer            _o_retval,"
	  "gpointer           *_o_args,"
	  "CORBA_Context       _o_ctx,"
	  "CORBA_Environment  *_o_ev,\n", opname, ifname);
  orbit_cbe_op_write_proto(ci->fh, tree, "_impl_", TRUE);
  fprintf(ci->fh, ")\n");
  fprintf(ci->fh, "{\n");

  if(has_retval) {
    fprintf(ci->fh, "*(");
    orbit_cbe_write_param_typespec(ci->fh, tree);
    fprintf(ci->fh, " *)_o_retval = ");
  }

  fprintf(ci->fh, "_impl_%s(_o_servant, ", IDL_IDENT(IDL_OP_DCL(tree).ident).str);
  
  cbe_small_unflatten_args (tree, ci->fh, "_o_args");

  if(IDL_OP_DCL(tree).context_expr)
    fprintf(ci->fh, "_o_ctx, ");

  fprintf(ci->fh, "_o_ev);\n");

  fprintf (ci->fh, "}\n");

  g_free(opname);
  g_free(ifname);

  (*idx)++;
}

static void
ck_output_except(IDL_tree tree, OIDL_C_Info *ci)
{
  char *id;
  OIDL_Except_Info *ei;

  ei = tree->data;
  g_assert(ei);

  id = IDL_ns_ident_to_qstring(IDL_IDENT_TO_NS(IDL_EXCEPT_DCL(tree).ident), "_", 0);

  fprintf(ci->fh, "void\n_ORBIT_%s_marshal(GIOPSendBuffer *_ORBIT_send_buffer, CORBA_Environment *ev)\n", id);
  fprintf(ci->fh, "{\n");
  if(IDL_EXCEPT_DCL(tree).members) {
    orbit_cbe_alloc_tmpvars(ei->demarshal, ci);
    fprintf(ci->fh, "%s *_ORBIT_exdata = ev->_any._value;\n", id);
    c_marshalling_generate(ei->marshal, ci, FALSE);
  }

  fprintf(ci->fh, "}\n");

  g_free(id);
}

/** 
   NOTE: we no longer use {free_internal}. Instead, any data we
   	do not want to free is tagged as such at run-time 
	at the time we reference it.
   	See ORBit/docs/orbit-mem2.txt.
**/
static void
cbe_skel_param_subfree(IDL_tree tree, OIDL_C_Info *ci)
{
  char *id, *varname;

  if(IDL_NODE_TYPE(tree) != IDLN_PARAM_DCL) {
    id = orbit_cbe_get_typespec_str(tree);
    varname = "_ORBIT_retval";
  } else {
    id = orbit_cbe_get_typespec_str(IDL_PARAM_DCL(tree).param_type_spec);
    varname = IDL_IDENT(IDL_PARAM_DCL(tree).simple_declarator).str;
  }
  fprintf(ci->fh, "%s__freekids(%s, NULL);\n", id, varname);
  g_free(id);
}

void
cbe_op_retval_free(IDL_tree tree, OIDL_C_Info *ci)
{
  IDL_tree ts;

  ts = orbit_cbe_get_typespec(tree);

  switch(IDL_NODE_TYPE(ts)) {
  case IDLN_TYPE_UNION:
  case IDLN_TYPE_STRUCT:
    if(orbit_cbe_type_is_fixed_length(ts))
      return;
  case IDLN_TYPE_SEQUENCE:
  case IDLN_TYPE_STRING:
  case IDLN_TYPE_ARRAY:
  case IDLN_TYPE_ANY:
    break;
  case IDLN_TYPE_OBJECT:
  case IDLN_INTERFACE:
  case IDLN_FORWARD_DCL:
    fprintf(ci->fh, "CORBA_Object_release(" ORBIT_RETVAL_VAR_NAME ", ev);\n");
  default:
    return;
  }

  fprintf(ci->fh, "CORBA_free(" ORBIT_RETVAL_VAR_NAME ");\n");
}

static gboolean
cbe_skel_op_param_has_sequence(IDL_tree ts)
{
  gboolean has_seq = FALSE, subhas;
  IDL_tree curitem, sn;

  ts = orbit_cbe_get_typespec(ts);

  switch(IDL_NODE_TYPE(ts)) {
  case IDLN_TYPE_UNION:
    for(curitem = IDL_TYPE_UNION(ts).switch_body; curitem;
	curitem = IDL_LIST(curitem).next) {
      sn = IDL_MEMBER(IDL_CASE_STMT(IDL_LIST(curitem).data).element_spec).type_spec;
      subhas = cbe_skel_op_param_has_sequence(sn);
      has_seq = has_seq || subhas;
    }
    return has_seq;
    break;
  case IDLN_TYPE_STRUCT:
    for(curitem = IDL_TYPE_STRUCT(ts).member_list; curitem;
	curitem = IDL_LIST(curitem).next) {
      sn = IDL_MEMBER(IDL_LIST(curitem).data).type_spec;
      subhas = cbe_skel_op_param_has_sequence(sn);
      has_seq = has_seq || subhas;
    }
    return has_seq;
    break;
  case IDLN_TYPE_ARRAY:
    return cbe_skel_op_param_has_sequence(IDL_TYPE_DCL(IDL_get_parent_node(ts, IDLN_TYPE_DCL, NULL)).type_spec);
    break;
  case IDLN_TYPE_SEQUENCE:
    return TRUE;
  default:
    return FALSE;
  }
}

void
cbe_op_param_free(IDL_tree tree, OIDL_C_Info *ci, gboolean is_skels)
{
  IDL_tree ts;

  ts = orbit_cbe_get_typespec(tree);
  if ( orbit_cbe_type_is_fixed_length(ts) )
	return;

  switch(IDL_PARAM_DCL(tree).attr) {
  case IDL_PARAM_IN:
    if(is_skels)
      switch(IDL_NODE_TYPE(ts)) {
      case IDLN_TYPE_UNION:
      case IDLN_TYPE_STRUCT:
      case IDLN_TYPE_ARRAY:
      case IDLN_TYPE_ANY:
      case IDLN_TYPE_SEQUENCE:
	/* ANY and SEQUENCE always have allocated sub-memory that must
	 * be freed, regardless of the underlying types */
	cbe_skel_param_subfree(tree, ci);
	break;
      case IDLN_TYPE_OBJECT:
      case IDLN_INTERFACE:
      case IDLN_FORWARD_DCL:
	fprintf(ci->fh, "CORBA_Object_release(%s, ev);\n",
		IDL_IDENT(IDL_PARAM_DCL(tree).simple_declarator).str);
	break;
      default:
	break;
      }
    break;
  case IDL_PARAM_OUT:
    switch(IDL_NODE_TYPE(ts)) {
    case IDLN_TYPE_OBJECT:
    case IDLN_INTERFACE:
      fprintf(ci->fh, "CORBA_Object_release(*%s, ev);\n",
	      IDL_IDENT(IDL_PARAM_DCL(tree).simple_declarator).str);
      break;
    default:
      fprintf(ci->fh, "CORBA_free(*%s);\n",
	      IDL_IDENT(IDL_PARAM_DCL(tree).simple_declarator).str);
    }
    break;
  case IDL_PARAM_INOUT:
    switch(IDL_NODE_TYPE(ts)) {
    case IDLN_TYPE_OBJECT:
    case IDLN_INTERFACE:
      fprintf(ci->fh, "CORBA_Object_release(*%s, ev);\n",
	      IDL_IDENT(IDL_PARAM_DCL(tree).simple_declarator).str);
      break;
    default:
      cbe_skel_param_subfree(tree, ci);
      break;
    }
    break;
  }

}

static void
cbe_skel_op_params_free(IDL_tree tree, OIDL_C_Info *ci)
{
  IDL_tree curitem;

  /* We have to free the params from last to first */
  fprintf(ci->fh, "if(ev->_major == CORBA_NO_EXCEPTION)\n{\n");
  for(curitem = IDL_list_nth(IDL_OP_DCL(tree).parameter_dcls, IDL_list_length(IDL_OP_DCL(tree).parameter_dcls) - 1);
      curitem; curitem = IDL_LIST(curitem).prev)
    {
      IDL_tree param;

      param = IDL_LIST(curitem).data;
      if(IDL_PARAM_DCL(param).attr == IDL_PARAM_OUT)
	cbe_op_param_free(param, ci, TRUE);
    }

  if(IDL_OP_DCL(tree).op_type_spec)
    cbe_op_retval_free(IDL_OP_DCL(tree).op_type_spec, ci);
  fprintf(ci->fh, "}\n");

  /* Concious decision made to not send back an exception to report the problem - if they are sending us total junk,
     then it is probably not even a compliant ORB that needs to know. The real reason is that it would complicate the
     code a lot, just to handle this corner case */
  if(IDL_OP_DCL(tree).context_expr)
    fprintf(ci->fh, "_context_demarshal_error: ORBit_Context_server_free(&_ctx);\n");
  for(curitem = IDL_list_nth(IDL_OP_DCL(tree).parameter_dcls, IDL_list_length(IDL_OP_DCL(tree).parameter_dcls) - 1);
      curitem; curitem = IDL_LIST(curitem).prev)
    {
      IDL_tree param;

      param = IDL_LIST(curitem).data;
      if(IDL_PARAM_DCL(param).attr != IDL_PARAM_OUT)
	{
	  fprintf(ci->fh, "%s_demarshal_error:\n", IDL_IDENT(IDL_PARAM_DCL(param).simple_declarator).str);
	  cbe_op_param_free(param, ci, TRUE);
	}
    }
  if(IDL_OP_DCL(tree).parameter_dcls)
    fprintf(ci->fh, "_ORBIT_demarshal_error:\n");
}


/*****************************************/
static void cbe_skel_do_interface(IDL_tree tree, OIDL_Run_Info *rinfo, OIDL_C_Info *ci);

static void
ck_output_poastuff(IDL_tree tree, OIDL_Run_Info *rinfo, OIDL_C_Info *ci)
{
  if( !tree || (tree->declspec & IDLF_DECLSPEC_PIDL)!=0 ) 
    return;

  switch(IDL_NODE_TYPE(tree)) {
  case IDLN_MODULE:
    ck_output_poastuff(IDL_MODULE(tree).definition_list, rinfo, ci);
    break;
  case IDLN_LIST:
    {
      IDL_tree sub;
      for(sub = tree; sub; sub = IDL_LIST(sub).next) {
	ck_output_poastuff(IDL_LIST(sub).data, rinfo, ci);
      }
    }
    break;
  case IDLN_INTERFACE:
    cbe_skel_do_interface(tree, rinfo, ci);
    break;
  default:
    break;
  }
}

/* Blatantly copied from the old IDL compiler. (A few fixes to the
   get_skel generation stuff to do proper checking of the opname...) */

#if 0
static void
cbe_skel_print_skelptr(FILE *of, IDL_tree tree)
{
  char *id = NULL;
  IDL_tree curitem;

  switch(IDL_NODE_TYPE(tree)) {
  case IDLN_OP_DCL:
    id = IDL_ns_ident_to_qstring(IDL_IDENT_TO_NS(IDL_OP_DCL(tree).ident),
				 "_", 0);
    fprintf(of, "  skel_%s,\n", id);
    break;
  case IDLN_ATTR_DCL:
    id = IDL_ns_ident_to_qstring(IDL_IDENT_TO_NS(IDL_INTERFACE(IDL_get_parent_node(tree, IDLN_INTERFACE, NULL)).ident),
				 "_", 0);
    for(curitem = IDL_ATTR_DCL(tree).simple_declarations;
	curitem; curitem = IDL_LIST(curitem).data) {
      fprintf(of, "  skel_%s__get_%s,\n", id,
	      IDL_IDENT(IDL_LIST(curitem).data).str);
      if(!IDL_ATTR_DCL(tree).f_readonly)
	fprintf(of, "  skel_%s__set_%s,\n", id,
		IDL_IDENT(IDL_LIST(curitem).data).str);
    }
    break;
  default:
    break;
  }
  g_free(id);
}
#endif

static gint
cbe_skel_compare_op_dcls(CBESkelOpInfo *op1, CBESkelOpInfo *op2)
{
  return strcmp(op1->opname, op2->opname);
}

static void
cbe_skel_free_op_info(CBESkelOpInfo *op)
{
  g_free(op->opname);
  g_free(op->iface_id);
  g_free(op);
}

static void
cbe_skel_interface_add_relayer(IDL_tree intf, CBESkelInterfaceTraverseInfo *iti)
{
  CBESkelOpInfo *newopi;
  IDL_tree curitem, curdcl, curattr, curattrdcl;
  char *iface_id;
  int   idx = 0;

  iface_id =
    IDL_ns_ident_to_qstring(IDL_IDENT_TO_NS(IDL_INTERFACE(intf).ident),
			    "_", 0);

  for(curitem = IDL_INTERFACE(intf).body; curitem;
      curitem = IDL_LIST(curitem).next) {
    curdcl = IDL_LIST(curitem).data;

    switch(IDL_NODE_TYPE(curdcl)) {
    case IDLN_OP_DCL:
      newopi = g_new0(CBESkelOpInfo, 1);
      newopi->iface_id = g_strdup(iface_id);
      newopi->opname = g_strdup(IDL_IDENT(IDL_OP_DCL(curdcl).ident).str);
      newopi->idx = idx++;
      iti->oplist = g_slist_insert_sorted(iti->oplist, newopi,
					  (GCompareFunc)cbe_skel_compare_op_dcls);
      break;
    case IDLN_ATTR_DCL:
      for(curattr = IDL_ATTR_DCL(curdcl).simple_declarations;
	  curattr; curattr = IDL_LIST(curattr).next) {
	curattrdcl = IDL_LIST(curattr).data;

	newopi = g_new0(CBESkelOpInfo, 1);
	newopi->iface_id = g_strdup(iface_id);
	newopi->opname = g_strdup_printf("_get_%s", IDL_IDENT(curattrdcl).str);
	newopi->idx = idx++;
	iti->oplist = g_slist_insert_sorted(iti->oplist, newopi,
					    (GCompareFunc)cbe_skel_compare_op_dcls);
	if(!IDL_ATTR_DCL(curdcl).f_readonly) {
	  newopi = g_new0(CBESkelOpInfo, 1);
	  newopi->iface_id = g_strdup(iface_id);
	  newopi->opname = g_strdup_printf("_set_%s", IDL_IDENT(curattrdcl).str);
	  newopi->idx = idx++;
	  iti->oplist = g_slist_insert_sorted(iti->oplist, newopi,
					      (GCompareFunc)cbe_skel_compare_op_dcls);
	}
      }
      break;
    default:
      break;
    }
  }

  g_free(iface_id);
}

static void
cbe_skel_interface_print_relayers(const CBESkelInterfaceTraverseInfo *iti)
{
  CBESkelInterfaceTraverseInfo subiti = *iti;
  GSList *curnode;
  CBESkelOpInfo *opi;
  char curchar;

  curnode = iti->oplist;
  subiti.curlevel = iti->curlevel+1;
  fprintf(iti->ci->fh, "switch(opname[%d]) {\n", iti->curlevel);
  while(curnode) {
    opi = (CBESkelOpInfo *)curnode->data;
    if(iti->curlevel > strlen(opi->opname)) {
      curnode = g_slist_next(curnode);
      continue;
    }
    curchar = opi->opname[iti->curlevel];
    if(curchar)
      fprintf(iti->ci->fh, "case '%c':\n", curchar);
    else
      fprintf(iti->ci->fh, "case '\\0':\n");
    subiti.oplist = NULL;
    while(curnode && ((CBESkelOpInfo *)curnode->data)->opname[iti->curlevel]
	  == curchar) {
      subiti.oplist = g_slist_append(subiti.oplist, curnode->data);
      curnode = g_slist_next(curnode);
    }

    if(g_slist_length(subiti.oplist) > 1) {
      if(curchar)
	cbe_skel_interface_print_relayers(&subiti);
      else
	g_error("two ops with same name!!!!");
    } else {
      if(strlen(opi->opname + iti->curlevel))
	fprintf(iti->ci->fh, "if(strcmp((opname + %d), \"%s\")) break;\n",
		iti->curlevel + 1, opi->opname + iti->curlevel+1);
      fprintf(iti->ci->fh, "*impl = (gpointer)servant->vepv->%s_epv->%s;\n",
	      opi->iface_id, opi->opname);
      if (iti->small) {
        fprintf(iti->ci->fh, "*m_data = (gpointer)&%s__iinterface.methods._buffer [%d];\n",
		opi->iface_id, opi->idx);
        fprintf(iti->ci->fh, "return (ORBitSmallSkeleton)_ORBIT_skel_small_%s_%s;\n",
	        opi->iface_id, opi->opname);
      } else
        fprintf(iti->ci->fh, "return (ORBitSkeleton)_ORBIT_skel_%s_%s;\n",
	        opi->iface_id, opi->opname);
    }
    fprintf(iti->ci->fh, "break;\n");
    g_slist_free(subiti.oplist);
  }
  fprintf(iti->ci->fh, "default: break; \n}\n");
}

static void
cbe_skel_interface_print_relayer(IDL_tree tree, OIDL_Run_Info *rinfo, OIDL_C_Info *ci)
{
  char *id;
  CBESkelInterfaceTraverseInfo iti;

  id = IDL_ns_ident_to_qstring(IDL_IDENT_TO_NS(IDL_INTERFACE(tree).ident), "_", 0);
  if (rinfo->small_skels) {
    fprintf(ci->fh, "static ORBitSmallSkeleton get_skel_small_%s(POA_%s *servant,\nconst char *opname,"
	    "gpointer *m_data, gpointer *impl)\n{\n", id, id);
  } else {
    fprintf(ci->fh, "static ORBitSkeleton get_skel_%s(POA_%s *servant,\nGIOPRecvBuffer *_ORBIT_recv_buffer,\ngpointer *impl)\n{\n", id, id);
    fprintf(ci->fh, "gchar *opname = giop_recv_buffer_get_opname(_ORBIT_recv_buffer);\n\n");
  }

  iti.ci = ci;
  iti.oplist = NULL;
  iti.curlevel = 0;
  iti.small = rinfo->small_skels;

  IDL_tree_traverse_parents(tree,
			    (GFunc)cbe_skel_interface_add_relayer, &iti);

  cbe_skel_interface_print_relayers(&iti);

  g_slist_foreach(iti.oplist, (GFunc)cbe_skel_free_op_info, NULL);
  g_slist_free(iti.oplist);

  fprintf(ci->fh, "return NULL;\n");
  fprintf(ci->fh, "}\n\n");

  g_free(id);
}

#if 0
static void
cbe_skel_interface_print_initializer(IDL_tree node,
				     OIDL_C_Info *ci)
{
  char *id;

  g_assert(IDL_NODE_TYPE(node) == IDLN_INTERFACE);

  /* Print the operations defined for this interface, but in current's
     namespace */

  id = IDL_ns_ident_to_qstring(IDL_IDENT_TO_NS(IDL_INTERFACE(node).ident),
			       "_", 0);

  fprintf(ci->fh, "obj->vepv[%s__classid] = servant->vepv->%s_epv;\n", id, id);

  g_free(id);
}
#endif

static void
cbe_skel_interface_print_vepvmap_line(IDL_tree node, OIDL_C_Info *ci)
{
  char *id;
  id = IDL_ns_ident_to_qstring(IDL_IDENT_TO_NS(IDL_INTERFACE(node).ident),
			       "_", 0);
  fprintf(ci->fh, "class_info.vepvmap[%s__classid]"
    " = (((char*)&(fakevepv->%s_epv)) - ((char*)(fakevepv)))/sizeof(GFunc);\n",
    id, id);
  g_free(id);
}

static void
cbe_skel_do_interface(IDL_tree tree, OIDL_Run_Info *rinfo, OIDL_C_Info *ci)
{
  char *id, *id2;
  IDL_tree curitem, pnt;
  int i;

  /* PIDL methods dont have normal skel functions. */
  for ( pnt=tree; pnt; pnt=IDL_NODE_UP(pnt) ) {
      if ( pnt->declspec & IDLF_DECLSPEC_PIDL )
	return;
  }

  id = IDL_ns_ident_to_qstring(IDL_IDENT_TO_NS(IDL_INTERFACE(tree).ident), "_", 0);

  cbe_skel_interface_print_relayer(tree, rinfo, ci);

  fprintf(ci->fh,
	  "void POA_%s__init(PortableServer_Servant servant,\nCORBA_Environment *env)\n",
	  id);
  fprintf(ci->fh, "{\n");
  fprintf(ci->fh,"  static PortableServer_ClassInfo class_info = {");

  if (rinfo->small_skels)
	  fprintf(ci->fh, "NULL, (ORBit_small_impl_finder)&get_skel_small_%s, ", id);
  else
	  fprintf(ci->fh, "(ORBit_impl_finder)&get_skel_%s, NULL, ", id);

  fprintf(ci->fh,"\"%s\", &%s__classid, NULL, &%s__iinterface};\n",
	  IDL_IDENT(IDL_INTERFACE(tree).ident).repo_id, id, id);
  fprintf(ci->fh, "   POA_%s__vepv *fakevepv = NULL;", id);

  {
  	const char *finref = 
      		"((PortableServer_ServantBase*)servant)->vepv[0]->finalize";
  	fprintf(ci->fh, "if ( %s == 0 ) { ", finref);
      	fprintf(ci->fh, "%s = POA_%s__fini;\n", finref, id);
  	fprintf(ci->fh, "}\n");
  }
  fprintf(ci->fh,
	  "  PortableServer_ServantBase__init(((PortableServer_ServantBase *)servant), env);\n");

  for(curitem = IDL_INTERFACE(tree).inheritance_spec; curitem;
      curitem = IDL_LIST(curitem).next) {
    id2 = IDL_ns_ident_to_qstring(IDL_IDENT_TO_NS(IDL_LIST(curitem).data),
				  "_", 0);
    fprintf(ci->fh, "  POA_%s__init(servant, env);\n", id2);
    g_free(id2);
  }
  /* registering after other __inits() makes the classids increment nicely. */
  fprintf(ci->fh, "  ORBit_classinfo_register(&class_info);\n");
  /* Set classinfo after other __inits() for most derived interface. */
  fprintf(ci->fh, "  ORBIT_SERVANT_SET_CLASSINFO(servant,&class_info);\n");
  fprintf(ci->fh, "\nif (!class_info.vepvmap) {\n");
  fprintf(ci->fh, "   class_info.vepvmap = g_new0 (ORBit_VepvIdx, %s__classid + 1);\n", id);
  IDL_tree_traverse_parents(tree, (GFunc) cbe_skel_interface_print_vepvmap_line, ci);
  fprintf(ci->fh, "}\n");

  fprintf(ci->fh, "}\n\n");

  fprintf(ci->fh,
	  "void POA_%s__fini(PortableServer_Servant servant,\nCORBA_Environment *env)\n",
	  id);
  fprintf(ci->fh, "{\n");
  if(IDL_INTERFACE(tree).inheritance_spec)
    {
      for(i = IDL_list_length(IDL_INTERFACE(tree).inheritance_spec) - 1;
	  i >= 0; i--) {
	curitem = IDL_list_nth(IDL_INTERFACE(tree).inheritance_spec, i);
	id2 = IDL_ns_ident_to_qstring(IDL_IDENT_TO_NS(IDL_LIST(curitem).data),
				      "_", 0);
	/* XXX fixme - this is going to call ServantBase__fini multiple times */
	fprintf(ci->fh, "  POA_%s__fini(servant, env);\n",
		id2);
	g_free(id2);
      }
    }
  fprintf(ci->fh, "  PortableServer_ServantBase__fini(servant, env);\n");
  fprintf(ci->fh, "}\n\n");

  g_free(id);
}
