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
  Begin Interpretter Section
  ==========================
*/

typedef enum {
  SCAN_START,
  SCAN_AID,
  SCAN_BID,
  SCAN_STRING,
  SCAN_COMMENT,
  SCAN_ERROR,
} scan_state_t;

typedef enum {
  LPAREN,
  RPAREN,
  SEMICOLON,
  PIPE,

  CREATE,
  LOOKUP,
  BIND,
  LABEL,
  UNLABEL,
  DESTROY,
  UNBIND,
  START,

  AID,
  BID,
  STRING,

  ARGSTART,
} token_type_t;

typedef enum {
  OPERAND_STRING,
  OPERAND_CONSTELLATION,
  OPERAND_ARGSTART,
} operand_type_t;

typedef struct token token_t;
struct token {
  token_type_t type;
  union {
    const char* string;
    int val;
  } u;
  token_t* next;
};

typedef struct operand operand_t;
struct operand {
  operand_type_t type;
  union {
    const char* string;
    struct {
      aid_t* aids;
      size_t aids_size;
      size_t aids_capacity;
      bid_t* bids;
      size_t bids_size;
      size_t bids_capacity;
    } constellation;
  } u;
  operand_t* next;
};

typedef struct interpretter interpretter_t;
struct interpretter {
  char* line_original;
  char* line_str;
  size_t line_size;

  /* Scanner state. */
  scan_state_t scan_state;

  /* Semantic values. */
  char* scan_string;
  size_t scan_string_size;

  /* Railyard shunt algorithm.  Queue is also used during evaluation. */
  token_t* token_queue_head;
  token_t** token_queue_tail;
  token_t* token_stack;

  operand_t* operand_stack;
  size_t operand_stack_size;

  interpretter_t* next;
};

static interpretter_t* interpretter_head = 0;
static interpretter_t** interpretter_tail = &interpretter_head;

static token_t*
token_create (token_type_t type)
{
  token_t* token = malloc (sizeof (token_t));
  memset (token, 0, sizeof (token_t));

  token->type = type;

  return token;
}

static void
token_destroy (token_t* token)
{
  free (token);
}

static operand_t*
operand_create (operand_type_t type)
{
  operand_t* operand = malloc (sizeof (operand_t));
  memset (operand, 0, sizeof (operand_t));
  
  operand->type = type;

  return operand;
}

static void
operand_destroy (operand_t* operand)
{
  if (operand->type == OPERAND_CONSTELLATION) {
    free (operand->u.constellation.aids);
    free (operand->u.constellation.bids);
  }
  free (operand);
}

static void
operand_insert_aid (operand_t* operand,
		    aid_t aid)
{
  for (size_t idx = 0; idx != operand->u.constellation.aids_size; ++idx) {
    if (operand->u.constellation.aids[idx] == aid) {
      return;
    }
  }

  if (operand->u.constellation.aids_size == operand->u.constellation.aids_capacity) {
    operand->u.constellation.aids_capacity = operand->u.constellation.aids_capacity * 2 + 1;
    operand->u.constellation.aids = realloc (operand->u.constellation.aids, operand->u.constellation.aids_capacity * sizeof (aid_t));
  }
  operand->u.constellation.aids[operand->u.constellation.aids_size++] = aid;
}

static void
operand_insert_bid (operand_t* operand,
		    bid_t bid)
{
  for (size_t idx = 0; idx != operand->u.constellation.bids_size; ++idx) {
    if (operand->u.constellation.bids[idx] == bid) {
      return;
    }
  }

  if (operand->u.constellation.bids_size == operand->u.constellation.bids_capacity) {
    operand->u.constellation.bids_capacity = operand->u.constellation.bids_capacity * 2 + 1;
    operand->u.constellation.bids = realloc (operand->u.constellation.bids, operand->u.constellation.bids_capacity * sizeof (bid_t));
  }
  operand->u.constellation.bids[operand->u.constellation.bids_size++] = bid;
}

