#include <automaton.h>
#include <buffer_file.h>
#include <dymem.h>
#include <string.h>

/* TODO:  This is wrong on multiple levels. */
static int
atoi (const char* s)
{
  int retval = 0;

  while (*s != 0) {
    switch (*s) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      retval *= 10;
      retval += (*s - '0');
      break;
    default:
      goto out;
    }
    ++s;
  }

 out:

  return retval;
}

#define INIT_NO 1
#define START_NO 2
#define STDIN_NO 3
#define STDOUT_NO 4

/* Initialization flag. */
static bool initialized = false;

/* Started flag. */
static bool started = false;

static bd_t stdout_bd = 1;
static buffer_file_t stdout_buffer;

#define PROMPT "$ "

/*
  Begin Token Section
  ===================
*/

typedef enum {
  TOK_LPAREN,
  TOK_RPAREN,
  TOK_STRING,
  TOK_INTEGER,
  TOK_END,
} token_type_t;

typedef struct token token_t;
struct token {
  token_type_t type;
  size_t location;
  const char* string;
  int val;
  token_t* next;
};

static token_t*
token_create (token_type_t type,
	      size_t location,
	      const char* string)
{
  token_t* token = malloc (sizeof (token_t));
  memset (token, 0, sizeof (token_t));

  token->type = type;
  token->location = location;
  token->string = string;
  if (type == TOK_INTEGER) {
    token->val = atoi (string);
  }

  return token;
}

static void
token_destroy (token_t* token)
{
  free (token);
}

/*
  End Token Section
  =================
*/

/*
  Begin AST Section
  =================
*/

typedef enum {
  AST_ERROR,
  AST_PROGRAM,
  AST_EXPRESSION,
  AST_STRING,
  AST_INTEGER,
  AST_CREATE,
} ast_node_type_t;

typedef struct ast_node ast_node_t;
struct ast_node {
  ast_node_type_t type;
  ast_node_t* first_child;
  ast_node_t* next_sibling;
  union {
    token_t* token;
    const char* string;
    struct {
      const char* string;
      int val;
    } integer;
  } u;
};

static ast_node_t*
ast_node_alloc (ast_node_type_t type)
{
  ast_node_t* n = malloc (sizeof (ast_node_t));
  memset (n, 0, sizeof (ast_node_t));
  n->type = type;
  return n;
}

static void
ast_node_free (ast_node_t* n)
{
  while (n->first_child != 0) {
    ast_node_t* c = n->first_child;
    n->first_child = c->next_sibling;
    ast_node_free (c);
  }

  free (n);
}

static ast_node_t*
ast_node_append_child (ast_node_t* p,
		       ast_node_t* c)
{
  if (c != 0) {
    ast_node_t** ptr;
    for (ptr = &p->first_child; *ptr != 0; ptr = &(*ptr)->next_sibling) ;;
    
    *ptr = c;
  }

  return p;
}

static void
ast_node_append_sibling (ast_node_t* left,
			 ast_node_t* right)
{
  if (right != 0) {
    left->next_sibling = right;
  }
}

static ast_node_t*
ast_node_error (token_t* token)
{
  ast_node_t* n = ast_node_alloc (AST_ERROR);
  n->u.token = token;
  return n;
}

static ast_node_t*
ast_node_program (void)
{
  return ast_node_alloc (AST_PROGRAM);
}

static ast_node_t*
ast_node_expression (void)
{
  return ast_node_alloc (AST_EXPRESSION);
}

static ast_node_t*
ast_node_string (const char* string)
{
  ast_node_t* n = ast_node_alloc (AST_STRING);
  n->u.string = string;
  return n;
}

static ast_node_t*
ast_node_integer (const char* string,
		  int integer)
{
  ast_node_t* n = ast_node_alloc (AST_INTEGER);
  n->u.integer.string = string;
  n->u.integer.val = integer;
  return n;
}

/*
  End AST Section
  ===============
*/

/* typedef enum { */
/*   OPERAND_STRING, */
/*   OPERAND_CONSTELLATION, */
/*   OPERAND_ARGSTART, */
/* } operand_type_t; */

/* typedef struct operand operand_t; */
/* struct operand { */
/*   operand_type_t type; */
/*   union { */
/*     const char* string; */
/*     struct { */
/*       aid_t* aids; */
/*       size_t aids_size; */
/*       size_t aids_capacity; */
/*       bid_t* bids; */
/*       size_t bids_size; */
/*       size_t bids_capacity; */
/*     } constellation; */
/*   } u; */
/*   operand_t* next; */
/* }; */

