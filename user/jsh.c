#include <automaton.h>
#include <buffer_file.h>
#include <dymem.h>
#include <string.h>
#include <description.h>
#include <ctype.h>
#include "de.h"
#include "environment.h"
#include "system.h"
#include "fs_set.h"

/* TODO:  Improve the error handling. */

#define INIT_NO 1
#define TEXT_IN_NO 2
#define PROCESS_LINE_NO 3
#define TEXT_OUT_NO 4
#define DESTROYED_NO 5
#define UNBOUND_NO 6
#define COM_OUT_NO 7
#define COM_IN_NO 8
#define RECV_NO 9
#define SEND_NO 10
#define BIND_ACTION_NO 11
#define INIT_OUT_NO 12
#define STATUS_REQUEST_OUT_NO 13
#define STATUS_REQUEST_IN_NO 14
#define STATUS_RESPONSE_OUT_NO 15
#define STATUS_RESPONSE_IN_NO 16
#define BINDING_UPDATE_OUT_NO 17
#define BINDING_UPDATE_IN_NO 18

#define FS_DESCEND_REQUEST_OUT_NO 21
#define FS_DESCEND_RESPONSE_IN_NO 22
#define FS_READFILE_REQUEST_OUT_NO 23
#define FS_READFILE_RESPONSE_IN_NO 24

/* Initialization flag. */
static bool initialized = false;

/* Output buffers. */
static bd_t output_bda = -1;
static bd_t output_bdb = -1;
static buffer_file_t output_bfa;

/* System automaton. */
static system_t system;

/* File system set. */
static fs_set_t fs_set;

/* static bd_t text_out_bd = -1; */
/* static buffer_file_t text_out_buffer; */

/* static bool process_hold = false; */

/* static bd_t com_out_bd = -1; */
/* static buffer_file_t com_out_buffer; */

#define LOG_BUFFER_SIZE 128
static char log_buffer[LOG_BUFFER_SIZE];
#define ERROR __FILE__ ": error: "
#define WARNING __FILE__ ": warning: "
#define INFO __FILE__ ": info: "
#define TODO __FILE__ ": todo: "

typedef struct automaton_item automaton_item_t;
/* typedef struct binding_item binding_item_t; */

struct automaton_item {
  automaton_t* automaton;
  char* name_begin;
  char* name_end;
  automaton_item_t* next;
};

/* struct binding { */
/*   bid_t bid; */
/*   automaton_t* output_automaton; */
/*   ano_t output_action; */
/*   char* output_action_name; */
/*   int output_parameter; */
/*   automaton_t* input_automaton; */
/*   ano_t input_action; */
/*   char* input_action_name; */
/*   int input_parameter; */
/*   binding_t* next; */
/* }; */

/* struct binding_item { */
/*   binding_t* binding; */
/*   binding_item_t* next; */
/* }; */

//static automaton_item_t* this_automaton = 0;
static automaton_item_t* automaton_head = 0;
/* static binding_t* bindings_head = 0; */

/* static automaton_item_t* */
/* create_automaton (automaton_t* a, */
/* 		  const char* name_begin, */
/* 		  const char* name_end) */
/* { */
/*   if (name_end == 0) { */
/*     name_end = name_begin + strlen (name_begin); */
/*   } */
/*   automaton_item_t* ai = malloc (sizeof (automaton_item_t)); */
/*   memset (ai, 0, sizeof (automaton_item_t)); */
/*   ai->automaton = a; */
/*   size_t name_size = name_end - name_begin; */
/*   ai->name_begin = malloc (name_size); */
/*   memcpy (ai->name_begin, name_begin, name_size); */
/*   ai->name_end = ai->name_begin + name_size; */
  
/*   ai->next = automaton_head; */
/*   automaton_head = ai; */
  
/*   return ai; */
/* } */

/* static void */
/* destroy_automaton (automaton_t* automaton) */
/* { */
/*   free (automaton->name); */
/*   free (automaton->path); */
/*   free (automaton); */
/* } */

static automaton_item_t*
find_automaton (const char* name_begin,
		const char* name_end)
{
  automaton_item_t* automaton;
  for (automaton = automaton_head; automaton != 0; automaton = automaton->next) {
    if (pstrcmp (automaton->name_begin, automaton->name_end, name_begin, name_end) == 0) {
      break;
    }
  }
  return automaton;
}

/* static automaton_t* */
/* find_automaton_aid (aid_t aid) */
/* { */
/*   automaton_t* automaton; */
/*   for (automaton = automata_head; automaton != 0; automaton = automaton->next) { */
/*     if (automaton->aid == aid) { */
/*       break; */
/*     } */
/*   } */
/*   return automaton; */
/* } */

/* static binding_t* */
/* create_binding (bid_t bid, */
/* 		automaton_t* output_automaton, */
/* 		ano_t output_action, */
/* 		const char* output_action_name, */
/* 		int output_parameter, */
/* 		automaton_t* input_automaton, */
/* 		ano_t input_action, */
/* 		const char* input_action_name, */
/* 		int input_parameter) */
/* { */
/*   binding_t* binding = malloc (sizeof (binding_t)); */
/*   memset (binding, 0, sizeof (binding_t)); */

/*   binding->bid = bid; */
/*   binding->output_automaton = output_automaton; */
/*   binding->output_action = output_action; */
/*   size_t size = strlen (output_action_name) + 1; */
/*   binding->output_action_name = malloc (size); */
/*   memcpy (binding->output_action_name, output_action_name, size); */
/*   binding->output_parameter = output_parameter; */
/*   binding->input_automaton = input_automaton; */
/*   binding->input_action = input_action; */
/*   size = strlen (input_action_name) + 1; */
/*   binding->input_action_name = malloc (size); */
/*   memcpy (binding->input_action_name, input_action_name, size); */
/*   binding->input_parameter = input_parameter; */

/*   return binding; */
/* } */

/* static void */
/* destroy_binding (binding_t* binding) */
/* { */
/*   free (binding->output_action_name); */
/*   free (binding->input_action_name); */
/*   free (binding); */
/* } */

/* static void */
/* unbound_ (binding_t** ptr) */
/* { */
/*   if (*ptr != 0) { */
/*     binding_t* binding = *ptr; */
/*     *ptr = binding->next; */
    
