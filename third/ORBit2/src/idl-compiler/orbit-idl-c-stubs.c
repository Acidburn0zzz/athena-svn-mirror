#include "config.h"

#include "orbit-idl-c-backend.h"

#include <string.h>

static void cs_output_stubs(IDL_tree tree, OIDL_C_Info *ci);
static void cs_small_output_stubs(IDL_tree tree, OIDL_C_Info *ci, int *idx);

void
orbit_idl_output_c_stubs(OIDL_Output_Tree *tree, OIDL_Run_Info *rinfo, OIDL_C_Info *ci)
{
  fprintf (ci->fh, OIDL_C_WARNING);
  fprintf(ci->fh, "#include <string.h>\n");
  fprintf(ci->fh, "#define ORBIT2_STUBS_API\n");
  fprintf(ci->fh, "#include \"%s.h\"\n\n", ci->base_name);

  if (rinfo->small_stubs)
    cs_small_output_stubs(tree->tree, ci, NULL);
  else
    cs_output_stubs(tree->tree, ci);
}

static void cs_small_output_stub(IDL_tree tree, OIDL_C_Info *ci, int *idx);

/* Very similar to cs_output_stubs */
static void
cs_small_output_stubs(IDL_tree tree, OIDL_C_Info *ci, int *idx)
{
  if(!tree) return;

  switch(IDL_NODE_TYPE(tree)) {
  case IDLN_MODULE:
    cs_small_output_stubs(IDL_MODULE(tree).definition_list, ci, idx);
    break;
  case IDLN_LIST:
    {
      IDL_tree sub;
      for(sub = tree; sub; sub = IDL_LIST(sub).next) {
	cs_small_output_stubs(IDL_LIST(sub).data, ci, idx);
      }
    }
    break;
  case IDLN_ATTR_DCL:
    {
      OIDL_Attr_Info *ai = tree->data;

      IDL_tree curitem;
      
      for(curitem = IDL_ATTR_DCL(tree).simple_declarations; curitem; curitem = IDL_LIST(curitem).next) {
	ai = IDL_LIST(curitem).data->data;
	
	cs_small_output_stubs(ai->op1, ci, idx);
	if(ai->op2)
	  cs_small_output_stubs(ai->op2, ci, idx);
      }
    }
    break;
  case IDLN_INTERFACE: {
    int real_idx = 0;
    cs_small_output_stubs(IDL_INTERFACE(tree).body, ci, &real_idx);
    break;
  }
  case IDLN_OP_DCL:
    cs_small_output_stub(tree, ci, idx);
    break;
  default:
    break;
  }
}

