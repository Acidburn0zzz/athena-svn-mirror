
BEGIN {
  type_name = "";	# GtkEnumType
  type_macro = "";	# GTK_TYPE_ENUM_TYPE
  type_ident = "";	# _gtk_enum_type
  type_counter = 0;
  gen_macros = 0;
  gen_entries = 0;
  gen_vars = 0;
  
  for (i = 2; i < ARGC; i++)
    {
      if (ARGV[i] == "macros")
	gen_macros = 1;
      else if (ARGV[i] == "entries")
	gen_entries = 1;
      else if (ARGV[i] == "variables")
	gen_vars = 1;
      ARGV[i] = "";
    }
  
  if (gen_macros)
    printf ("/* type macros, generated by maketypes.awk */\n\n");
  else if (gen_entries)
    printf ("/* type entries, generated by maketypes.awk */\n\n");
  else if (gen_vars)
    printf ("/* type variables, generated by maketypes.awk */\n\n");
  else
    {
      printf ("hm? what do you want me to do?\n") > "/dev/stderr";
      exit 1;
    }
}

function set_type (set_type_1)
{
  type_counter += 1;
  type_name = set_type_1;
  type_macro = "GTK_TYPE";

  tmp = type_name
  gsub ("[A-Z][a-z]", "_&", tmp);
# OK, the following is ridiculous. But easier than writing a loop
  gsub ("[a-z]", "&@", tmp);
  gsub ("@[A-Z]", "@&", tmp);
  gsub ("@@", "_", tmp);
  gsub ("@", "", tmp);
  type_macro = type_macro toupper (tmp);
  type_ident = tolower (tmp);

  sub ("^GTK_TYPE_GTK_", "GTK_TYPE_", type_macro);
}

function generate (generate_1)
{
  if (gen_macros)
    {
      printf ("extern GtkType %s;\n", type_macro);
    }
  if (gen_entries)
    {
      printf ("  { \"%s\", &%s,\n", type_name, type_macro);
      if (generate_1 == "BOXED")
	printf ("    GTK_TYPE_%s, NULL },\n", generate_1);
      else
	printf ("    GTK_TYPE_%s, %s_values },\n", generate_1, type_ident);
    }
  if (gen_vars)
    {
      printf ("GtkType %s = 0;\n", type_macro);
    }
}

# skip scheme comments
";" {
  sub (";.*", "");
}

# parse keywords

/\(define-enum/ {
  if ($2 == "")
    printf ("huh? define-enum keyword without arg?\n") > "/dev/stderr";
  else
    {
      set_type($2);
      generate("ENUM");
    }
}

/\(define-flags/ {
  if ($2 == "")
    printf ("huh? define-flags keyword without arg?\n") > "/dev/stderr";
  else
    {
      set_type($2);
      generate("FLAGS");
    }
}

/\(define-boxed/ {
  if ($2 == "")
    printf ("huh? define-boxed keyword without arg?\n") > "/dev/stderr";
  else
    {
      set_type($2);
      generate("BOXED");
    }
}

END {
  if (gen_macros)
    printf("\n#define\tEEL_TYPE_NUM_BUILTINS\t(%u)\n", type_counter);
}