/*     bfprintf (&text_out_buffer, "-> %d: (%s, %s, %d) -> (%s, %s, %d) unbound\n", binding->bid, binding->output_automaton->name, binding->output_action_name, binding->output_parameter, binding->input_automaton->name, binding->input_action_name, binding->input_parameter); */
    
/*     destroy_binding (binding); */
/*   } */
/* } */

/* static void */
/* destroyed_ (automaton_t** aptr) */
/* { */
/*   if (*aptr != 0) { */
/*     automaton_t* automaton = *aptr; */
/*     *aptr = automaton->next; */

/*     /\* Remove the bindings. *\/ */
/*     binding_t** bptr = &bindings_head; */
/*     while (*bptr != 0) { */
/*       if ((*bptr)->output_automaton == automaton || */
/* 	  (*bptr)->input_automaton == automaton) { */
/* 	binding_t* binding = *bptr; */
/* 	*bptr = binding->next; */

/* 	bfprintf (&text_out_buffer, "-> %d: (%s, %s, %d) -> (%s, %s, %d) unbound\n", binding->bid, binding->output_automaton->name, binding->output_action_name, binding->output_parameter, binding->input_automaton->name, binding->input_action_name, binding->input_parameter); */

/* 	destroy_binding (binding); */
/*       } */
/*       else { */
/* 	bptr = &(*bptr)->next; */
/*       } */
/*     } */

/*     bfprintf (&text_out_buffer, "-> %d: destroyed\n", automaton->aid); */

/*     destroy_automaton (automaton); */
/*   } */
/* } */

/*
  Begin Line Queue Section
  ========================
*/

/* typedef struct string string_t; */
/* struct string { */
/*   char* str; */
/*   size_t size; */
/*   string_t* next; */
/* }; */

/* static string_t* string_head = 0; */
/* static string_t** string_tail = &string_head; */

/* static void */
/* string_push_front (char* str, */
/* 		   size_t size) */
/* { */
/*   string_t* s = malloc (sizeof (string_t)); */
/*   s->str = str; */
/*   s->size = size; */
/*   s->next = string_head; */
/*   string_head = s; */

/*   if (s->next == 0) { */
/*     string_tail = &s->next; */
/*   } */
/* } */

/* static void */
/* string_push_back (char* str, */
/* 		  size_t size) */
/* { */
/*   string_t* s = malloc (sizeof (string_t)); */
/*   s->str = str; */
/*   s->size = size; */
/*   s->next = 0; */
/*   *string_tail = s; */
/*   string_tail = &s->next; */
/* } */

/* static void */
/* string_front (char** str, */
/* 	      size_t* size) */
/* { */
/*   *str = string_head->str; */
/*   *size = string_head->size; */
/* } */

/* static void */
/* string_pop (void) */
/* { */
/*   string_t* s = string_head; */
/*   string_head = s->next; */
/*   if (string_head == 0) { */
/*     string_tail = &string_head; */
/*   } */

/*   free (s); */
/* } */

/* static bool */
/* string_empty (void) */
/* { */
/*   return string_head == 0; */
/* } */

/* static size_t current_string_idx = 0; */

/* static char* current_line = 0; */
/* static size_t current_line_size = 0; */
/* static size_t current_line_capacity = 0; */

/* static void */
/* line_append (char c) */
/* { */
/*   if (current_line_size == current_line_capacity) { */
/*     current_line_capacity = 2 * current_line_capacity + 1; */
/*     current_line = realloc (current_line, current_line_capacity); */
/*   } */
/*   current_line[current_line_size++] = c; */
/* } */

/* static void */
/* line_back (void) */
/* { */
/*   if (current_line_size != 0) { */
/*     --current_line_size; */
/*   } */
/* } */

/* static void */
/* line_reset (void) */
/* { */
/*   current_line_size = 0; */
/* } */

/*
  End Line Queue Section
  ======================
*/

typedef struct {
  const char* begin;
  const char* end;
} string_t;

/*
  Begin Dispatch Section
  ======================
*/

/* typedef struct com_item com_item_t; */
/* struct com_item { */
/*   aid_t aid; */
/*   char* str; */
/*   com_item_t* next; */
/* }; */

/* static com_item_t* com_head = 0; */
/* static com_item_t** com_tail = &com_head; */

/* static bool */
/* com_queue_empty (void) */
/* { */
/*   return com_head == 0; */
/* } */

/* static aid_t */
/* com_queue_front (void) */
/* { */
/*   return com_head->aid; */
/* } */

/* static void */
/* com_queue_push (aid_t aid, */
/* 		const char* str) */
/* { */
/*   com_item_t* item = malloc (sizeof (com_item_t)); */
/*   memset (item, 0, sizeof (com_item_t)); */
/*   item->aid = aid; */
/*   size_t len = strlen (str) + 1; */
/*   item->str = malloc (len); */
/*   memcpy (item->str, str, len); */
/*   *com_tail = item; */
/*   com_tail = &item->next; */
/* } */

/* static void */
/* com_queue_pop (void) */
/* { */
/*   com_item_t* item = com_head; */
/*   com_head = item->next; */
/*   if (com_head == 0) { */
/*     com_tail = &com_head; */
/*   } */

/*   free (item->str); */
/*   free (item); */
/* } */

/* static const char* create_name = 0; */
/* static bool create_retain_privilege = false; */
/* static const char* create_register_name = 0; */
/* static const char* create_path = 0; */
/* static size_t create_argv_idx = 0; */

/* static void */
/* create_readfile_callback (void* arg, */
/* 			  const fs_readfile_response_t* res, */
/* 			  bd_t bd) */
/* { */
/*   //automaton_item_t* ai = arg; */
/*   switch (res->error) { */
/*   case FS_READFILE_SUCCESS: */
/*     //automaton_set_text (ai->automaton, bd); */
/*     buffer_destroy (bd); */
/*     break; */
/*   default: */
/*     /\* TODO *\/ */
/*     logs (ERROR "TODO:  could not read file"); */
/*     break; */
/*   } */
/* } */

static void
create_usage (void)
{
  /* TODO */
  logs (ERROR "TODO:  create_usage");
  /* buffer_file_puts (&text_out_buffer, "-> usage: create [-p ] NAME PATH [OPTIONS...]\n"); */
}