static void
cs_small_output_stub(IDL_tree tree, OIDL_C_Info *ci, int *idx)
{
	FILE         *of = ci->fh;
	char         *id;
	gboolean      has_retval, has_args;

	g_return_if_fail (idx != NULL);

	id = orbit_cbe_op_get_interface_name (tree);
	has_retval = IDL_OP_DCL(tree).op_type_spec != NULL;
	has_args   = IDL_OP_DCL(tree).parameter_dcls != NULL;

	orbit_cbe_op_write_proto (of, tree, "", FALSE);

	fprintf (of, "{\n");

	if (has_retval) {
		orbit_cbe_write_param_typespec (of, tree);
		fprintf (of, " " ORBIT_RETVAL_VAR_NAME ";\n");
	}
	fprintf (ci->fh, "POA_%s__epv *%s;\n", id, ORBIT_EPV_VAR_NAME);

	{
		IDL_tree curitem;
		char *id;

		curitem = IDL_get_parent_node (tree, IDLN_INTERFACE, 0);
		g_assert (curitem);
		id = IDL_ns_ident_to_qstring (IDL_IDENT_TO_NS (IDL_INTERFACE (curitem).ident),
					      "_", 0);

		fprintf (ci->fh, "if (ORBit_small_flags & ORBIT_SMALL_FAST_LOCALS && \n");
		fprintf (ci->fh, "    ORBIT_STUB_IsBypass (_obj, %s__classid) && \n", id);
		fprintf (ci->fh, "    (%s = (POA_%s__epv *) ORBIT_STUB_GetEpv (_obj, %s__classid))->%s) {\n",
			 ORBIT_EPV_VAR_NAME, id, id, IDL_IDENT (IDL_OP_DCL (tree).ident).str);

		fprintf (ci->fh, "ORBIT_STUB_PreCall (_obj);\n");

		fprintf (ci->fh, "%s%s->%s (ORBIT_STUB_GetServant (_obj), ",
			 IDL_OP_DCL (tree).op_type_spec? ORBIT_RETVAL_VAR_NAME " = ":"",
			 ORBIT_EPV_VAR_NAME,
			 IDL_IDENT (IDL_OP_DCL (tree).ident).str);
		g_free(id);

		for(curitem = IDL_OP_DCL(tree).parameter_dcls; curitem;
		    curitem = IDL_LIST(curitem).next) {
			fprintf(ci->fh, "%s, ",
				IDL_IDENT(IDL_PARAM_DCL(IDL_LIST(curitem).data).simple_declarator).str);
		}
		if(IDL_OP_DCL(tree).context_expr)
			fprintf(ci->fh, "_ctx, ");

		fprintf(ci->fh, "ev);\n");

		fprintf(ci->fh, "ORBIT_STUB_PostCall (_obj);\n");
	}

	fprintf (of, " } else { /* remote marshal */\n");

	{
		if (has_args)
			cbe_small_flatten_args (tree, of, "_args");

		fprintf (of, "ORBit_small_invoke_stub_n (_obj, "
			 "&%s__iinterface.methods, %d, ", id, *idx);

		if (has_retval)
			fprintf (of, "&_ORBIT_retval, ");
		else
			fprintf (of, "NULL, ");

		if (has_args)
			fprintf (of, "_args, ");
		else
			fprintf (of, "NULL, ");

		if (IDL_OP_DCL(tree).context_expr)
			fprintf(ci->fh, "_ctx, ");
		else
			fprintf(ci->fh, "NULL, ");
		
		fprintf (of, "ev);\n\n");

	}

	fprintf (of, "}\n");

	if (has_retval)
		fprintf (of, "return " ORBIT_RETVAL_VAR_NAME ";\n");

	fprintf (of, "}\n");

	(*idx)++;
}

static void cs_output_stub(IDL_tree tree, OIDL_C_Info *ci);
static void cs_output_except(IDL_tree tree, OIDL_C_Info *ci);

/* Very similar to cs_output_small_stubs */
static void
cs_output_stubs(IDL_tree tree, OIDL_C_Info *ci)
{
  if(!tree) return;

  switch(IDL_NODE_TYPE(tree)) {
  case IDLN_MODULE:
    cs_output_stubs(IDL_MODULE(tree).definition_list, ci);
    break;
  case IDLN_LIST:
    {
      IDL_tree sub;
      for(sub = tree; sub; sub = IDL_LIST(sub).next) {
	cs_output_stubs(IDL_LIST(sub).data, ci);
      }
    }
    break;
  case IDLN_ATTR_DCL:
    {
      OIDL_Attr_Info *ai = tree->data;

      IDL_tree curitem;
      
      for(curitem = IDL_ATTR_DCL(tree).simple_declarations; curitem; curitem = IDL_LIST(curitem).next) {
	ai = IDL_LIST(curitem).data->data;
	
	cs_output_stubs(ai->op1, ci);
	if(ai->op2)
	  cs_output_stubs(ai->op2, ci);
      }
    }
    break;
  case IDLN_INTERFACE:
    cs_output_stubs(IDL_INTERFACE(tree).body, ci);
    break;
  case IDLN_OP_DCL:
    cs_output_stub(tree, ci);
    break;
  case IDLN_EXCEPT_DCL:
    cs_output_except(tree, ci);
    break;
  default:
    break;
  }
}

