#include "consumer.h"
#include <automaton.h>
#include <io.h>
#include <dymem.h>
#include <string.h>

#define VIDEO_RAM 0xB8000
#define ATTRIBUTE ((0 << 12) | (0xF << 8))
#define HEIGHT 25
#define WIDTH 80

static bool initialized = false;
  
typedef struct display_page display_page_t;
typedef struct client_context client_context_t;

struct display_page {
  unsigned short* buffer;
};

struct client_context {
  aid_t aid;
  bd_t bd;
  unsigned short* buffer;
  display_page_t* display_page;
  size_t x_location;
  size_t y_location;
  client_context_t* next;
};

static client_context_t* context_list_head = 0;
static display_page_t dp;

static void
clear_buffer (unsigned short* buffer)
{
  for (size_t idx = 0; idx < WIDTH * HEIGHT; ++idx) {
    buffer[idx] = ATTRIBUTE | ' ';
  }
}

static client_context_t*
create_client_context (aid_t aid)
{
  client_context_t* context = malloc (sizeof (client_context_t));
  context->aid = aid;
  context->bd = buffer_create (2 * WIDTH * HEIGHT);
  context->buffer = buffer_map (context->bd);
  clear_buffer (context->buffer);
  context->display_page = &dp; //0;
  context->x_location = 0;
  context->y_location = 0;
  context->next = 0;
  return context;
}

static void
destroy_client_context (client_context_t* context)
{
  buffer_destroy (context->bd);
  free (context);
}

static client_context_t*
find_client_context (aid_t aid)
{
  client_context_t* ptr;
  for (ptr = context_list_head; ptr != 0 && ptr->aid != aid; ptr = ptr->next) ;;
  return ptr;
}

static void
scroll_buffer (client_context_t* context,
	       unsigned short* buffer,
	       size_t lines)
{
  if (lines != 0) {
    /* Scroll. */
    memmove (&buffer[0], &buffer[lines * WIDTH], 2 * WIDTH * (HEIGHT - lines));
    context->y_location -= lines;
    /* Fill with spaces. */
    for (size_t y = context->y_location + 1; y != HEIGHT; ++y) {
      for (size_t x = 0; x != WIDTH; ++x) {
	context->buffer[y * WIDTH + x] = ATTRIBUTE | ' ';
      }
    }
  }
}

/* Works like a circular buffer. */
static size_t
print_on_buffer (size_t* x_location,
		 size_t* y_location,
		 unsigned short* buffer,
		 size_t line_limit,
		 const char* string,
		 size_t length,
		 const char** mark)
{
  size_t lines = 0;
  size_t x_loc = *x_location;
  size_t y_loc = *y_location;

  for (; length != 0 && lines != line_limit; --length, ++string) {
    switch (*string) {
    case '\b':
      if (x_loc > 0) {
	--x_loc;
      }
      break;
    case '\t':
      /* A tab is a position divisible by 8. */
      x_loc = (x_loc + 8) & ~(8-1);
      break;
    case '\n':
      x_loc = 0;
      ++y_loc;
      ++lines;
      break;
    case '\r':
      x_loc = 0;
      break;
    default:
      /* Print the character using black on white. */
      if (buffer != 0) {
	buffer[y_loc * WIDTH + x_loc] = ATTRIBUTE | *string;
      }
      /* Advance the cursor. */
      ++x_loc;
      if (x_loc == WIDTH) {
	x_loc = 0;
	++y_loc;
	++lines;
      }
    }
  }

  *x_location = x_loc;
  *y_location = y_loc;

  if (mark != 0) {
    *mark = string;
  }
  return lines;
}