static bool
create_ (const string_t* strings,
	 size_t size)
{
  if (pstrcmp ("create", 0, strings[0].begin, strings[0].end) == 0) {
    bool retain_privilege = false;
    size_t name_idx;
    size_t path_idx;

    size_t idx = 1;
    if (size > idx && pstrcmp ("-p", 0, strings[idx].begin, strings[idx].end) == 0) {
      retain_privilege = true;
      ++idx;
    }

    /* The name should be next. */
    if (size > idx) {
      name_idx = idx;
      ++idx;
    }
    else {
      create_usage ();
      return true;
    }

    /* The path should be next. */
    if (size > idx) {
      path_idx = idx;
      ++idx;
    }
    else {
      create_usage ();
      return true;
    }

    /* The rest are arguments. */
    /* TODO:  Pass the arguments. */

    if (find_automaton (strings[name_idx].begin, strings[name_idx].end) == 0) {
      //automaton_t* a = system_add_managed_automaton (&system, -1, -1, -1, retain_privilege);
      //automaton_item_t* ai = create_automaton (a, strings[name_idx].begin, strings[name_idx].end);
      //fs_set_readfile (&fs_set, strings[path_idx].begin, strings[path_idx].end, create_readfile_callback, ai);
    }
    else {
      logs (ERROR "TODO:  name already taken");
    }

    return true;
  }

  return false;
}

/* static void */
/* bind_usage (void) */
/* { */
/*   /\* TODO *\/ */
/*   logs (ERROR "TODO:  bind_usage"); */
/*   /\* buffer_file_puts (&text_out_buffer, "-> usage: bind [-o OPARAM -i IPARAM] OAID OACTION IAID IACTION\n"); *\/ */
/* } */

/* static bool */
/* bind_ (const string_t* strings, */
/*        size_t size) */
/* { */
/*   if (pstrcmp ("bind", 0, strings[0].begin, strings[0].end) == 0) { */
/*     automaton_item_t* output_automaton; */
/*     size_t output_action_idx; */
/*     int output_parameter = 0; */
/*     automaton_item_t* input_automaton; */
/*     size_t input_action_idx; */
/*     int input_parameter = 0; */

/*     size_t idx = 1; */
/*     if (size > idx && pstrcmp ("-o", 0, strings[idx].begin, strings[idx].end) == 0) { */
/*       ++idx; */

/*       if (size > idx) { */
/* 	output_parameter = pstrtol (strings[idx].begin, strings[idx].end, 0, 0); */
/* 	if (string_error != STRING_SUCCESS) { */
/* 	  logs (ERROR "TODO:  could not parse output parameter"); */
/* 	  return true; */
/* 	} */
/* 	++idx; */
/*       } */
/*       else { */
/* 	bind_usage (); */
/* 	return true; */
/*       } */
/*     } */

/*     if (size > idx && pstrcmp ("-i", 0, strings[idx].begin, strings[idx].end) == 0) { */
/*       ++idx; */

/*       if (size > idx) { */
/* 	input_parameter = pstrtol (strings[idx].begin, strings[idx].end, 0, 0); */
/* 	if (string_error != STRING_SUCCESS) { */
/* 	  logs (ERROR "TODO:  could not parse input parameter"); */
/* 	  return true; */
/* 	} */
/* 	++idx; */
/*       } */
/*       else { */
/* 	bind_usage (); */
/* 	return true; */
/*       } */
/*     } */

/*     if (size > idx) { */
/*       output_automaton = find_automaton (strings[idx].begin, strings[idx].end); */
/*       if (output_automaton == 0) { */
/* 	logs (ERROR "TODO:  output automaton not declared"); */
/* 	return true; */
/*       } */
/*       ++idx; */
/*     } */
/*     else { */
/*       bind_usage (); */
/*       return true; */
/*     } */

/*     if (size > idx) { */
/*       output_action_idx = idx; */
/*       ++idx; */
/*     } */
/*     else { */
/*       bind_usage (); */
/*       return true; */
/*     } */

/*     if (size > idx) { */
/*       input_automaton = find_automaton (strings[idx].begin, strings[idx].end); */
/*       if (input_automaton == 0) { */
/* 	logs (ERROR "TODO:  input automaton not declared"); */
/* 	return true; */
/*       } */
/*       ++idx; */
/*     } */
/*     else { */
/*       bind_usage (); */
/*       return true; */
/*     } */

/*     if (size > idx) { */
/*       input_action_idx = idx; */
/*       ++idx; */
/*     } */
/*     else { */
/*       bind_usage (); */
/*       return true; */
/*     } */
    
/*     /\* Are we globbing? *\/ */
/*     const char* output_glob = pstrchr (strings[output_action_idx].begin, strings[output_action_idx].end,'*'); */
/*     const char* input_glob = pstrchr (strings[input_action_idx].begin, strings[input_action_idx].end, '*'); */

/*     if (!((output_glob == strings[output_action_idx].end && input_glob == strings[input_action_idx].end) || */
/* 	  (output_glob != strings[output_action_idx].end && input_glob != strings[input_action_idx].end))) { */
/*       logs (ERROR "TODO:  glob disagreement"); */
/*       return true; */
/*     } */

/*     if (output_glob == strings[output_action_idx].end) { */
/*       system_add_binding (&system, output_automaton->automaton, strings[output_action_idx].begin, strings[output_action_idx].end, -1, output_parameter, input_automaton->automaton, strings[input_action_idx].begin, strings[input_action_idx].end, -1, input_parameter, system_get_this (&system)); */
/*     } */
/*     else { */
/*       system_add_globbed_binding (&system, output_automaton->automaton, strings[output_action_idx].begin, strings[output_action_idx].end, output_parameter, input_automaton->automaton, strings[input_action_idx].begin, strings[input_action_idx].end, input_parameter, system_get_this (&system)); */
/*     } */

/*     return true; */
/*   } */

/*   return false; */
/* } */

/* static bool */
/* unbind_ (void) */
/* { */
/*   if (scan_strings_size >= 1 && strcmp (scan_strings[0], "unbind") == 0) { */
/*     if (scan_strings_size != 2) { */
/*       buffer_file_puts (&text_out_buffer, "-> usage: unbind BID\n"); */
/*       return true; */
/*     } */
  
/*     bid_t bid = atoi (scan_strings[1]); */

/*     binding_t** ptr; */
/*     for (ptr = &bindings_head; *ptr != 0; ptr = &(*ptr)->next) { */
/*       if ((*ptr)->bid == bid) { */
/* 	break; */
/*       } */
/*     } */