/* static operand_t* */
/* operand_create (operand_type_t type) */
/* { */
/*   operand_t* operand = malloc (sizeof (operand_t)); */
/*   memset (operand, 0, sizeof (operand_t)); */
  
/*   operand->type = type; */

/*   return operand; */
/* } */

/* static void */
/* operand_destroy (operand_t* operand) */
/* { */
/*   if (operand->type == OPERAND_CONSTELLATION) { */
/*     free (operand->u.constellation.aids); */
/*     free (operand->u.constellation.bids); */
/*   } */
/*   free (operand); */
/* } */

/* static void */
/* operand_insert_aid (operand_t* operand, */
/* 		    aid_t aid) */
/* { */
/*   for (size_t idx = 0; idx != operand->u.constellation.aids_size; ++idx) { */
/*     if (operand->u.constellation.aids[idx] == aid) { */
/*       return; */
/*     } */
/*   } */

/*   if (operand->u.constellation.aids_size == operand->u.constellation.aids_capacity) { */
/*     operand->u.constellation.aids_capacity = operand->u.constellation.aids_capacity * 2 + 1; */
/*     operand->u.constellation.aids = realloc (operand->u.constellation.aids, operand->u.constellation.aids_capacity * sizeof (aid_t)); */
/*   } */
/*   operand->u.constellation.aids[operand->u.constellation.aids_size++] = aid; */
/* } */

/* static void */
/* operand_insert_bid (operand_t* operand, */
/* 		    bid_t bid) */
/* { */
/*   for (size_t idx = 0; idx != operand->u.constellation.bids_size; ++idx) { */
/*     if (operand->u.constellation.bids[idx] == bid) { */
/*       return; */
/*     } */
/*   } */

/*   if (operand->u.constellation.bids_size == operand->u.constellation.bids_capacity) { */
/*     operand->u.constellation.bids_capacity = operand->u.constellation.bids_capacity * 2 + 1; */
/*     operand->u.constellation.bids = realloc (operand->u.constellation.bids, operand->u.constellation.bids_capacity * sizeof (bid_t)); */
/*   } */
/*   operand->u.constellation.bids[operand->u.constellation.bids_size++] = bid; */
/* } */

/* static operand_t* */
/* operand_to_constellation (operand_t* operand) */
/* { */
/*   /\* TODO *\/ */
/*   switch (operand->type) { */
/*   case OPERAND_STRING: */
/*     /\* Treat it as a label. *\/ */
/*     operand->type = OPERAND_CONSTELLATION; */
/*     break; */
/*   case OPERAND_CONSTELLATION: */
/*     /\* Do nothing. *\/ */
/*     break; */
/*   case OPERAND_ARGSTART: */
/*     /\* Do nothing. *\/ */
/*     break; */
/*   } */
/*   return operand; */
/* } */

/*
  Begin Interpreter Section
  ==========================
*/

typedef enum {
  SCAN_START,
  SCAN_STRING,
  SCAN_INTEGER,
  SCAN_COMMENT,
  SCAN_ERROR,
  SCAN_DONE,
} scan_state_t;

typedef struct interpreter interpreter_t;
struct interpreter {
  char* line_original;
  char* line_str;
  size_t line_size;

  /* Scanner state. */
  scan_state_t scan_state;
  size_t scan_error_location;

  /* Tokens. */
  token_t* token_list_head;
  token_t** token_list_tail;

  /* Semantic values. */
  char* scan_string;
  size_t scan_string_size;

  /* Current token for parsing. */
  token_t* current_token;

  /* Root of the AST. */
  ast_node_t* root;

  /* operand_t* operand_stack; */
  /* size_t operand_stack_size; */

  interpreter_t* next;
};

static interpreter_t* interpreter_head = 0;
static interpreter_t** interpreter_tail = &interpreter_head;

static interpreter_t*
interpreter_create (char* line_str,
		    size_t line_size)
{
  interpreter_t* i = malloc (sizeof (interpreter_t));
  memset (i, 0, sizeof (interpreter_t));

  /* Save the original in case we need to print part of it. */
  i->line_original = line_str;
  /* Make a copy for parsing.  We will treat it as a string table. */
  i->line_str = malloc (line_size);
  memcpy (i->line_str, line_str, line_size);
  i->line_size = line_size;

  i->scan_state = SCAN_START;

  i->token_list_tail = &i->token_list_head;

  if (i->root != 0) {
    ast_node_free (i->root);
  }

  return i;
}

