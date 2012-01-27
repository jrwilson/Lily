#include "consumer.h"
#include <automaton.h>
#include <io.h>
#include <dymem.h>
#include <string.h>

#define ATTRIBUTE ((0 << 12) | (0xF << 8))
#define HEIGHT 25
#define WIDTH 80

static bool initialized = false;

static unsigned short* video_ram = (unsigned short*)0xB8000;

/* static void */
/* put (char c) */
/* { */
/*   // TODO:  Can we scroll with hardware?  Use REP MOVS. */
/*   /\* Scroll if we are at the bottom of the screen. *\/ */
/*   if (y_location == HEIGHT) { */
/*     unsigned int y; */
/*     for (y = 0; y < HEIGHT - 1; ++y) { */
/*       for (unsigned int x = 0; x < WIDTH; ++x) { */
/* 	video_ram[y * WIDTH + x] = video_ram[(y + 1) * WIDTH + x]; */
/*       } */
/*     } */
/*     /\* Fill last line with spaces. *\/ */
/*     for (unsigned int x = 0; x < WIDTH; ++x) { */
/*       video_ram[y * WIDTH + x] = ATTRIBUTE | ' '; */
/*     } */
/*     --y_location; */
/*   } */
  
/*   switch (c) { */
/*   case '\b': */
/*     if (x_location > 0) { */
/*       --x_location; */
/*     } */
/*     break; */
/*   case '\t': */
/*     /\* A tab is a position divisible by 8. *\/ */
/*     x_location = (x_location + 8) & ~(8-1); */
/*     break; */
/*   case '\n': */
/*     x_location = 0; */
/*     ++y_location; */
/*     break; */
/*   case '\r': */
/*     x_location = 0; */
/*     break; */
/*   default: */
/*     /\* Print the character using black on white. *\/ */
/*     video_ram[y_location * WIDTH + x_location] = ATTRIBUTE | c; */
/*     /\* Advance the cursor. *\/ */
/*     ++x_location; */
/*     if (x_location == WIDTH) { */
/*       ++y_location; */
/*       x_location = 0; */
/*     } */
/*   } */
/* } */
  
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