/*     if (*ptr == 0) { */
/*       bfprintf (&text_out_buffer, "-> error: %s does not refer to a binding", scan_strings[1]); */
/*       return true; */
/*     } */

/*     unbind ((*ptr)->bid); */
/*     unbound_ (ptr); */

/*     return true; */
/*   } */
/*   return false; */
/* } */

/* static bool */
/* destroy_ (void) */
/* { */
/*   if (scan_strings_size >= 1 && strcmp (scan_strings[0], "destroy") == 0) { */
/*     if (scan_strings_size != 2) { */
/*       buffer_file_puts (&text_out_buffer, "-> usage: destroy NAME\n"); */
/*       return true; */
/*     } */
  
/*     automaton_t** aptr; */
/*     for (aptr = &automata_head; *aptr != 0; aptr = &(*aptr)->next) { */
/*       if (strcmp ((*aptr)->name, scan_strings[1]) == 0) { */
/* 	break; */
/*       } */
/*     } */

/*     if (*aptr == 0) { */
/*       bfprintf (&text_out_buffer, "-> error: %s does not name an automaton", scan_strings[1]); */
/*       return true; */
/*     } */

/*     destroy ((*aptr)->aid); */
/*     destroyed_ (aptr); */

/*     return true; */
/*   } */
/*   return false; */
/* } */

/* static void */
/* finda_found_callback (void* arg) */
/* { */

/* } */

/* static void */
/* find_usage (void) */
/* { */
/*   /\* TODO *\/ */
/*   logs (ERROR "TODO:  find_usage"); */
/*   /\* buffer_file_puts (&text_out_buffer, "-> usage: NAME = lookup NAME\n"); *\/ */
/* } */

/* static bool */
/* find_ (const string_t* strings, */
/*        size_t size) */
/* { */
/*   if (pstrcmp ("find", 0, strings[0].begin, strings[0].end) == 0) { */
/*     size_t local_name_idx; */
/*     size_t remote_name_idx; */

/*     size_t idx = 1; */

/*     /\* The local name should be next. *\/ */
/*     if (size > idx) { */
/*       local_name_idx = idx; */
/*       ++idx; */
/*     } */
/*     else { */
/*       find_usage (); */
/*       return true; */
/*     } */

/*     /\* The remove name should be next. *\/ */
/*     if (size > idx) { */
/*       remote_name_idx = idx; */
/*       ++idx; */
/*     } */
/*     else { */
/*       find_usage (); */
/*       return true; */
/*     } */

/*     if (find_automaton (strings[local_name_idx].begin, strings[local_name_idx].end) == 0) { */
/*       automaton_t* a = constellation_add_unmanaged_automaton (&constellation, -1); */
/*       finda_find (&finda, strings[remote_name_idx].begin, strings[remote_name_idx].end, finda_found_callback, a); */
/*       create_automaton (a, strings[local_name_idx].begin, strings[local_name_idx].end); */
/*     } */
/*     else { */
/*       logs (ERROR "TODO:  name already taken"); */
/*     } */

/*     return true; */
/*   } */

/*   return false; */

/* /\*   if (scan_strings_size >= 1) { *\/ */
/* /\*     if (strcmp ("lookup", scan_strings[0]) == 0) { *\/ */
/* /\*       lookup_usage (); *\/ */
/* /\*       return true; *\/ */
/* /\*     } *\/ */
/* /\*   } *\/ */
/* /\*   if (scan_strings_size >= 3 &&  *\/ */
/* /\*       strcmp ("=", scan_strings[1]) == 0 && *\/ */
/* /\*       strcmp ("lookup", scan_strings[2]) == 0) { *\/ */
/* /\*     if (scan_strings_size == 4) { *\/ */
/* /\*       if (find_automaton (scan_strings[0]) != 0) { *\/ */
/* /\* 	bfprintf (&text_out_buffer, "-> error: name %s is taken\n", scan_strings[0]); *\/ */
/* /\* 	return true; *\/ */
/* /\*       } *\/ */
      
/* /\*       /\\* Perform the lookup. *\\/ *\/ */
/* /\*       aid_t aid = lookups (scan_strings[3]); *\/ */
/* /\*       if (aid == -1) { *\/ */
/* /\* 	bfprintf (&text_out_buffer, "-> no automaton registered under %s\n", scan_strings[3]); *\/ */
/* /\* 	return true; *\/ */
/* /\*       } *\/ */
      
/* /\*       automaton_t* a; *\/ */
/* /\*       if ((a = find_automaton_aid (aid)) != 0) { *\/ */
/* /\* 	bfprintf (&text_out_buffer, "-> error: automaton already exists with name %s\n", a->name); *\/ */
/* /\* 	return true; *\/ */
/* /\*       } *\/ */
      
/* /\*       if (subscribe_destroyed (aid, DESTROYED_NO) != 0) { *\/ */
/* /\* 	buffer_file_puts (&text_out_buffer, "-> error: subscribe failed\n"); *\/ */
/* /\* 	return true; *\/ */
/* /\*       } *\/ */

/* /\*       /\\* Add to the list of automata we know about. *\\/ *\/ */
/* /\*       automaton_t* automaton = create_automaton (scan_strings[0], aid, scan_strings[3]); *\/ */
      
/* /\*       automaton->next = automata_head; *\/ */
/* /\*       automata_head = automaton; *\/ */

/* /\*       bfprintf (&text_out_buffer, "-> %s = %d\n", scan_strings[0], aid); *\/ */
/* /\*       return true; *\/ */
/* /\*     } *\/ */
/* /\*     else { *\/ */
/* /\*       lookup_usage (); *\/ */
/* /\*       return true; *\/ */
/* /\*     } *\/ */
/* /\*   } *\/ */
/*   return false; */
/* } */

/* static bool */
/* describe_ (void) */
/* { */
/*   if (scan_strings_size >= 1 && strcmp (scan_strings[0], "describe") == 0) { */
/*     if (scan_strings_size != 2) { */
/*       buffer_file_puts (&text_out_buffer, "-> usage: describe NAME\n"); */
/*       return true; */
/*     } */

/*     automaton_t* automaton = find_automaton (scan_strings[1]); */
/*     if (automaton == 0) { */
/*       bfprintf (&text_out_buffer, "-> error: name %s not recognized\n", scan_strings[1]); */
/*       return true; */
/*     } */