static operand_t*
operand_to_constellation (operand_t* operand)
{
  /* TODO */
  switch (operand->type) {
  case OPERAND_STRING:
    /* Treat it as a label. */
    operand->type = OPERAND_CONSTELLATION;
    break;
  case OPERAND_CONSTELLATION:
    /* Do nothing. */
    break;
  case OPERAND_ARGSTART:
    /* Do nothing. */
    break;
  }
  return operand;
}

static int
priority (const token_t* token)
{
  switch (token->type) {
  case LPAREN:
    /* If an LPAREN is on the operator stack, any operator can be pushed. */
    return -3;
  case PIPE:
    return -2;
  case CREATE:
  case LOOKUP:
  case BIND:
  case LABEL:
  case UNLABEL:
  case DESTROY:
  case UNBIND:
  case START:
    return -1;
  case RPAREN:
  case SEMICOLON:
  case AID:
  case BID:
  case STRING:
  case ARGSTART:
    /* These should never appear on the stack. */
    return 0;
  }

  return 0;
}

static void
interpretter_push_queue (interpretter_t* i,
			 token_t* token)
{
  *(i->token_queue_tail) = token;
  i->token_queue_tail = &token->next;
  token->next = 0;
}

static token_t*
interpretter_pop_queue (interpretter_t* i)
{
  token_t* t = i->token_queue_head;
  i->token_queue_head = t->next;
  if (i->token_queue_head == 0) {
    i->token_queue_tail = &i->token_queue_head;
  }
  return t;
}

static void
interpretter_push_stack (interpretter_t* i,
			 token_t* token)
{
  token->next = i->token_stack;
  i->token_stack = token;
}

static token_t*
interpretter_pop_stack (interpretter_t* i)
{
  token_t* token = i->token_stack;
  i->token_stack = token->next;
  return token;
}

static void
interpretter_push_operand (interpretter_t* i,
			   operand_t* operand)
{
  operand->next = i->operand_stack;
  i->operand_stack = operand;
  ++i->operand_stack_size;
}

static operand_t*
interpretter_pop_operand (interpretter_t* i)
{
  operand_t* operand = i->operand_stack;
  i->operand_stack = operand->next;
  --i->operand_stack_size;
  return operand;
}

static void
interpretter_push_token (interpretter_t* i,
			 token_type_t type,
			 char* string)
{
  /* Find keywords. */
  if (type == STRING) {
    if (strcmp ("create", string) == 0) {
      type = CREATE;
    }
    else if (strcmp ("lookup", string) == 0) {
      type = LOOKUP;
    }
    else if (strcmp ("bind", string) == 0) {
      type = BIND;
    }
    else if (strcmp ("label", string) == 0) {
      type = LABEL;
    }
    else if (strcmp ("unlabel", string) == 0) {
      type = UNLABEL;
    }
    else if (strcmp ("destroy", string) == 0) {
      type = DESTROY;
    }
    else if (strcmp ("unbind", string) == 0) {
      type = UNBIND;
    }
    else if (strcmp ("start", string) == 0) {
      type = START;
    }
  }

  token_t* token = token_create (type);

  switch (type) {
  case LPAREN:
    interpretter_push_queue (i, token);
    break;
  case RPAREN:
    while (i->token_stack != 0 && i->token_stack->type != LPAREN) {
      interpretter_push_queue (i, interpretter_pop_stack (i));
    }
    if (i->token_stack == 0) {
      /* TODO:  Mismatched parentheses. */
    }
    /* Destroy the left. */
    token_destroy (interpretter_pop_stack (i));
    /* Destroy the right. */
    token_destroy (token);
    break;
  case SEMICOLON:
    /* Drain the stack. */
    while (i->token_stack != 0) {
      token_t* t = interpretter_pop_stack (i);
      if (t->type == LPAREN) {
	/* TODO:  Mismatched parentheses. */
      }
      interpretter_push_queue (i, t);
    }
    interpretter_push_queue (i, token);
    break;
  case CREATE:
  case BIND:
    /* Variable number of arguments. */
    interpretter_push_queue (i, token_create (ARGSTART));
    /* Fall through. */
  case PIPE:
  case LOOKUP:
  case LABEL:
  case UNLABEL:
  case DESTROY:
  case UNBIND:
  case START:
    while (i->token_stack != 0 && priority (i->token_stack) > priority (token)) {
      token_t* t = interpretter_pop_stack (i);
      interpretter_push_queue (i, t);
    }
    interpretter_push_stack (i, token);      
    break;
  case AID:
    token->u.val = atoi (string);
    interpretter_push_queue (i, token);
    break;
  case BID:
    token->u.val = atoi (string);
    interpretter_push_queue (i, token);
    break;
  case STRING:
    token->u.string = string;
    interpretter_push_queue (i, token);
    break;
  case ARGSTART:
    /* TODO:  Should never appear. */
    break;
  }
}