/* Here's the fun part ;-) */
static void cs_stub_alloc_params(IDL_tree tree, OIDL_C_Info *ci);
static void cs_free_inout_params(IDL_tree tree, OIDL_C_Info *ci);
static void cs_stub_print_return(IDL_tree tree, OIDL_C_Info *ci);

static void
cs_output_stub(IDL_tree tree, OIDL_C_Info *ci)
{
  OIDL_Op_Info *oi;

  oi = tree->data;
  g_assert(oi);

  if ( oidl_tree_is_pidl(tree) )
  	return;

  orbit_cbe_op_write_proto(ci->fh, tree, "", FALSE);
  fprintf(ci->fh, "{\n");
  if(IDL_OP_DCL(tree).raises_expr) {
    IDL_tree curitem;
    fprintf(ci->fh, "const ORBit_exception_demarshal_info _ORBIT_user_exceptions[] = { ");
    for(curitem = IDL_OP_DCL(tree).raises_expr; curitem;
	curitem = IDL_LIST(curitem).next) {
      char *id;
      IDL_tree curnode = IDL_LIST(curitem).data;

      id = orbit_cbe_get_typespec_str(curnode);
      fprintf(ci->fh, "{(const CORBA_TypeCode)&TC_%s_struct, (gpointer)_ORBIT_%s_demarshal},",
	      id, id);
      g_free(id);
    }
    fprintf(ci->fh, "{CORBA_OBJECT_NIL, NULL}};\n");
  }

  if(IDL_OP_DCL(tree).context_expr) {
    IDL_tree curitem;
    fprintf(ci->fh, "const ORBit_ContextMarshalItem _context_items[] = {\n");

    for(curitem = IDL_OP_DCL(tree).context_expr; curitem; curitem = IDL_LIST(curitem).next) {
      fprintf(ci->fh, "{%lu, \"%s\"},\n", (unsigned long)strlen(IDL_STRING(IDL_LIST(curitem).data).value) + 1,
	      IDL_STRING(IDL_LIST(curitem).data).value);
    }
    fprintf(ci->fh, "};\n");
  }

  fprintf(ci->fh, "register CORBA_unsigned_long _ORBIT_request_id;\n");
  fprintf(ci->fh, "register CORBA_char *_ORBIT_system_exception_ex = ex_CORBA_COMM_FAILURE;\n");
  fprintf(ci->fh, "register CORBA_completion_status _ORBIT_completion_status = CORBA_COMPLETED_NO;\n");
  fprintf(ci->fh, "register GIOPSendBuffer *_ORBIT_send_buffer = NULL;\n");
  if(!IDL_OP_DCL(tree).f_oneway)
    {
      fprintf(ci->fh, "register GIOPRecvBuffer *_ORBIT_recv_buffer = NULL;\n");
      fprintf(ci->fh, "GIOPMessageQueueEntry _ORBIT_mqe;\n");
    }
  fprintf(ci->fh, "register GIOPConnection *_cnx;\n");
  fprintf(ci->fh, "register guchar *_ORBIT_buf_end;\n");

  if(oi->out_stubs) /* Lame hack to ensure we get _ORBIT_retval available */
    orbit_cbe_alloc_tmpvars(oi->out_stubs, ci);

  /* Check if we can do a direct call, and if so, do it */
  {
    IDL_tree curitem;
    char *id;

    curitem = IDL_get_parent_node(tree, IDLN_INTERFACE, 0);
    g_assert(curitem);
    id = IDL_ns_ident_to_qstring(IDL_IDENT_TO_NS(IDL_INTERFACE(curitem).ident),
				 "_", 0);
    fprintf(ci->fh, "if(ORBIT_STUB_IsBypass(_obj,%s__classid))\n{\n", id);
#if 0
    fprintf(ci->fh, "ORBit_POAInvocation invoke_rec;\n");
#endif

    fprintf(ci->fh, "POA_%s__epv *_epv=(POA_%s__epv*)ORBIT_STUB_GetEpv(_obj,%s__classid);\n",
	    id, id, id);
    fprintf(ci->fh, "ORBit_POAInvocation _invoke_rec;\n");
    fprintf(ci->fh, "ORBIT_STUB_PreCall(_obj, _invoke_rec);\n");
    fprintf(ci->fh, "%s _epv->%s(ORBIT_STUB_GetServant(_obj), ",
	    IDL_OP_DCL(tree).op_type_spec?"_ORBIT_retval = ":"",
	    IDL_IDENT(IDL_OP_DCL(tree).ident).str);
    g_free(id);
    for(curitem = IDL_OP_DCL(tree).parameter_dcls; curitem;
	curitem = IDL_LIST(curitem).next) {
      fprintf(ci->fh, "%s, ",
	      IDL_IDENT(IDL_PARAM_DCL(IDL_LIST(curitem).data).simple_declarator).str);
    }
    if(IDL_OP_DCL(tree).context_expr)
      fprintf(ci->fh, "_ctx, ");

    fprintf(ci->fh, "ev);\n");

    fprintf(ci->fh, "ORBIT_STUB_PostCall(_obj, _invoke_rec);\n");
    fprintf(ci->fh, "return %s;\n}\n", IDL_OP_DCL(tree).op_type_spec?"_ORBIT_retval":"");
  }

  fprintf(ci->fh, "_cnx = ORBit_object_get_connection(_obj);\n");
  fprintf(ci->fh, "if(!_cnx) goto _ORBIT_system_exception;\n");

  if(!IDL_OP_DCL(tree).f_oneway) /* For location forwarding */
    {
      fprintf(ci->fh, "_ORBIT_retry_request:\n");
      fprintf(ci->fh, "_ORBIT_recv_buffer = NULL;\n");
    }

  fprintf(ci->fh, "_ORBIT_send_buffer = NULL;\n");
  fprintf(ci->fh, "_ORBIT_completion_status = CORBA_COMPLETED_NO;\n");

  if(sizeof(gpointer) > sizeof(CORBA_unsigned_long))
    fprintf(ci->fh, "_ORBIT_request_id = giop_get_request_id();\n");
  else
    fprintf(ci->fh, "_ORBIT_request_id = GPOINTER_TO_UINT(g_alloca(0));\n");

  fprintf(ci->fh, "{ /* marshalling */\n");
  fprintf(ci->fh, "static const struct { CORBA_unsigned_long len; char opname[%lu]; } _ORBIT_operation_name_data = { %lu, \"%s\" };\n",
	  /* We align it now instead of at runtime */
	  ALIGN_VALUE(strlen(IDL_IDENT(IDL_OP_DCL(tree).ident).str) + 1, sizeof(CORBA_unsigned_long)),
	  (unsigned long)strlen(IDL_IDENT(IDL_OP_DCL(tree).ident).str) + 1,
	  IDL_IDENT(IDL_OP_DCL(tree).ident).str);
  fprintf(ci->fh, "const struct iovec _ORBIT_operation_vec = {(gpointer)&_ORBIT_operation_name_data, %lu};\n",
	  sizeof(CORBA_unsigned_long) +
	  ALIGN_VALUE(strlen(IDL_IDENT(IDL_OP_DCL(tree).ident).str) + 1,
		      sizeof(CORBA_unsigned_long)));

  orbit_cbe_alloc_tmpvars(oi->in_stubs, ci);

  fprintf(ci->fh, "_ORBIT_send_buffer = \n");
  fprintf(ci->fh, "giop_send_buffer_use_request(_cnx->giop_version, _ORBIT_request_id, %s,\n",
	  IDL_OP_DCL(tree).f_oneway?"CORBA_FALSE":"CORBA_TRUE");
  fprintf(ci->fh, "&(_obj->oki->object_key_vec), &_ORBIT_operation_vec, NULL /* &ORBit_default_principal_iovec */);\n\n");
  fprintf(ci->fh, "_ORBIT_system_exception_ex = ex_CORBA_COMM_FAILURE;\n");
  fprintf(ci->fh, "if(!_ORBIT_send_buffer) goto _ORBIT_system_exception;\n");

  c_marshalling_generate(oi->in_stubs, ci, TRUE);

  if(!IDL_OP_DCL(tree).f_oneway)
    {
      fprintf(ci->fh, "giop_recv_list_setup_queue_entry(&_ORBIT_mqe, _cnx, GIOP_REPLY, _ORBIT_request_id);\n");
      fprintf(ci->fh, "if(giop_send_buffer_write(_ORBIT_send_buffer, _cnx)) { giop_recv_list_destroy_queue_entry(&_ORBIT_mqe); goto _ORBIT_system_exception; }\n");
    }
  else
    fprintf(ci->fh, "if(giop_send_buffer_write(_ORBIT_send_buffer, _cnx)) goto _ORBIT_system_exception;\n");
  fprintf(ci->fh, "_ORBIT_completion_status = CORBA_COMPLETED_MAYBE;\n");
  fprintf(ci->fh, "giop_send_buffer_unuse(_ORBIT_send_buffer); _ORBIT_send_buffer = NULL;\n");
  fprintf(ci->fh, "}\n");

  if(!IDL_OP_DCL(tree).f_oneway) {
    /* free inout params as needed */
    cs_free_inout_params(tree, ci);

    fprintf(ci->fh, "{ /* demarshalling */\n");

    if(oi->out_stubs)
      fprintf(ci->fh, "register guchar *_ORBIT_curptr G_GNUC_UNUSED;\n");

    fprintf(ci->fh, "_ORBIT_recv_buffer = giop_recv_buffer_get(&_ORBIT_mqe, TRUE);\n");

    fprintf(ci->fh, "if(!_ORBIT_recv_buffer) goto _ORBIT_system_exception;\n");
    fprintf(ci->fh, "_ORBIT_buf_end = _ORBIT_recv_buffer->end;\n");
    fprintf(ci->fh, "_ORBIT_completion_status = CORBA_COMPLETED_YES;\n");

    fprintf(ci->fh, "if(giop_recv_buffer_reply_status(_ORBIT_recv_buffer) != GIOP_NO_EXCEPTION) goto _ORBIT_msg_exception;\n");

    cs_stub_alloc_params(tree, ci);

    c_demarshalling_generate(oi->out_stubs, ci, FALSE, FALSE);

    fprintf(ci->fh, "giop_recv_buffer_unuse(_ORBIT_recv_buffer);\n");
    cs_stub_print_return (tree, ci);

  } else
    fprintf(ci->fh, "return;\n");

  if(!IDL_OP_DCL(tree).f_oneway) {
    IDL_tree curitem;

    for(curitem = IDL_list_nth(IDL_OP_DCL(tree).parameter_dcls, IDL_list_length(IDL_OP_DCL(tree).parameter_dcls) - 1);
	curitem; curitem = IDL_LIST(curitem).prev)
      {
	IDL_tree param;

	param = IDL_LIST(curitem).data;
	if(IDL_PARAM_DCL(param).attr != IDL_PARAM_IN)
	  {
	    fprintf(ci->fh, "%s_demarshal_error:\n", IDL_IDENT(IDL_PARAM_DCL(param).simple_declarator).str);
	    cbe_op_param_free(param, ci, FALSE);
	  }
      }    

    if(IDL_OP_DCL(tree).op_type_spec)
      {
	fprintf(ci->fh, "%s_demarshal_error:\n", ORBIT_RETVAL_VAR_NAME);
	cbe_op_retval_free(IDL_OP_DCL(tree).op_type_spec, ci);
      }

    fprintf(ci->fh, "_ORBIT_demarshal_error:\n");
    fprintf(ci->fh, "_ORBIT_system_exception_ex = ex_CORBA_MARSHAL;\n");
  }
  fprintf(ci->fh, "_ORBIT_system_exception:\n");
#ifdef BACKWARDS_COMPAT_0_4
  fprintf(ci->fh, "CORBA_exception_set_system(ev, _ORBIT_system_exception_ex, _ORBIT_completion_status);\n");
  fprintf(ci->fh, "giop_recv_buffer_unuse(_ORBIT_recv_buffer);\n");
  fprintf(ci->fh, "giop_send_buffer_unuse(_ORBIT_send_buffer);\n");
#else
  fprintf(ci->fh, "ORBit_handle_system_exception(ev, _ORBIT_system_exception_ex, _ORBIT_completion_status, %s, _ORBIT_send_buffer);\n",
	  IDL_OP_DCL(tree).f_oneway?"NULL":"_ORBIT_recv_buffer");
#endif
  if(IDL_OP_DCL(tree).op_type_spec)
    /* This will avoid warning about uninitialized memory while
     * compiling the stubs with no extra code size cost */
    fprintf(ci->fh, "#if defined(__GNUC__) && defined(__OPTIMIZE__)\n"
	    "if(&%s);\n"
	    "#endif\n", ORBIT_RETVAL_VAR_NAME);

  cs_stub_print_return (tree, ci);

  if(!IDL_OP_DCL(tree).f_oneway) {
    fprintf(ci->fh, "_ORBIT_msg_exception:\n");
    /* deal with LOCATION_FORWARD exceptions */
    fprintf(ci->fh, "if(giop_recv_buffer_reply_status(_ORBIT_recv_buffer) == GIOP_LOCATION_FORWARD) {\n");
#ifdef BACKWARDS_COMPAT_0_4
    fprintf(ci->fh, "if (_obj->forward_locations != NULL) ORBit_delete_profiles(_obj->forward_locations);\n");
    fprintf(ci->fh, "_obj->forward_locations = ORBit_demarshal_IOR(_ORBIT_recv_buffer);\n");
    fprintf(ci->fh, "_cnx = ORBit_object_get_forwarded_connection(_obj);\n");
    fprintf(ci->fh, "giop_recv_buffer_unuse(_ORBIT_recv_buffer);\n");
#else
    fprintf(ci->fh, "_cnx = ORBit_handle_location_forward(_ORBIT_recv_buffer, _obj);\n");
#endif
    fprintf(ci->fh, "\ngoto _ORBIT_retry_request;\n");
    fprintf(ci->fh, "} else {\n");

    fprintf(ci->fh, "ORBit_handle_exception(_ORBIT_recv_buffer, ev, %s, _obj->orb);\n",
	    IDL_OP_DCL(tree).raises_expr?"_ORBIT_user_exceptions":"NULL");
    fprintf(ci->fh, "giop_recv_buffer_unuse(_ORBIT_recv_buffer);\n");
    cs_stub_print_return (tree, ci);
    fprintf(ci->fh, "}\n}\n");
  }

  fprintf(ci->fh, "}\n");
}
    