/*     description_t description; */
/*     if (description_init (&description, automaton->aid) != 0) { */
/*       buffer_file_puts (&text_out_buffer, "-> error: bad description\n"); */
/*       return true; */
/*     } */

/*     size_t action_count = description_action_count (&description); */
/*     if (action_count == -1) { */
/*       buffer_file_puts (&text_out_buffer, "-> error: bad description\n"); */
/*       description_fini (&description); */
/*       return true; */
/*     } */
    
/*     action_desc_t* actions = malloc (action_count * sizeof (action_desc_t)); */
/*     if (description_read_all (&description, actions) != 0) { */
/*       buffer_file_puts (&text_out_buffer, "-> error: bad description\n"); */
/*       description_fini (&description); */
/*       free (actions); */
/*       return true; */
/*     } */

/*     bfprintf (&text_out_buffer, "-> aid=%d path=%s\n", automaton->aid, automaton->path); */
    
/*     /\* Print the actions. *\/ */
/*     for (size_t action = 0; action != action_count; ++action) { */

/*       buffer_file_puts (&text_out_buffer, "-> "); */

/*       switch (actions[action].type) { */
/*       case LILY_ACTION_INPUT: */
/* 	buffer_file_puts (&text_out_buffer, " IN "); */
/* 	break; */
/*       case LILY_ACTION_OUTPUT: */
/* 	buffer_file_puts (&text_out_buffer, "OUT "); */
/* 	break; */
/*       case LILY_ACTION_INTERNAL: */
/* 	buffer_file_puts (&text_out_buffer, "INT "); */
/* 	break; */
/*       case LILY_ACTION_SYSTEM_INPUT: */
/* 	buffer_file_puts (&text_out_buffer, "SYS "); */
/* 	break; */
/*       default: */
/* 	buffer_file_puts (&text_out_buffer, "??? "); */
/* 	break; */
/*       } */
      
/*       switch (actions[action].parameter_mode) { */
/*       case NO_PARAMETER: */
/* 	buffer_file_puts (&text_out_buffer, "  "); */
/* 	break; */
/*       case PARAMETER: */
/* 	buffer_file_puts (&text_out_buffer, "* "); */
/* 	break; */
/*       case AUTO_PARAMETER: */
/* 	buffer_file_puts (&text_out_buffer, "@ "); */
/* 	break; */
/*       default: */
/* 	buffer_file_puts (&text_out_buffer, "? "); */
/* 	break; */
/*       } */
      
/*       bfprintf (&text_out_buffer, "%d\t%s\t%s\n", actions[action].number, actions[action].name, actions[action].description); */
/*     } */
    
/*     free (actions); */
    
/*     description_fini (&description); */

/*     /\* Print the input bindings. *\/ */
/*     for (binding_t* binding = bindings_head; binding != 0; binding = binding->next) { */
/*       if (binding->input_automaton == automaton) { */
/* 	bfprintf (&text_out_buffer, "-> %d: (%s, %s, %d) -> (%s, %s, %d)\n", binding->bid, binding->output_automaton->name, binding->output_action_name, binding->output_parameter, binding->input_automaton->name, binding->input_action_name, binding->input_parameter); */
/*       } */
/*     } */

/*     for (binding_t* binding = bindings_head; binding != 0; binding = binding->next) { */
/*       if (binding->output_automaton == automaton) { */
/* 	bfprintf (&text_out_buffer, "-> %d: (%s, %s, %d) -> (%s, %s, %d)\n", binding->bid, binding->output_automaton->name, binding->output_action_name, binding->output_parameter, binding->input_automaton->name, binding->input_action_name, binding->input_parameter); */
/*       } */
/*     } */
    
/*     return true; */
/*   } */
/*   return false; */
/* } */

/* static bool */
/* list_ (void) */
/* { */
/*   if (scan_strings_size >= 1) { */
/*     if (strcmp ("list", scan_strings[0]) == 0) { */
/*       if (scan_strings_size == 1) { */

/* 	for (automaton_t* automaton = automata_head; automaton != 0; automaton = automaton->next) { */
/*  	  bfprintf (&text_out_buffer, "-> %s\n", automaton->name); */
/* 	} */
	
/* 	return true; */
/*       } */
/*       else { */
/* 	buffer_file_puts (&text_out_buffer, "-> usage: list\n"); */
/* 	return true; */
/*       } */
/*     } */
/*   } */
/*   return false; */
/* } */

/* static bool */
/* com_ (void) */
/* { */
/*   if (scan_strings_size >= 1 && strcmp (scan_strings[0], "com") == 0) { */
/*     if (scan_strings_size < 3) { */
/*       buffer_file_puts (&text_out_buffer, "-> usage: com ID ARGS...\n"); */
/*       return true; */
/*     } */

/*     automaton_t* automaton = find_automaton (scan_strings[1]); */
/*     if (automaton == 0) { */
/*       bfprintf (&text_out_buffer, "-> warning: automaton %s\n", scan_strings[1]); */
/*       return true; */
/*     } */

/*     /\* Check if we are already bound. *\/ */
/*     binding_t* b; */
/*     for (b = bindings_head; b != 0; b = b->next) { */
/*       if (b->output_automaton == this_automaton && */
/* 	  b->input_automaton == automaton && */
/* 	  b->output_action == COM_OUT_NO) { */
/* 	break; */
/*       } */
/*     } */
/*     if (b == 0) { */
/*       /\* Bind to com. *\/ */
/*       single_bind (this_automaton, "com_out", 0, automaton, "com_in", 0); */
/*     } */

/*     for (b = bindings_head; b != 0; b = b->next) { */
/*       if (b->output_automaton == automaton && */
/* 	  b->input_automaton == this_automaton && */
/* 	  b->input_action == COM_IN_NO) { */
/* 	break; */
/*       } */
/*     } */
/*     if (b == 0) { */
/*       /\* Bind to com. *\/ */
/*       single_bind (automaton, "com_out", 0, this_automaton, "com_in", 0); */
/*     } */
	
/*     /\* Add to the COM queue. *\/ */
/*     com_queue_push (automaton->aid, current_line + (scan_strings[2] - scan_string_copy)); */

/*     return true; */
/*   } */
/*   return false; */
/* } */