/* Tokens

   string - [a-zA-Z0-9_/-]+

   whitespace - [ \t]

   comment - #[^\n]*

   left paren - (

   right paren - )

   comma - ,

   semicolon - ;

   evaluation symbol - [;\n]

   automaton id - @[0-9]+

   binding id - &[0-9]+
 */
static void
interpretter_put (interpretter_t* in,
		  char c,
		  char* str)
{
  switch (in->scan_state) {
  case SCAN_START:
    if ((c >= 'a' && c <= 'z') ||
	(c >= 'A' && c <= 'Z') ||
	(c >= '0' && c <= '9') ||
	c == '_' ||
	c == '/' ||
	c == '-') {
      in->scan_string = str;
      in->scan_string_size = 1;
      in->scan_state = SCAN_STRING;
      break;
    }

    switch (c) {
    case '(':
      interpretter_push_token (in, LPAREN, 0);
      break;
    case ')':
      interpretter_push_token (in, RPAREN, 0);
      break;
    case ';':
      interpretter_push_token (in, SEMICOLON, 0);
      break;
    case '|':
      interpretter_push_token (in, PIPE, 0);
      break;
    case '@':
      in->scan_string = str + 1;
      in->scan_string_size = 0;
      in->scan_state = SCAN_AID;
      break;
    case '&':
      in->scan_string = str + 1;
      in->scan_string_size = 0;
      in->scan_state = SCAN_BID;
      break;
    case '#':
      in->scan_state = SCAN_COMMENT;
      break;
    case ' ':
    case '\t':
    case '\n':
      /* Eat whitespace. */
      break;
    case 0:
      /* Do nothing. */
      break;
    default:
      if (*str != 0) {
	bfprintf (&stdout_buffer, "syntax error near %s\n", str);
      }
      else {
	bfprintf (&stdout_buffer, "syntax error\n", str);
      }
      in->scan_state = SCAN_ERROR;
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

    // Not a valid string character.
    {
      /* Terminate with null. */
      in->scan_string[in->scan_string_size] = 0;
      interpretter_push_token (in, STRING, in->scan_string);
      in->scan_state = SCAN_START;
      // Recur.
      return interpretter_put (in, c, str);
    }

  case SCAN_AID:
    if (c >= '0' && c <= '9') {
      /* Continue. */
      ++in->scan_string_size;
      break;
    }

    // Not a valid aid character.
    {
      /* Terminate with null. */
      in->scan_string[in->scan_string_size] = 0;
      interpretter_push_token (in, AID, in->scan_string);
      in->scan_state = SCAN_START;
      // Recur.
      return interpretter_put (in, c, str);
    }
    
  case SCAN_BID:
    if (c >= '0' && c <= '9') {
      /* Continue. */
      ++in->scan_string_size;
      break;
    }

    // Not a valid bid character.
    {
      /* Terminate with null. */
      in->scan_string[in->scan_string_size] = 0;
      interpretter_push_token (in, BID, in->scan_string);
      in->scan_state = SCAN_START;
      // Recur.
      return interpretter_put (in, c, str);
    }

  case SCAN_COMMENT:
    switch (c) {
    case 0:
      in->scan_state = SCAN_START;
      break;
    }
    break;

  case SCAN_ERROR:
    /* Do nothing. */
    return;
  }

  return;
}

static void
interpret (char* line_str,
	   size_t line_size)
{
  interpretter_t* i = malloc (sizeof (interpretter_t));
  memset (i, 0, sizeof (interpretter_t));

  i->line_original = line_str;
  i->line_str = malloc (line_size);
  memcpy (i->line_str, line_str, line_size);
  i->line_size = line_size;

  i->scan_state = SCAN_START;

  i->token_queue_tail = &i->token_queue_head;

  bfprintf (&stdout_buffer, "interpretting: %s\n", line_str);

  /* Process the line. */
  for (size_t idx = 0; idx != line_size; ++idx) {
    interpretter_put (i, i->line_str[idx], i->line_str + idx);
  }

  /* Move tokens on the stack to the queue. */
  while (i->token_stack != 0) {
    token_t* t = interpretter_pop_stack (i);
    interpretter_push_queue (i, t);
  }

  /* *interpretter_tail = i; */
  /* interpretter_tail = &i->next; */

  /* while (i->token_queue_head) { */
  /*   token_t* token = interpretter_pop_queue (i); */
  /*   switch (token->type) { */
  /*   case LPAREN: */
  /*   case RPAREN: */
  /*     /\* TODO:  These should not appear. *\/ */
  /*     break; */
  /*   case SEMICOLON: */
  /*     if (i->operand_stack != 0) { */
  /* 	/\* The operand stack contains a value.  Destroy it. *\/ */
  /* 	operand_destroy (interpretter_pop_operand (i)); */
  /*     } */
  /*     if (i->operand_stack != 0) { */
  /* 	/\* TODO:  Error the stack should be clear. *\/ */
  /*     } */
  /*     break; */
  /*   case PIPE: */
  /*     { */
  /* 	if (i->operand_stack_size >= 2) { */
  /* 	  operand_t* input = operand_to_constellation (interpretter_pop_operand (i)); */
  /* 	  operand_t* output = operand_to_constellation (interpretter_pop_operand (i)); */

  /* 	  if (output->type == OPERAND_CONSTELLATION && input->type == OPERAND_CONSTELLATION) { */
  /* 	    bfprintf (&stdout_buffer, "autobind\n"); */
  /* 	  } */
  /* 	  else { */
  /* 	    /\* TODO:  Probably not enough arguments. *\/ */
  /* 	  } */

  /* 	  operand_destroy (output); */
  /* 	  interpretter_push_operand (i, input); */
  /* 	} */
  /* 	else { */
  /* 	  /\* TODO:  Underflow. *\/ */
  /* 	} */
  /*     } */
  /*     break; */
  /*   case CREATE: */
  /*     { */
  /* 	operand_t* top = 0; */
	
  /* 	/\* Pop the operand stack until we find an arg start. *\/ */
  /* 	while (i->operand_stack != 0 && */
  /* 	       (i->operand_stack->type == OPERAND_STRING || i->operand_stack->type == OPERAND_ARGSTART)) { */
  /* 	  operand_t* o = interpretter_pop_operand (i); */
  /* 	  o->next = top; */
  /* 	  top = o; */
  /* 	  if (o->type == OPERAND_ARGSTART) { */
  /* 	    break; */
  /* 	  } */
  /* 	} */

  /* 	if (top == 0 || top->type != OPERAND_ARGSTART) { */
  /* 	  /\* TODO:  Error. *\/ */
  /* 	} */

  /* 	bfprintf (&stdout_buffer, "executing: create\n"); */
  /* 	while (top != 0) { */
  /* 	  bfprintf (&stdout_buffer, "\t%s\n", top->u.string); */
  /* 	  operand_t* o = top; */
  /* 	  top = top->next; */
  /* 	  operand_destroy (o); */
  /* 	} */

  /* 	operand_t* result = operand_create (OPERAND_CONSTELLATION); */
  /* 	operand_insert_aid (result, -1); */
  /* 	interpretter_push_operand (i, result); */
  /*     } */
  /*     break; */
  /*   case LOOKUP: */
  /*     { */
  /* 	if (i->operand_stack != 0 && i->operand_stack->type == OPERAND_STRING) { */
  /* 	  operand_t* o = interpretter_pop_operand (i); */
  /* 	  bfprintf (&stdout_buffer, "executing: lookup %s\n", o->u.string); */
  /* 	  operand_destroy (o); */
	  
  /* 	  o = operand_create (OPERAND_CONSTELLATION); */
  /* 	  operand_insert_aid (o, -1); */
  /* 	  interpretter_push_operand (i, o); */
  /* 	} */
  /* 	else { */
  /* 	  /\* TODO *\/ */
  /* 	  bfprintf (&stdout_buffer, "error: lookup takes exactly one string\n"); */
  /* 	} */
  /*     } */
  /*     break; */
  /*   case BIND: */
  /*     { */
  /* 	operand_t* top = 0; */
	
  /* 	/\* Pop the operand stack until we find an arg start. *\/ */
  /* 	while (i->operand_stack != 0 && */
  /* 	       (i->operand_stack->type == OPERAND_STRING || i->operand_stack->type == OPERAND_ARGSTART)) { */
  /* 	  operand_t* o = interpretter_pop_operand (i); */
  /* 	  o->next = top; */
  /* 	  top = o; */
  /* 	  if (o->type == OPERAND_ARGSTART) { */
  /* 	    break; */
  /* 	  } */
  /* 	} */

  /* 	if (top == 0 || top->type != OPERAND_ARGSTART) { */
  /* 	  /\* TODO:  Error. *\/ */
  /* 	} */

  /* 	bfprintf (&stdout_buffer, "executing: bind\n"); */
  /* 	while (top != 0) { */
  /* 	  bfprintf (&stdout_buffer, "\t%s\n", top->u.string); */
  /* 	  operand_t* o = top; */
  /* 	  top = top->next; */
  /* 	  operand_destroy (o); */
  /* 	} */

  /* 	operand_t* result = operand_create (OPERAND_CONSTELLATION); */
  /* 	operand_insert_aid (result, -1); */
  /* 	interpretter_push_operand (i, result); */
  /*     } */
  /*     break; */
  /*   case LABEL: */
  /*     { */
  /* 	if (i->operand_stack_size >= 2) { */
  /* 	  operand_t* label = interpretter_pop_operand (i); */
  /* 	  operand_t* constellation = operand_to_constellation (interpretter_pop_operand (i)); */

  /* 	  if (label->type == OPERAND_STRING && constellation->type == OPERAND_CONSTELLATION) { */
  /* 	    bfprintf (&stdout_buffer, "label [constellation] %s\n", label->u.string); */
  /* 	  } */
  /* 	  else { */
  /* 	    /\* TODO:  Probably not enough arguments. *\/ */
  /* 	  } */

  /* 	  operand_destroy (label); */
  /* 	  interpretter_push_operand (i, constellation); */
  /* 	} */
  /* 	else { */
  /* 	  /\* TODO:  Underflow. *\/ */
  /* 	} */
  /*     } */
  /*     break; */
  /*   case UNLABEL: */
  /*     { */
  /* 	if (i->operand_stack_size >= 2) { */
  /* 	  operand_t* label = interpretter_pop_operand (i); */
  /* 	  operand_t* constellation = operand_to_constellation (interpretter_pop_operand (i)); */

  /* 	  if (label->type == OPERAND_STRING && constellation->type == OPERAND_CONSTELLATION) { */
  /* 	    bfprintf (&stdout_buffer, "unlabel [constellation] %s\n", label->u.string); */
  /* 	  } */
  /* 	  else { */
  /* 	    /\* TODO:  Probably not enough arguments. *\/ */
  /* 	  } */

  /* 	  operand_destroy (label); */
  /* 	  interpretter_push_operand (i, constellation); */
  /* 	} */
  /* 	else { */
  /* 	  /\* TODO:  Underflow. *\/ */
  /* 	} */
  /*     } */
  /*     break; */
  /*   case DESTROY: */
  /*     { */
  /* 	if (i->operand_stack != 0) { */
  /* 	  operand_t* constellation = operand_to_constellation (i->operand_stack); */
  /* 	  if (constellation->type == OPERAND_CONSTELLATION) { */
  /* 	    for (size_t idx = 0; idx != i->operand_stack->u.constellation.aids_size; ++idx) { */
  /* 	      bfprintf (&stdout_buffer, "executing: destroy %d\n", i->operand_stack->u.constellation.aids[idx]); */
  /* 	    } */
  /* 	    i->operand_stack->u.constellation.aids_size = 0; */
  /* 	  } */
  /* 	  else { */
  /* 	    /\* TODO *\/ */
  /* 	    bfprintf (&stdout_buffer, "error: couldn't convert argument to constellation\n"); */
  /* 	  } */
  /* 	} */
  /* 	else { */
  /* 	  /\* TODO *\/ */
  /* 	  bfprintf (&stdout_buffer, "error: start takes exactly one constellation\n"); */
  /* 	} */
  /*     } */
  /*     break; */
  /*   case UNBIND: */
  /*     { */
  /* 	if (i->operand_stack != 0) { */
  /* 	  operand_t* constellation = operand_to_constellation (i->operand_stack); */
  /* 	  if (constellation->type == OPERAND_CONSTELLATION) { */
  /* 	    for (size_t idx = 0; idx != i->operand_stack->u.constellation.bids_size; ++idx) { */
  /* 	      bfprintf (&stdout_buffer, "executing: unbind %d\n", i->operand_stack->u.constellation.bids[idx]); */
  /* 	    } */
  /* 	    i->operand_stack->u.constellation.bids_size = 0; */
  /* 	  } */
  /* 	  else { */
  /* 	    /\* TODO *\/ */
  /* 	    bfprintf (&stdout_buffer, "error: couldn't convert argument to constellation\n"); */
  /* 	  } */
  /* 	} */
  /* 	else { */
  /* 	  /\* TODO *\/ */
  /* 	  bfprintf (&stdout_buffer, "error: start takes exactly one constellation\n"); */
  /* 	} */
  /*     } */
  /*     break; */
  /*   case START: */
  /*     { */
  /* 	if (i->operand_stack != 0) { */
  /* 	  operand_t* constellation = operand_to_constellation (i->operand_stack); */
  /* 	  if (constellation->type == OPERAND_CONSTELLATION) { */
  /* 	    for (size_t idx = 0; idx != i->operand_stack->u.constellation.aids_size; ++idx) { */
  /* 	      bfprintf (&stdout_buffer, "executing: start %d\n", i->operand_stack->u.constellation.aids[idx]); */
  /* 	    } */
  /* 	  } */
  /* 	  else { */
  /* 	    /\* TODO *\/ */
  /* 	    bfprintf (&stdout_buffer, "error: couldn't convert argument to constellation\n"); */
  /* 	  } */
  /* 	} */
  /* 	else { */
  /* 	  /\* TODO *\/ */
  /* 	  bfprintf (&stdout_buffer, "error: start takes exactly one constellation\n"); */
  /* 	} */
  /*     } */
  /*     break; */
  /*   case AID: */
  /*     { */
  /* 	operand_t* operand = operand_create (OPERAND_CONSTELLATION); */
  /* 	operand_insert_aid (operand, token->u.val); */
  /* 	interpretter_push_operand (i, operand); */
  /*     } */
  /*     break; */
  /*   case BID: */
  /*     { */
  /* 	operand_t* operand = operand_create (OPERAND_CONSTELLATION); */
  /* 	operand_insert_bid (operand, token->u.val); */
  /* 	interpretter_push_operand (i, operand); */
  /*     } */
  /*     break; */
  /*   case STRING: */
  /*     { */
  /* 	operand_t* operand = operand_create (OPERAND_STRING); */
  /* 	operand->u.string = token->u.string; */
  /* 	interpretter_push_operand (i, operand); */
  /*     } */
  /*     break; */
  /*   case ARGSTART: */
  /*     { */
  /* 	interpretter_push_operand (i, operand_create (OPERAND_ARGSTART)); */
  /*     } */
  /*     break; */
  /*   } */

  /*   /\* Destroy the token. *\/ */
  /*   token_destroy (token); */
  /* } */
}

/*
  End Interpretter Section
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

	/* Create a new interpretter. */
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