static void cs_stub_print_return(IDL_tree tree, OIDL_C_Info *ci)
{
  if(IDL_OP_DCL(tree).op_type_spec)
    fprintf(ci->fh, "return %s;\n", ORBIT_RETVAL_VAR_NAME);
  else
    fprintf(ci->fh, "return;\n");
}

static void
cbe_stub_op_param_alloc(FILE *of, IDL_tree node, GString *tmpstr)
{
  int n, i;
  char *id;
  IDL_tree ts = orbit_cbe_get_typespec(node);
  gboolean isSlice;

  n = oidl_param_info(node, 
			 oidl_attr_to_paramrole(IDL_PARAM_DCL(node).attr),
			 &isSlice);
  if ( isSlice )
  	n -= 1;

  if(((n - 1) <= 0) && IDL_NODE_TYPE(ts) != IDLN_TYPE_ARRAY)
    return;

  switch(IDL_NODE_TYPE(ts)) {
  case IDLN_TYPE_ARRAY:
    if((IDL_PARAM_DCL(node).attr != IDL_PARAM_OUT)
       || orbit_cbe_type_is_fixed_length(ts))
      return;
    n++;
    break;
  case IDLN_TYPE_SEQUENCE:
  case IDLN_TYPE_ANY:
    if(IDL_PARAM_DCL(node).attr == IDL_PARAM_INOUT)
      return;
    break;
  case IDLN_TYPE_UNION:
  case IDLN_TYPE_STRUCT:
    if(orbit_cbe_type_is_fixed_length(ts))
      return;
  default:
    break;
  }

  g_string_assign(tmpstr, "");
  for(i = 0; i < n - 1; i++)
    g_string_append_c(tmpstr, '*');

  g_string_append_printf(tmpstr, "%s",
		    IDL_IDENT(IDL_PARAM_DCL(node).simple_declarator).str);

  id = orbit_cbe_get_typespec_str(IDL_PARAM_DCL(node).param_type_spec);
  fprintf(of, "%s = %s__alloc();\n", tmpstr->str, id);
  g_free(id);
}