static bool
error_ (const string_t* strings,
	size_t size)
{
  /* TODO */
  logs ("error_");
  //exit (-1);
  /* if (scan_strings_size != 0) { */
  /*   /\* Catch all. *\/ */
  /*   bfprintf (&text_out_buffer, "-> error: unknown command %s\n", scan_strings[0]); */
  /* } */
  return true;
}

typedef bool (*dispatch_func_t) (const string_t* strings,
				 size_t size);

static dispatch_func_t dispatch[] = {
  create_,
  /* bind_, */
  /* unbind_, */
  /* destroy_, */
  /* find_, */
  /* describe_, */
  /* list_, */
  /* com_, */
  /* Catch errors. */
  error_,
  0
};

/*
  End Dispatch Section
  ====================
*/

/*
  Begin Scanner Section
  =====================
*/

typedef enum {
  SCAN_START,
  SCAN_STRING,
  SCAN_COMMENT,
  SCAN_ACCEPT,
} scan_state_t;

static scan_state_t scan_state;
static const char* scan_string_begin;
static string_t* scan_strings;
static size_t scan_strings_size = 0;
static size_t scan_strings_capacity = 0;

/* Tokens

   string - [a-zA-Z0-9_/-=*?!]+

   whitespace - [ \t\0]

   comment - #[^\n]*
 */

static void
scanner_reset (void)
{
  scan_state = SCAN_START;
  scan_string_begin = 0;
  scan_strings_size = 0;

}
static void
scanner_push_string (const char* end)
{
  if (scan_strings_size == scan_strings_capacity) {
    scan_strings_capacity = 2 * scan_strings_capacity + 1;
    scan_strings = realloc (scan_strings, scan_strings_capacity * sizeof (string_t));
  }
  scan_strings[scan_strings_size].begin = scan_string_begin;
  scan_strings[scan_strings_size].end = end;
  ++scan_strings_size;
  scan_string_begin = 0;
}

static void
scanner_put (const char* ptr)
{
  const char c = *ptr;

  switch (scan_state) {
  case SCAN_START:
    if (c == '\n') {
      scan_state = SCAN_ACCEPT;
    }
    else if (c == '#') {
      scan_state = SCAN_COMMENT;
    }
    else if (isalnum (c) || ispunct (c)) {
      scan_string_begin = ptr;
      scan_state = SCAN_STRING;
    }
    else {
      scan_state = SCAN_START;
    }
    break;
  case SCAN_STRING:
    if (c == '\n') {
      scanner_push_string (ptr);
      scan_state = SCAN_ACCEPT;
    }
    else if (c == '#') {
      scanner_push_string (ptr);
      scan_state = SCAN_COMMENT;
    }
    else if (isalnum (c) || ispunct (c)) {
      scan_state = SCAN_STRING;
    }
    else {
      scanner_push_string (ptr);
      scan_state = SCAN_START;
    }
    break;
  case SCAN_COMMENT:
    if (c == '\n') {
      scan_state = SCAN_ACCEPT;
    }
    /* Otherwise, stay in SCAN_COMMENT. */
    break;
  case SCAN_ACCEPT:
    /* Do nothing. */
    break;
  }
}

static void
scanner_finalize (const char* end)
{
  if (scan_state == SCAN_STRING) {
    /* We were in the middle of a string. */
    scanner_push_string (end);
  }
  scan_state = SCAN_ACCEPT;
}

static int
process (void)
{
  if (scan_strings_size != 0) {
    for (size_t idx = 0; dispatch[idx] != 0; ++idx) {
      if (dispatch[idx] (scan_strings, scan_strings_size)) {
	break;
      }
    }
  }
  return 0;
}

static void
interpret (const char* begin,
	   size_t size)
{
  scanner_reset ();
  const char* end = begin + size;
  for (const char* ptr = begin; ptr != end; ++ptr) {
    scanner_put (ptr);
    if (scan_state == SCAN_ACCEPT) {
      if (process () != 0) {
	/* Error. */
	return;
      }
      scanner_reset ();
    }
  }
  scanner_finalize (end);
  if (process () != 0) {
    /* Error. */
    return;
  }
}

/*
  End Scanner Section
  ===================
*/

static void
readscript_callback (void* arg,
		     const fs_readfile_response_t* res,
		     bd_t bd)
{
  switch (res->error) {
  case FS_READFILE_SUCCESS:
    {
      const char* str = buffer_map (bd);
      if (str == 0) {
	snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not map script: %s", lily_error_string (lily_error));
	logs (log_buffer);
	exit (-1);
      }
      interpret (str, res->size);
      buffer_unmap (bd);
    }
    break;
  case FS_READFILE_BAD_START:
  case FS_READFILE_NODE_DNE:
  case FS_READFILE_NOT_FILE:
    logs (TODO "readscript failed");
    break;
  }
}