static void
interpreter_destroy (interpreter_t* i)
{
  free (i->line_original);
  free (i->line_str);
  
  while (i->token_list_head != 0) {
    token_t* temp = i->token_list_head;
    i->token_list_head = temp->next;
    token_destroy (temp);
  }

  free (i);
}

static void
interpreter_push_token (interpreter_t* i,
			token_type_t type,
			size_t location,
			const char* string)
{
  token_t* token = token_create (type, location, string);
  *i->token_list_tail = token;
  i->token_list_tail = &token->next;
  token->next = 0;
}

/* static void */
/* interpreter_push_operand (interpreter_t* i, */
/* 			   operand_t* operand) */
/* { */
/*   operand->next = i->operand_stack; */
/*   i->operand_stack = operand; */
/*   ++i->operand_stack_size; */
/* } */

/* static operand_t* */
/* interpreter_pop_operand (interpreter_t* i) */
/* { */
/*   operand_t* operand = i->operand_stack; */
/*   i->operand_stack = operand->next; */
/*   --i->operand_stack_size; */
/*   return operand; */
/* } */

/* Tokens

   string - [a-zA-Z_/-][a-zA-Z0-9_/-]*

   whitespace - [ \t]

   comment - #[^\n]*

   left paren - (

   right paren - )

   integer - [0-9]+

   end of input - 0
 */
static void
interpreter_put (interpreter_t* in,
		 size_t location)
{
  const char c = in->line_original[location];

  switch (in->scan_state) {
  case SCAN_START:
    if ((c >= 'a' && c <= 'z') ||
	(c >= 'A' && c <= 'Z') ||
	c == '_' ||
	c == '/' ||
	c == '-') {
      in->scan_string = in->line_str + location;
      in->scan_string_size = 1;
      in->scan_state = SCAN_STRING;
      break;
    }

    if (c >= '0' && c <= '9') {
      in->scan_string = in->line_str + location;
      in->scan_string_size = 1;
      in->scan_state = SCAN_INTEGER;
      break;
    }

    switch (c) {
    case '(':
      interpreter_push_token (in, TOK_LPAREN, location, 0);
      break;
    case ')':
      interpreter_push_token (in, TOK_RPAREN, location, 0);
      break;
    case '#':
      in->scan_state = SCAN_COMMENT;
      break;
    case ' ':
    case '\t':
      /* Eat whitespace. */
      break;
    case 0:
      interpreter_push_token (in, TOK_END, location, 0);
      in->scan_state = SCAN_DONE;
      break;
    default:
      in->scan_state = SCAN_ERROR;
      in->scan_error_location = location;
      return;
      break;
    }
    break;

  case SCAN_STRING:
    if ((c >= 'a' && c <= 'z') ||
	(c >= 'A' && c <= 'Z') ||
	(c >= '0' && c <= '9') ||
	c == '_' ||
	c == '/' ||
	c == '-') {
      /* Continue. */
      ++in->scan_string_size;
      break;
    }

    {
      /* Terminate with null. */
      in->scan_string[in->scan_string_size] = 0;
      interpreter_push_token (in, TOK_STRING, location, in->scan_string);
      in->scan_state = SCAN_START;
      /* Recur. */
      interpreter_put (in, location);
      return;
    }

  case SCAN_INTEGER:
    if (c >= '0' && c <= '9') {
      /* Continue. */
      ++in->scan_string_size;
      break;
    }

    {
      /* Terminate with null. */
      in->scan_string[in->scan_string_size] = 0;
      interpreter_push_token (in, TOK_INTEGER, location, in->scan_string);
      in->scan_state = SCAN_START;
      /* Recur. */
      interpreter_put (in, location);
      return;
    }

  case SCAN_COMMENT:
    switch (c) {
    case 0:
      in->scan_state = SCAN_DONE;
      break;
    }
    break;

  case SCAN_ERROR:
    /* Do nothing. */
    return;

  case SCAN_DONE:
    /* Do nothing. */
    return;
  }

  return;
}