void
cbe_stub_op_retval_alloc(FILE *of, IDL_tree node, GString *tmpstr)
{
  int n;
  char *id;
  IDL_tree ts = orbit_cbe_get_typespec(node);

  n = oidl_param_numptrs(node, DATA_RETURN);

  if((n <= 0)
     && (IDL_NODE_TYPE(ts) != IDLN_TYPE_ARRAY))
    return;

  g_string_assign(tmpstr, ORBIT_RETVAL_VAR_NAME );

  switch(IDL_NODE_TYPE(ts)) {
  case IDLN_TYPE_UNION:
  case IDLN_TYPE_STRUCT:
    if(orbit_cbe_type_is_fixed_length(ts))
      return;
  case IDLN_TYPE_ANY:
  case IDLN_TYPE_SEQUENCE:
  case IDLN_TYPE_ARRAY:
    break;
  default:
    return;
  }

  id = orbit_cbe_get_typespec_str(node);
  fprintf(of, "%s = %s__alloc();\n", tmpstr->str, id);
  g_free(id);
}

static void
cs_stub_alloc_params(IDL_tree tree, OIDL_C_Info *ci)
{
  IDL_tree curitem, param;
  GString *tmpstr = g_string_new(NULL);

  if(IDL_OP_DCL(tree).op_type_spec)
    cbe_stub_op_retval_alloc(ci->fh, IDL_OP_DCL(tree).op_type_spec,
			     tmpstr);

  for(curitem = IDL_OP_DCL(tree).parameter_dcls; curitem;
      curitem = IDL_LIST(curitem).next) {

    param = IDL_LIST(curitem).data;

    if(IDL_PARAM_DCL(param).attr == IDL_PARAM_INOUT
       || IDL_PARAM_DCL(param).attr == IDL_PARAM_OUT)
      cbe_stub_op_param_alloc(ci->fh, param, tmpstr);
  }

  g_string_free(tmpstr, TRUE);
}