BEGIN_INPUT (NO_PARAMETER, INIT_NO, SA_INIT_IN_NAME, "", init, ano_t ano, int param, bd_t bda, bd_t bdb)
{
  if (!initialized) {
    initialized = true;

    /* aid_t finda_aid = -1; */

    output_bda = buffer_create (0);
    output_bdb = buffer_create (0);
    if (output_bda == -1 || output_bdb == -1) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not create output buffer: %s\n", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
    if (buffer_file_initw (&output_bfa, output_bda) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not initialize output buffer: %s\n", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }

    if (system_init (&system) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not initialize system");
      logs (log_buffer);
      exit (-1);
    }
    fs_set_init (&fs_set, &system, &output_bfa, FS_DESCEND_REQUEST_OUT_NO, FS_DESCEND_RESPONSE_IN_NO, FS_READFILE_REQUEST_OUT_NO, FS_READFILE_RESPONSE_IN_NO);

    //this_automaton = create_automaton (system_get_this (&system), "this", 0);

    if (bda != -1) {
      buffer_file_t de_buffer;
      if (buffer_file_initr (&de_buffer, bda) != 0) {
      	snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not initialize de buffer: %s\n", lily_error_string (lily_error));
      	logs (log_buffer);
      	exit (-1);
      }

      de_val_t* root = de_deserialize (&de_buffer);
      if (root == 0) {
      	snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not parse de buffer: %s\n", lily_error_string (lily_error));
      	logs (log_buffer);
      	exit (-1);
      }

      de_val_t* fs = de_get (root, "." FS);
      if (de_type (fs) == DE_ARRAY) {
      	for (size_t idx = 0; idx != de_array_size (fs); ++idx) {
      	  de_val_t* entry = de_array_at (fs, idx);

    	  aid_t aid = de_integer_val (de_get (entry, ".aid"), -1);
    	  const char* name = de_string_val (de_get (entry, ".name"), 0);
    	  fs_nodeid_t nodeid = de_integer_val (de_get (entry, ".nodeid"), -1);

    	  if (aid != -1 && name != 0 && nodeid != -1) {
    	    fs_set_insert (&fs_set, aid, name, nodeid);
    	  }
      	}
      }

      aid_t active_fs = de_integer_val (de_get (root, "." ACTIVE_FS), -1);
      if (active_fs != -1) {
    	fs_set_make_active (&fs_set, active_fs);
      }

      const char* script_name = de_string_val (de_get (root, "." ARGS "." "script"), 0);
      if (script_name != 0) {
    	fs_set_readfile (&fs_set, script_name, script_name + strlen (script_name), readscript_callback, 0);
      }

      /*   finda_aid = de_integer_val (de_get (root, "." FINDA), -1); */

      de_destroy (root);
    }

    /* finda_init (&finda, &constellation, finda_aid, SEND_NO, RECV_NO); */

    /* text_out_bd = buffer_create (0); */
    /* if (text_out_bd == -1) { */
    /*   snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not create text_out buffer: %s", lily_error_string (lily_error)); */
    /*   logs (log_buffer); */
    /*   exit (-1); */
    /* } */
    /* if (buffer_file_initw (&text_out_buffer, text_out_bd) != 0) { */
    /*   snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not initialize text_out buffer: %s", lily_error_string (lily_error)); */
    /*   logs (log_buffer); */
    /*   exit (-1); */
    /* } */

    /* com_out_bd = buffer_create (0); */
    /* if (com_out_bd == -1) { */
    /*   snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not create com_out buffer: %s", lily_error_string (lily_error)); */
    /*   logs (log_buffer); */
    /*   exit (-1); */
    /* } */
    /* if (buffer_file_initw (&com_out_buffer, com_out_bd) != 0) { */
    /*   snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not initialize com_out buffer: %s", lily_error_string (lily_error)); */
    /*   logs (log_buffer); */
    /*   exit (-1); */
    /* } */
  }

  finish_input (bda, bdb);
}

/* BEGIN_INPUT (NO_PARAMETER, TEXT_IN_NO, "text_in", "buffer_file_t", text_in, ano_t ano, int param, bd_t bda, bd_t bdb) */
/* { */
/*   initialize (); */

/*   buffer_file_t input_buffer; */
/*   if (buffer_file_initr (&input_buffer, bda) != 0) { */
/*     finish_input (bda, bdb); */
/*   } */

/*   size_t size = buffer_file_size (&input_buffer); */
/*   const char* str = buffer_file_readp (&input_buffer, size); */
/*   if (str == 0) { */
/*     finish_input (bda, bdb); */
/*   } */

/*   char* str_copy = malloc (size); */
/*   memcpy (str_copy, str, size); */

/*   string_push_back (str_copy, size); */

/*   finish_input (bda, bdb); */
/* } */

/* static bool */
/* process_line_precondition (void) */
/* { */
/*   return !process_hold && !string_empty () && com_queue_empty () && callback_queue_empty (&vfs_response_queue); */
/* } */

/* BEGIN_INTERNAL (NO_PARAMETER, PROCESS_LINE_NO, "", "", process_line, ano_t ano, int param) */
/* { */
/*   initialize (); */

/*   if (process_line_precondition ()) { */
/*     char* current_string; */
/*     size_t current_string_size; */
/*     string_front (&current_string, &current_string_size); */

/*     for (; current_string_idx != current_string_size; ++current_string_idx) { */
/*       const char c = current_string[current_string_idx]; */
/*       if (c >= ' ' && c < 127) { */
/* 	/\* Printable character. *\/ */
/* 	line_append (c); */
/* 	buffer_file_put (&text_out_buffer, c); */
/*       } */
/*       else { */
/* 	/\* Control character. *\/ */
/* 	switch (c) { */
/* 	case '\b': */
/* 	  line_back (); */
/* 	  buffer_file_put (&text_out_buffer, '\b'); */
/* 	  buffer_file_put (&text_out_buffer, ' '); */
/* 	  buffer_file_put (&text_out_buffer, '\b'); */
/* 	  break; */
/* 	case '\n': */
/* 	  /\* Terminate. *\/ */
/* 	  line_append (0); */
/* 	  buffer_file_put (&text_out_buffer, '\n'); */

/* 	  if (scanner_process (current_line, current_line_size) == 0) { */
/* 	    /\* Scanning succeeded.  Interpret. *\/ */
/* 	    for (size_t idx = 0; dispatch[idx] != 0; ++idx) { */
/* 	      if (dispatch[idx] ()) { */
/* 		break; */
/* 	      } */
/* 	    } */
/* 	  } */

/* 	  /\* Reset the line. *\/ */
/* 	  line_reset (); */

/* 	  ++current_string_idx; */

/* 	  /\* Done. *\/ */
/* 	  finish_internal (); */

/* 	  break; */
/* 	} */
/*       } */
/*     } */

/*     string_pop (); */
/*     free (current_string); */
/*     current_string_idx = 0; */
/*   } */
/*   finish_internal (); */
/* } */

/* static bool */
/* text_out_precondition (void) */
/* { */
/*   return buffer_file_size (&text_out_buffer) != 0; */
/* } */

/* BEGIN_OUTPUT (NO_PARAMETER, TEXT_OUT_NO, "text_out", "buffer_file_t", text_out, ano_t ano, int param, size_t bc) */
/* { */
/*   initialize (); */

/*   if (text_out_precondition ()) { */
/*     buffer_file_truncate (&text_out_buffer); */
/*     finish_output (true, text_out_bd, -1); */
/*   } */
/*   else { */
/*     finish_output (false, -1, -1); */
/*   } */
/* } */

/* BEGIN_SYSTEM_INPUT (DESTROYED_NO, "", "", destroyed, ano_t ano, aid_t aid, bd_t bda, bd_t bdb) */
/* { */
/*   initialize (); */
  
/*   automaton_t** aptr; */
/*   for (aptr = &automata_head; *aptr != 0; aptr = &(*aptr)->next) { */
/*     if ((*aptr)->aid == aid) { */
/*       break; */
/*     } */
/*   } */

/*   destroyed_ (aptr); */

/*   finish_input (bda, bdb); */
/* } */

/* BEGIN_SYSTEM_INPUT (UNBOUND_NO, "", "", unbound, ano_t ano, bid_t bid, bd_t bda, bd_t bdb) */
/* { */
/*   initialize (); */

/*   binding_t** ptr; */
/*   for (ptr = &bindings_head; *ptr != 0; ptr = &(*ptr)->next) { */
/*     if ((*ptr)->bid == bid) { */
/*       break; */
/*     } */
/*   } */

/*   unbound_ (ptr); */

/*   finish_input (bda, bdb); */
/* } */

/* BEGIN_INPUT (NO_PARAMETER, RECV_NO, "", "", recv, ano_t ano, int param, bd_t bda, bd_t bdb) */
/* { */
/*   initialize (); */
/*   finda_recv (&finda, bda, bdb); */
/* } */

/* BEGIN_OUTPUT (NO_PARAMETER, SEND_NO, "", "", send, ano_t ano, int param) */
/* { */
/*   initialize (); */
/*   finda_send (&finda); */
/* } */

/* BEGIN_OUTPUT (AUTO_PARAMETER, COM_OUT_NO, "com_out", "", com_out, ano_t ano, aid_t aid, size_t bc) */
/* { */
/*   initialize (); */

/*   if (!com_queue_empty () && aid == com_queue_front ()) { */
/*     buffer_file_truncate (&com_out_buffer); */
/*     buffer_file_puts (&com_out_buffer, com_head->str); */
/*     com_queue_pop (); */
/*     finish_output (true, com_out_bd, -1); */
/*   } */
/*   finish_output (false, -1, -1); */
/* } */

/* BEGIN_INPUT (AUTO_PARAMETER, COM_IN_NO, "com_in", "", com_in, ano_t ano, int param, bd_t bda, bd_t bdb) */
/* { */
/*   initialize (); */

/*   buffer_file_t bf; */
/*   if (buffer_file_initr (&bf, bda) != 0) { */
/*     finish_input (bda, bdb); */
/*   } */

/*   size_t size = buffer_file_size (&bf); */
/*   if (size == 0) { */
/*     finish_input (bda, bdb); */
/*   } */
/*   const char* begin = buffer_file_readp (&bf, size); */
/*   if (begin == 0) { */
/*     finish_input (bda, bdb); */
/*   } */

/*   buffer_file_write (&text_out_buffer, begin, size); */

/*   finish_input (bda, bdb); */
/* } */

BEGIN_SYSTEM (NO_PARAMETER, BIND_ACTION_NO, "", "", bind_action, ano_t ano, int param)
{
  //system_bind_action (&system);
}

BEGIN_OUTPUT (AUTO_PARAMETER, INIT_OUT_NO, SA_INIT_OUT_NAME, "", init_out, ano_t ano, aid_t aid)
{
  //system_init_out (&system, aid);
}

BEGIN_OUTPUT (AUTO_PARAMETER, STATUS_REQUEST_OUT_NO, SA_STATUS_REQUEST_OUT_NAME, "", status_request_out, ano_t ano, aid_t aid)
{
  //system_status_request_out (&system, aid);
}

BEGIN_INPUT (AUTO_PARAMETER, STATUS_REQUEST_IN_NO, SA_STATUS_REQUEST_IN_NAME, "", status_request_in, ano_t ano, aid_t aid, bd_t bda, bd_t bdb)
{
  //system_status_request_in (&system, aid, bda, bdb);
}

BEGIN_OUTPUT (AUTO_PARAMETER, STATUS_RESPONSE_OUT_NO, SA_STATUS_RESPONSE_OUT_NAME, "", status_response_out, ano_t ano, aid_t aid)
{
  //system_status_response_out (&system, aid);
}

BEGIN_INPUT (AUTO_PARAMETER, STATUS_RESPONSE_IN_NO, SA_STATUS_RESPONSE_IN_NAME, "", status_response_in, ano_t ano, aid_t aid, bd_t bda, bd_t bdb)
{
  //system_status_response_in (&system, aid, bda, bdb);
}

BEGIN_OUTPUT (AUTO_PARAMETER, BINDING_UPDATE_OUT_NO, SA_BINDING_UPDATE_OUT_NAME, "", binding_update_out, ano_t ano, aid_t aid)
{
  //system_binding_update_out (&system, aid);
}

BEGIN_INPUT (AUTO_PARAMETER, BINDING_UPDATE_IN_NO, SA_BINDING_UPDATE_IN_NAME, "", binding_update_in, ano_t ano, aid_t aid, bd_t bda, bd_t bdb)
{
  //system_binding_update_in (&system, aid, bda, bdb);
}

BEGIN_OUTPUT (AUTO_PARAMETER, FS_DESCEND_REQUEST_OUT_NO, "", "", fs_descend_request_out, ano_t ano, aid_t aid)
{
  fs_set_descend_request (&fs_set, aid);
}

BEGIN_INPUT (AUTO_PARAMETER, FS_DESCEND_RESPONSE_IN_NO, "", "", fs_descend_response_in, ano_t ano, aid_t aid, bd_t bda, bd_t bdb)
{
  fs_set_descend_response (&fs_set, aid, bda, bdb);
}

BEGIN_OUTPUT (AUTO_PARAMETER, FS_READFILE_REQUEST_OUT_NO, "", "", fs_readfile_request_out, ano_t ano, aid_t aid)
{
  fs_set_readfile_request (&fs_set, aid);
}

BEGIN_INPUT (AUTO_PARAMETER, FS_READFILE_RESPONSE_IN_NO, "", "", fs_readfile_response_in, ano_t ano, aid_t aid, bd_t bda, bd_t bdb)
{
  fs_set_readfile_response (&fs_set, aid, bda, bdb);
}

void
do_schedule (void)
{
  /* if (process_line_precondition ()) { */
  /*   schedule (PROCESS_LINE_NO, 0); */
  /* } */
  /* if (text_out_precondition ()) { */
  /*   schedule (TEXT_OUT_NO, 0); */
  /* } */
  /* if (!com_queue_empty ()) { */
  /*   schedule (COM_OUT_NO, com_queue_front ()); */
  /* } */
  system_schedule (&system);
  fs_set_schedule (&fs_set);
  /* finda_schedule (&finda); */
}