static bool
match (interpreter_t* in,
       token_type_t type)
{
  if (in->current_token != 0 && in->current_token->type == type) {
    in->current_token = in->current_token->next;
    return true;
  }
  else {
    return false;
  }
}

static ast_node_t*
expressions (interpreter_t* in);

static ast_node_t*
expression (interpreter_t* in)
{
  switch (in->current_token->type) {
  case TOK_LPAREN:
    {
      if (!match (in, TOK_LPAREN)) {
	return ast_node_error (in->current_token);
      }
      token_t* o = in->current_token;
      if (!match (in, TOK_STRING)) {
	return ast_node_error (in->current_token);
      }
      ast_node_t* a = expressions (in);
      if (a != 0 && a->type == AST_ERROR) {
	return a;
      }
      if (!match (in, TOK_RPAREN)) {
	return ast_node_error (in->current_token);
      }
      return ast_node_append_child (ast_node_append_child (ast_node_expression (), ast_node_string (o->string)), a);
    }
  case TOK_STRING:
    {
      token_t* s = in->current_token;
      if (!match (in, TOK_STRING)) {
	return ast_node_error (in->current_token);
      }
      return ast_node_string (s->string);
    }
  case TOK_INTEGER:
    {
      token_t* i = in->current_token;
      if (!match (in, TOK_INTEGER)) {
	return ast_node_error (in->current_token);
      }
      return ast_node_integer (i->string, i->val);
    }
  case TOK_END:
  case TOK_RPAREN:
    return ast_node_error (in->current_token);
  }

  return ast_node_error (in->current_token);
}

static ast_node_t*
expressions (interpreter_t* in)
{
  switch (in->current_token->type) {
  case TOK_LPAREN:
  case TOK_STRING:
  case TOK_INTEGER:
    {
      ast_node_t* n = expression (in);
      if (n->type == AST_ERROR) {
	return n;
      }
      ast_node_t* c = expressions (in);
      if (c != 0 && c->type == AST_ERROR) {
	ast_node_free (n);
	return c;
      }
      ast_node_append_sibling (n, c);
      return n;
    }
  case TOK_RPAREN:
  case TOK_END:
    /* Match lambda. */
    return 0;
  }

  return ast_node_error (in->current_token);
}

static ast_node_t*
program (interpreter_t* in)
{
  switch (in->current_token->type) {
  case TOK_LPAREN:
  case TOK_STRING:
  case TOK_INTEGER:
  case TOK_END:
    {
      ast_node_t* n = expressions (in);
      if (n != 0 && n->type == AST_ERROR) {
	return n;
      }
      if (!match (in, TOK_END)) {
	ast_node_free (n);
	return ast_node_error (in->current_token);
      }
      ast_node_t* p = ast_node_program ();
      ast_node_append_child (p, n);
      return p;
    }
  case TOK_RPAREN:
    return ast_node_error (in->current_token);
  }

  return ast_node_error (in->current_token);
}

static bool
resolve_operators (ast_node_t* n)
{
  bool error = false;

  switch (n->type) {
  case AST_EXPRESSION:
    if (strcmp (n->first_child->u.string, "create") == 0) {
      n->first_child->type = AST_CREATE;
    }
    else {
      bfprintf (&stdout_buffer, "unknown operator: %s\n", n->first_child->u.string);
      error = true;
    }
    break;
  default:
    break;
  }

  for (ast_node_t* c = n->first_child; c != 0; c = c->next_sibling) {
    bool e = resolve_operators (c);
    error = error || e;
  }

  return error;
}

static void
print (const ast_node_t* n,
       int indent)
{
  for (int i = 0; i != indent; ++i) {
    buffer_file_put (&stdout_buffer, ' ');
  }

  switch (n->type) {
  case AST_ERROR:
    bfprintf (&stdout_buffer, "ERROR\n");
    break;
  case AST_PROGRAM:
    bfprintf (&stdout_buffer, "PROGRAM\n");
    break;
  case AST_EXPRESSION:
    bfprintf (&stdout_buffer, "EXPRESSION\n");
    break;
  case AST_STRING:
    bfprintf (&stdout_buffer, "STRING: %s\n", n->u.string);
    break;
  case AST_INTEGER:
    bfprintf (&stdout_buffer, "INTEGER: %s %d\n", n->u.integer.string, n->u.integer.val);
    break;
  case AST_CREATE:
    bfprintf (&stdout_buffer, "CREATE\n");
    break;
  }

  for (const ast_node_t* c = n->first_child; c != 0; c = c->next_sibling) {
    print (c, indent + 2);
  }
}