/* This param freeing stuff really could be done better - perhaps shared code with the skels generation. */
static void
cbe_stub_op_param_free(FILE *of, IDL_tree node, GString *tmpstr)
{
  int n, i;
  IDL_tree ts;
  char *id;

  ts = orbit_cbe_get_typespec(IDL_PARAM_DCL(node).param_type_spec);
  n = oidl_param_numptrs(node, DATA_INOUT);
  g_string_assign(tmpstr, "");

  if(IDL_NODE_TYPE(ts) == IDLN_TYPE_STRUCT
     || IDL_NODE_TYPE(ts) == IDLN_TYPE_UNION)
    n--;

  for(i = 0; i < n; i++)
    g_string_append_c(tmpstr, '*');
  
  g_string_append_printf(tmpstr, "%s",
		    IDL_IDENT(IDL_PARAM_DCL(node).simple_declarator).str);
  switch(IDL_NODE_TYPE(ts)) {
  case IDLN_TYPE_ANY:
    fprintf(of, "if((%s)._release) CORBA_free((%s)._value);\n",
	    tmpstr->str, tmpstr->str);
    fprintf(of, "CORBA_Object_release((%s)._type, ev);\n", tmpstr->str);
    break;
  case IDLN_TYPE_SEQUENCE:
    fprintf(of, "if((%s)._release) CORBA_free((%s)._buffer);\n",
	    tmpstr->str, tmpstr->str);
    break;
  case IDLN_TYPE_STRUCT:
  case IDLN_TYPE_UNION:
  case IDLN_TYPE_ARRAY:
    if(orbit_cbe_type_is_fixed_length(ts))
      break;
    id = IDL_ns_ident_to_qstring(IDL_IDENT_TO_NS(IDL_TYPE_STRUCT(ts).ident), "_", 0);
    fprintf(of, "%s__freekids(%s, NULL);\n", id, tmpstr->str);
    g_free(id);
    break;
  case IDLN_TYPE_STRING:
    fprintf(of, "CORBA_free(%s);\n", tmpstr->str);
    break;
  case IDLN_INTERFACE:
  case IDLN_FORWARD_DCL:
  case IDLN_TYPE_OBJECT:
  case IDLN_TYPE_TYPECODE:
    fprintf(of, "CORBA_Object_release(%s, ev);\n", tmpstr->str);
    break;
  default:
    g_assert(orbit_cbe_type_is_fixed_length(node));
    break;
  }
}