static client_context_t*
create_client_context (aid_t aid)
{
  client_context_t* context = malloc (sizeof (client_context_t));
  context->aid = aid;
  context->bd = buffer_create (2 * WIDTH * HEIGHT);
  context->buffer = buffer_map (context->bd);
  for (size_t idx = 0; idx < WIDTH * HEIGHT; ++idx) {
    context->buffer[idx] = ATTRIBUTE | ' ';
  }
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

static size_t
count_lines (client_context_t* context,
	     const char* string,
	     size_t length)
{
  size_t x_location = context->x_location;
  size_t lines = 0;

  for (; length != 0; --length, ++string) {
    switch (*string) {
    case '\b':
      if (x_location > 0) {
	--x_location;
      }
      break;
    case '\t':
      /* A tab is a position divisible by 8. */
      x_location = (x_location + 8) & ~(8-1);
      break;
    case '\n':
      x_location = 0;
      ++lines;
      break;
    case '\r':
      x_location = 0;
      break;
    default:
      /* Advance the cursor. */
      ++x_location;
      if (x_location == WIDTH) {
	++lines;
	x_location = 0;
      }
    }
  }

  return lines;
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
static void
print_on_buffer (client_context_t* context,
		 unsigned short* buffer,
		 const char* string,
		 size_t length)
{
  for (; length != 0; --length, ++string) {
    switch (*string) {
    case '\b':
      if (context->x_location > 0) {
	--context->x_location;
      }
      break;
    case '\t':
      /* A tab is a position divisible by 8. */
      context->x_location = (context->x_location + 8) & ~(8-1);
      break;
    case '\n':
      context->x_location = 0;
      ++context->y_location;
      break;
    case '\r':
      context->x_location = 0;
      break;
    default:
      /* Print the character using black on white. */
      buffer[context->y_location * WIDTH + context->x_location] = ATTRIBUTE | *string;
      /* Advance the cursor. */
      ++context->x_location;
      if (context->x_location == WIDTH) {
	context->x_location = 0;
	++context->y_location;
      }
    }
  }
}

static void
fill (client_context_t* context)
{
  size_t x_location = context->x_location;
  size_t y_location = context->y_location;

  for (size_t x_location = context->x_location; x_location != WIDTH; ++x_location) {
    context->buffer[y_location * WIDTH + x_location] = ATTRIBUTE | ' ';
  }
  ++y_location;

  for (; y_location != HEIGHT; ++y_location) {
    for (x_location = 0; x_location != WIDTH; ++x_location) {
      context->buffer[y_location * WIDTH + x_location] = ATTRIBUTE | ' ';
    }
  }
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
  size_t lines = count_lines (context, string, length);

  if (context->y_location + lines < HEIGHT) {
    /* There is enough space, just print. */
    print_on_buffer (context, output_buffer, string, length);
  }
  else if (lines < HEIGHT) {
    /*
      There will be enough space if we scroll.
      y_location - scroll + lines == HEIGHT - 1 
      scroll == y_location + lines - (HEIGHT - 1)
     */
    scroll_buffer (context, output_buffer, context->y_location + lines - (HEIGHT - 1));
    print_on_buffer (context, output_buffer, string, length);
  }
  else {
    /* The entire buffer will be discarded. */
    /* Reset. */
  }

  /* We need to adjust the y_location so that: */
  /*    (y_location + adjust + lines == HEIGHT - 1) mod HEIGHT */
  /*    (adjust == HEIGHT - 1 - y_location - lines) mod HEIGHT */
  /* size_t adjust = (((HEIGHT - 1 - (int)context->y_location - (int)lines) % HEIGHT) + HEIGHT) % HEIGHT; */

  /* /\* Print the text. *\/ */

  /* /\* Fill remainder with spaces. *\/ */
  /* fill (context); */
}

static void
initialize (void)
{
  if (!initialized) {
    // Memory map the video ram.
    // Each element is two bytes corresponding to the character and attribute.
    map (video_ram, video_ram, WIDTH * HEIGHT * 2);

    // Clear the screen.
    for (size_t idx = 0; idx < WIDTH * HEIGHT; ++idx) {
      video_ram[idx] = ATTRIBUTE | ' ';
    }

    dp.buffer = video_ram;
    
    initialized = true;
  }
}

void
init (void)
{
  initialize ();
  finish (0, 0, 0, 0, -1, 0);
}
EMBED_ACTION_DESCRIPTOR (INTERNAL, NO_PARAMETER, LILY_ACTION_INIT, init);

void
focus (void)
{
  initialize ();
  finish (0, 0, 0, 0, -1, 0);
}
EMBED_ACTION_DESCRIPTOR (INPUT, NO_PARAMETER, CONSUMER_FOCUS, focus);

void
print (aid_t aid,
       const char* string,
       size_t length)
{
  initialize ();
  
  if (binding_count (CONSUMER_PRINT_SENSE, (const void*)aid) != 0) {
    client_context_t* context = find_client_context (aid);
    if (context == 0) {
      context = create_client_context (aid);
    }

    if (string != 0 && length != 0) {
      print_on_context (context, string, length);
    }
  }
  
  finish (0, 0, 0, 0, -1, 0);
}
EMBED_ACTION_DESCRIPTOR (INPUT, AUTO_PARAMETER, CONSUMER_PRINT, print);

void
print_sense (aid_t aid)
{
  initialize ();

  size_t count = binding_count (CONSUMER_PRINT_SENSE, (const void*)aid);
  if (count != 0) {
    /* Create client context if necessary. */
    if (find_client_context (aid) == 0) {
      client_context_t* context = create_client_context (aid);
      context->next = context_list_head;
      context_list_head = context;
    }
  }
  else {
    /* Destroy client context if necessary. */
    client_context_t** ptr = &context_list_head;
    for (; *ptr != 0 && (*ptr)->aid != aid; ptr = &(*ptr)->next) ;;
    if (*ptr != 0) {
      client_context_t* temp = *ptr;
      *ptr = temp->next;
      destroy_client_context (temp);
    }
  }

  finish (0, 0, 0, 0, -1, 0);
}
EMBED_ACTION_DESCRIPTOR (OUTPUT, AUTO_PARAMETER, CONSUMER_PRINT_SENSE, print_sense);