static void
interpret (char* line_str,
	   size_t line_size)
{
  interpreter_t* i = interpreter_create (line_str, line_size);

  /* Convert the line into a list of tokens. */
  for (size_t idx = 0; idx != line_size; ++idx) {
    interpreter_put (i, idx);
  }

  if (i->scan_state != SCAN_DONE) {
    bfprintf (&stdout_buffer, "syntax error near %s\n", i->line_original + i->scan_error_location);
    interpreter_destroy (i);
    return;
  }

  /* Parse the list of tokens. */
  i->current_token = i->token_list_head;

  i->root = program (i);
  if (i->root->type == AST_ERROR) {
    if (i->root->u.token->type != TOK_END) {
      bfprintf (&stdout_buffer, "syntax error near %s\n", i->line_original + i->root->u.token->location);
    }
    else {
      bfprintf (&stdout_buffer, "syntax error near end of input\n");
    }
    interpreter_destroy (i);
    return;
  }

  if (resolve_operators (i->root)) {
    interpreter_destroy (i);
    return;
  }

  print (i->root, 0);

  interpreter_destroy (i);
}

/*
  End Interpreter Section
  ========================
*/

/*
  Begin Line Editing Section
  ==========================
*/

static char* line_str = 0;
static size_t line_size = 0;
static size_t line_capacity = 0;

static void
line_append (char c)
{
  if (line_size == line_capacity) {
    line_capacity = 2 * line_capacity + 1;
    line_str = realloc (line_str, line_capacity);
  }
  line_str[line_size++] = c;
}

/* Update the line of text. */
static void
line_update (const char* str,
	     size_t size)
{
  for (size_t idx = 0; idx != size; ++idx, ++str) {
    char c = *str;
    if (c >= ' ' && c < 127) {
      /* Printable character. */
      line_append (c);
      if (buffer_file_put (&stdout_buffer, c) != 0) {
	exit ();
      }
    }
    else {
      /* Control character. */
      switch (c) {
      case '\n':
	/* Terminate. */
	line_append (0);
	if (buffer_file_put (&stdout_buffer, '\n') != 0) {
	  exit ();
	}

	/* Interpret the line. */
	interpret (line_str, line_size);
	
	/* Reset. */
	line_str = 0;
	line_size = 0;
	line_capacity = 0;
	break;
      }
    }
  }
}

/*
  End Line Editing Section
  ========================
*/

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    stdout_bd = buffer_create (0);
    if (stdout_bd == -1) {
      exit ();
    }
    if (buffer_file_initw (&stdout_buffer, stdout_bd) != 0) {
      exit ();
    }
  }
}

BEGIN_INTERNAL (NO_PARAMETER, INIT_NO, "init", "", init, ano_t ano, int param)
{
  initialize ();
  finish_internal ();
}

BEGIN_INPUT (NO_PARAMETER, START_NO, "start", "", start, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();

  if (!started) {
    started = true;
    if (buffer_file_puts (&stdout_buffer, PROMPT) != 0) {
      exit ();
    }
  }

  finish_input (bda, bdb);
}

BEGIN_INPUT (NO_PARAMETER, STDIN_NO, "stdin", "buffer_file_t", stdin, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  initialize ();

  buffer_file_t input_buffer;
  if (buffer_file_initr (&input_buffer, bda) != 0) {
    finish_input (bda, bdb);
  }

  size_t size = buffer_file_size (&input_buffer);
  const char* str = buffer_file_readp (&input_buffer, size);
  if (str == 0) {
    finish_input (bda, bdb);
  }

  line_update (str, size);

  finish_input (bda, bdb);
}

static bool
stdout_precondition (void)
{
  return buffer_file_size (&stdout_buffer) != 0;
}

BEGIN_OUTPUT (NO_PARAMETER, STDOUT_NO, "stdout", "buffer_file_t", stdout, ano_t ano, int param)
{
  initialize ();

  if (stdout_precondition ()) {
    buffer_file_truncate (&stdout_buffer);
    finish_output (true, stdout_bd, -1);
  }
  else {
    finish_output (false, -1, -1);
  }
}

void
do_schedule (void)
{
  if (stdout_precondition ()) {
    schedule (STDOUT_NO, 0);
  }
}