static void
cs_free_inout_params(IDL_tree tree, OIDL_C_Info *ci)
{
  IDL_tree curitem;
  GString *tmpstr;

  tmpstr = g_string_new(NULL);
  for(curitem = IDL_OP_DCL(tree).parameter_dcls; curitem; curitem = IDL_LIST(curitem).next) {
    IDL_tree curparam;

    curparam = IDL_LIST(curitem).data;

    if(IDL_PARAM_DCL(curparam).attr != IDL_PARAM_INOUT) continue;

    cbe_stub_op_param_free(ci->fh, curparam, tmpstr);
  }
  g_string_free(tmpstr, TRUE);
}

static void
cs_output_except(IDL_tree tree, OIDL_C_Info *ci)
{
  char *id;
  OIDL_Except_Info *ei;

  if ( oidl_tree_is_pidl(tree) )
  	return;

  ei = tree->data;
  g_assert(ei);

  id = IDL_ns_ident_to_qstring(IDL_IDENT_TO_NS(IDL_EXCEPT_DCL(tree).ident), "_", 0);

  fprintf(ci->fh, "gboolean\n_ORBIT_%s_demarshal(GIOPRecvBuffer *_ORBIT_recv_buffer, CORBA_Environment *ev)\n", id);
  fprintf(ci->fh, "{\n");
  if(IDL_EXCEPT_DCL(tree).members) {
    fprintf(ci->fh, "register guchar *_ORBIT_curptr;\n");
    fprintf(ci->fh, "register guchar *_ORBIT_buf_end = _ORBIT_recv_buffer->end;\n");
    orbit_cbe_alloc_tmpvars(ei->demarshal, ci);
    fprintf(ci->fh, "%s *_ORBIT_exdata;\n", id);
    c_demarshalling_generate(ei->demarshal, ci, FALSE, FALSE);
  }

  fprintf(ci->fh, "CORBA_exception_set(ev, CORBA_USER_EXCEPTION, TC_%s_struct.repo_id, %s);\n",
	  id, IDL_EXCEPT_DCL(tree).members?"_ORBIT_exdata":"NULL");

  fprintf(ci->fh, "return FALSE;");

  fprintf(ci->fh, "_ORBIT_demarshal_error:\nreturn TRUE;\n");

  fprintf(ci->fh, "}\n");
}