static void
print_on_context (client_context_t* context,
		  const char* string,
		  size_t length)
{
  /* Pick a buffer. */
  unsigned short* output_buffer = context->buffer;
  if (context->display_page != 0) {
    output_buffer = context->display_page->buffer;
  }

  /* Count the lines. */
  size_t x_loc = context->x_location;
  size_t y_loc = context->y_location;
  size_t lines = print_on_buffer (&x_loc, &y_loc, 0, -1, string, length, 0);

  if (context->y_location + lines < HEIGHT) {
    /* There is enough space, just print. */
    print_on_buffer (&context->x_location, &context->y_location, output_buffer, -1, string, length, 0);
  }
  else if (lines < HEIGHT) {
    /*
      There will be enough space if we scroll.
      y_location - scroll + lines == HEIGHT - 1 
      scroll == y_location + lines - (HEIGHT - 1)
     */
    scroll_buffer (context, output_buffer, context->y_location + lines - (HEIGHT - 1));
    print_on_buffer (&context->x_location, &context->y_location, output_buffer, -1, string, length, 0);
  }
  else {
    /* The entire buffer will be discarded. */

    /* Clear the buffer. */
    clear_buffer (output_buffer);

    /* Simulate the first (lines - HEIGHT) lines. */
    const char* ptr;
    print_on_buffer (&context->x_location, &context->y_location, 0, lines - HEIGHT + 1, string, length, &ptr);

    context->y_location = 0;

    /* Print the remaining lines. */
    print_on_buffer (&context->x_location, &context->y_location, output_buffer, -1, ptr, length - (ptr - string), 0);
  }
}

static void
initialize (void)
{
  if (!initialized) {

    dp.buffer = (unsigned short*)VIDEO_RAM;

    // Memory map the video ram.
    // Each element is two bytes corresponding to the character and attribute.
    map (dp.buffer, dp.buffer, WIDTH * HEIGHT * 2);

    // Clear the screen.
    clear_buffer (dp.buffer);
    
    initialized = true;
  }
}

void
init (void)
{
  initialize ();
  finish (NO_ACTION, 0, -1, 0, 0);
}
EMBED_ACTION_DESCRIPTOR (INTERNAL, NO_PARAMETER, LILY_ACTION_INIT, init);

void
focus (void)
{
  initialize ();
  finish (NO_ACTION, 0, -1, 0, 0);
}
EMBED_ACTION_DESCRIPTOR (INPUT, NO_PARAMETER, CONSUMER_FOCUS, focus);

void
print (aid_t aid,
       const char* string,
       size_t length)
{
  initialize ();
  
  /* if (binding_count (CONSUMER_PRINT_SENSE, (const void*)aid) != 0) { */
  /*   client_context_t* context = find_client_context (aid); */
  /*   if (context == 0) { */
  /*     context = create_client_context (aid); */
  /*   } */

  /*   if (string != 0 && length != 0) { */
  /*     print_on_context (context, string, length); */
  /*   } */
  /* } */
  
  finish (NO_ACTION, 0, -1, 0, 0);
}
EMBED_ACTION_DESCRIPTOR (INPUT, AUTO_PARAMETER, CONSUMER_PRINT, print);

/* void */
/* print_sense (aid_t aid) */
/* { */
/*   initialize (); */

/*   size_t count = binding_count (CONSUMER_PRINT_SENSE, (const void*)aid); */
/*   if (count != 0) { */
/*     /\* Create client context if necessary. *\/ */
/*     if (find_client_context (aid) == 0) { */
/*       client_context_t* context = create_client_context (aid); */
/*       context->next = context_list_head; */
/*       context_list_head = context; */
/*     } */
/*   } */
/*   else { */
/*     /\* Destroy client context if necessary. *\/ */
/*     client_context_t** ptr = &context_list_head; */
/*     for (; *ptr != 0 && (*ptr)->aid != aid; ptr = &(*ptr)->next) ;; */
/*     if (*ptr != 0) { */
/*       client_context_t* temp = *ptr; */
/*       *ptr = temp->next; */
/*       destroy_client_context (temp); */
/*     } */
/*   } */

/*   finish (NO_ACTION, 0, -1, 0, 0); */
/* } */
/* EMBED_ACTION_DESCRIPTOR (OUTPUT, AUTO_PARAMETER, CONSUMER_PRINT_SENSE, print_sense); */
