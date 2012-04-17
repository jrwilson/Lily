#include "ctype.h"

#define BLANK (1 << 0)
#define CNTRL (1 << 1)
#define PUNCT (1 << 2)
#define ALNUM (1 << 3)
#define UPPER (1 << 4)
#define LOWER (1 << 5)
#define ALPHA (1 << 6)
#define DIGIT (1 << 7)
#define XDIGIT (1 << 8)
#define SPACE (1 << 9)
#define PRINT (1 << 10)
#define GRAPH (1 << 11)

static unsigned short table[256] = {
  [0x0] = 0 | CNTRL,
  [0x1] = 0 | CNTRL,
  [0x2] = 0 | CNTRL,
  [0x3] = 0 | CNTRL,
  [0x4] = 0 | CNTRL,
  [0x5] = 0 | CNTRL,
  [0x6] = 0 | CNTRL,
  [0x7] = 0 | CNTRL,
  [0x8] = 0 | CNTRL,
  [0x9] = 0 | BLANK | CNTRL | SPACE,
  [0xa] = 0 | CNTRL | SPACE,
  [0xb] = 0 | CNTRL | SPACE,
  [0xc] = 0 | CNTRL | SPACE,
  [0xd] = 0 | CNTRL | SPACE,
  [0xe] = 0 | CNTRL,
  [0xf] = 0 | CNTRL,
  [0x10] = 0 | CNTRL,
  [0x11] = 0 | CNTRL,
  [0x12] = 0 | CNTRL,
  [0x13] = 0 | CNTRL,
  [0x14] = 0 | CNTRL,
  [0x15] = 0 | CNTRL,
  [0x16] = 0 | CNTRL,
  [0x17] = 0 | CNTRL,
  [0x18] = 0 | CNTRL,
  [0x19] = 0 | CNTRL,
  [0x1a] = 0 | CNTRL,
  [0x1b] = 0 | CNTRL,
  [0x1c] = 0 | CNTRL,
  [0x1d] = 0 | CNTRL,
  [0x1e] = 0 | CNTRL,
  [0x1f] = 0 | CNTRL,
  [0x20] = 0 | BLANK | SPACE | PRINT,
  [0x21] = 0 | PUNCT | PRINT | GRAPH,
  [0x22] = 0 | PUNCT | PRINT | GRAPH,
  [0x23] = 0 | PUNCT | PRINT | GRAPH,
  [0x24] = 0 | PUNCT | PRINT | GRAPH,
  [0x25] = 0 | PUNCT | PRINT | GRAPH,
  [0x26] = 0 | PUNCT | PRINT | GRAPH,
  [0x27] = 0 | PUNCT | PRINT | GRAPH,
  [0x28] = 0 | PUNCT | PRINT | GRAPH,
  [0x29] = 0 | PUNCT | PRINT | GRAPH,
  [0x2a] = 0 | PUNCT | PRINT | GRAPH,
  [0x2b] = 0 | PUNCT | PRINT | GRAPH,
  [0x2c] = 0 | PUNCT | PRINT | GRAPH,
  [0x2d] = 0 | PUNCT | PRINT | GRAPH,
  [0x2e] = 0 | PUNCT | PRINT | GRAPH,
  [0x2f] = 0 | PUNCT | PRINT | GRAPH,
  [0x30] = 0 | ALNUM | DIGIT | XDIGIT | PRINT | GRAPH,
  [0x31] = 0 | ALNUM | DIGIT | XDIGIT | PRINT | GRAPH,
  [0x32] = 0 | ALNUM | DIGIT | XDIGIT | PRINT | GRAPH,
  [0x33] = 0 | ALNUM | DIGIT | XDIGIT | PRINT | GRAPH,
  [0x34] = 0 | ALNUM | DIGIT | XDIGIT | PRINT | GRAPH,
  [0x35] = 0 | ALNUM | DIGIT | XDIGIT | PRINT | GRAPH,
  [0x36] = 0 | ALNUM | DIGIT | XDIGIT | PRINT | GRAPH,
  [0x37] = 0 | ALNUM | DIGIT | XDIGIT | PRINT | GRAPH,
  [0x38] = 0 | ALNUM | DIGIT | XDIGIT | PRINT | GRAPH,
  [0x39] = 0 | ALNUM | DIGIT | XDIGIT | PRINT | GRAPH,
  [0x3a] = 0 | PUNCT | PRINT | GRAPH,
  [0x3b] = 0 | PUNCT | PRINT | GRAPH,
  [0x3c] = 0 | PUNCT | PRINT | GRAPH,
  [0x3d] = 0 | PUNCT | PRINT | GRAPH,
  [0x3e] = 0 | PUNCT | PRINT | GRAPH,
  [0x3f] = 0 | PUNCT | PRINT | GRAPH,
  [0x40] = 0 | PUNCT | PRINT | GRAPH,
  [0x41] = 0 | ALNUM | UPPER | ALPHA | XDIGIT | PRINT | GRAPH,
  [0x42] = 0 | ALNUM | UPPER | ALPHA | XDIGIT | PRINT | GRAPH,
  [0x43] = 0 | ALNUM | UPPER | ALPHA | XDIGIT | PRINT | GRAPH,
  [0x44] = 0 | ALNUM | UPPER | ALPHA | XDIGIT | PRINT | GRAPH,
  [0x45] = 0 | ALNUM | UPPER | ALPHA | XDIGIT | PRINT | GRAPH,
  [0x46] = 0 | ALNUM | UPPER | ALPHA | XDIGIT | PRINT | GRAPH,
  [0x47] = 0 | ALNUM | UPPER | ALPHA | PRINT | GRAPH,
  [0x48] = 0 | ALNUM | UPPER | ALPHA | PRINT | GRAPH,
  [0x49] = 0 | ALNUM | UPPER | ALPHA | PRINT | GRAPH,
  [0x4a] = 0 | ALNUM | UPPER | ALPHA | PRINT | GRAPH,
  [0x4b] = 0 | ALNUM | UPPER | ALPHA | PRINT | GRAPH,
  [0x4c] = 0 | ALNUM | UPPER | ALPHA | PRINT | GRAPH,
  [0x4d] = 0 | ALNUM | UPPER | ALPHA | PRINT | GRAPH,
  [0x4e] = 0 | ALNUM | UPPER | ALPHA | PRINT | GRAPH,
  [0x4f] = 0 | ALNUM | UPPER | ALPHA | PRINT | GRAPH,
  [0x50] = 0 | ALNUM | UPPER | ALPHA | PRINT | GRAPH,
  [0x51] = 0 | ALNUM | UPPER | ALPHA | PRINT | GRAPH,
  [0x52] = 0 | ALNUM | UPPER | ALPHA | PRINT | GRAPH,
  [0x53] = 0 | ALNUM | UPPER | ALPHA | PRINT | GRAPH,
  [0x54] = 0 | ALNUM | UPPER | ALPHA | PRINT | GRAPH,
  [0x55] = 0 | ALNUM | UPPER | ALPHA | PRINT | GRAPH,
  [0x56] = 0 | ALNUM | UPPER | ALPHA | PRINT | GRAPH,
  [0x57] = 0 | ALNUM | UPPER | ALPHA | PRINT | GRAPH,
  [0x58] = 0 | ALNUM | UPPER | ALPHA | PRINT | GRAPH,
  [0x59] = 0 | ALNUM | UPPER | ALPHA | PRINT | GRAPH,
  [0x5a] = 0 | ALNUM | UPPER | ALPHA | PRINT | GRAPH,
  [0x5b] = 0 | PUNCT | PRINT | GRAPH,
  [0x5c] = 0 | PUNCT | PRINT | GRAPH,
  [0x5d] = 0 | PUNCT | PRINT | GRAPH,
  [0x5e] = 0 | PUNCT | PRINT | GRAPH,
  [0x5f] = 0 | PUNCT | PRINT | GRAPH,
  [0x60] = 0 | PUNCT | PRINT | GRAPH,
  [0x61] = 0 | ALNUM | LOWER | ALPHA | XDIGIT | PRINT | GRAPH,
  [0x62] = 0 | ALNUM | LOWER | ALPHA | XDIGIT | PRINT | GRAPH,
  [0x63] = 0 | ALNUM | LOWER | ALPHA | XDIGIT | PRINT | GRAPH,
  [0x64] = 0 | ALNUM | LOWER | ALPHA | XDIGIT | PRINT | GRAPH,
  [0x65] = 0 | ALNUM | LOWER | ALPHA | XDIGIT | PRINT | GRAPH,
  [0x66] = 0 | ALNUM | LOWER | ALPHA | XDIGIT | PRINT | GRAPH,
  [0x67] = 0 | ALNUM | LOWER | ALPHA | PRINT | GRAPH,
  [0x68] = 0 | ALNUM | LOWER | ALPHA | PRINT | GRAPH,
  [0x69] = 0 | ALNUM | LOWER | ALPHA | PRINT | GRAPH,
  [0x6a] = 0 | ALNUM | LOWER | ALPHA | PRINT | GRAPH,
  [0x6b] = 0 | ALNUM | LOWER | ALPHA | PRINT | GRAPH,
  [0x6c] = 0 | ALNUM | LOWER | ALPHA | PRINT | GRAPH,
  [0x6d] = 0 | ALNUM | LOWER | ALPHA | PRINT | GRAPH,
  [0x6e] = 0 | ALNUM | LOWER | ALPHA | PRINT | GRAPH,
  [0x6f] = 0 | ALNUM | LOWER | ALPHA | PRINT | GRAPH,
  [0x70] = 0 | ALNUM | LOWER | ALPHA | PRINT | GRAPH,
  [0x71] = 0 | ALNUM | LOWER | ALPHA | PRINT | GRAPH,
  [0x72] = 0 | ALNUM | LOWER | ALPHA | PRINT | GRAPH,
  [0x73] = 0 | ALNUM | LOWER | ALPHA | PRINT | GRAPH,
  [0x74] = 0 | ALNUM | LOWER | ALPHA | PRINT | GRAPH,
  [0x75] = 0 | ALNUM | LOWER | ALPHA | PRINT | GRAPH,
  [0x76] = 0 | ALNUM | LOWER | ALPHA | PRINT | GRAPH,
  [0x77] = 0 | ALNUM | LOWER | ALPHA | PRINT | GRAPH,
  [0x78] = 0 | ALNUM | LOWER | ALPHA | PRINT | GRAPH,
  [0x79] = 0 | ALNUM | LOWER | ALPHA | PRINT | GRAPH,
  [0x7a] = 0 | ALNUM | LOWER | ALPHA | PRINT | GRAPH,
  [0x7b] = 0 | PUNCT | PRINT | GRAPH,
  [0x7c] = 0 | PUNCT | PRINT | GRAPH,
  [0x7d] = 0 | PUNCT | PRINT | GRAPH,
  [0x7e] = 0 | PUNCT | PRINT | GRAPH,
  [0x7f] = 0 | CNTRL,
  [0x80] = 0,
  [0x81] = 0,
  [0x82] = 0,
  [0x83] = 0,
  [0x84] = 0,
  [0x85] = 0,
  [0x86] = 0,
  [0x87] = 0,
  [0x88] = 0,
  [0x89] = 0,
  [0x8a] = 0,
  [0x8b] = 0,
  [0x8c] = 0,
  [0x8d] = 0,
  [0x8e] = 0,
  [0x8f] = 0,
  [0x90] = 0,
  [0x91] = 0,
  [0x92] = 0,
  [0x93] = 0,
  [0x94] = 0,
  [0x95] = 0,
  [0x96] = 0,
  [0x97] = 0,
  [0x98] = 0,
  [0x99] = 0,
  [0x9a] = 0,
  [0x9b] = 0,
  [0x9c] = 0,
  [0x9d] = 0,
  [0x9e] = 0,
  [0x9f] = 0,
  [0xa0] = 0,
  [0xa1] = 0,
  [0xa2] = 0,
  [0xa3] = 0,
  [0xa4] = 0,
  [0xa5] = 0,
  [0xa6] = 0,
  [0xa7] = 0,
  [0xa8] = 0,
  [0xa9] = 0,
  [0xaa] = 0,
  [0xab] = 0,
  [0xac] = 0,
  [0xad] = 0,
  [0xae] = 0,
  [0xaf] = 0,
  [0xb0] = 0,
  [0xb1] = 0,
  [0xb2] = 0,
  [0xb3] = 0,
  [0xb4] = 0,
  [0xb5] = 0,
  [0xb6] = 0,
  [0xb7] = 0,
  [0xb8] = 0,
  [0xb9] = 0,
  [0xba] = 0,
  [0xbb] = 0,
  [0xbc] = 0,
  [0xbd] = 0,
  [0xbe] = 0,
  [0xbf] = 0,
  [0xc0] = 0,
  [0xc1] = 0,
  [0xc2] = 0,
  [0xc3] = 0,
  [0xc4] = 0,
  [0xc5] = 0,
  [0xc6] = 0,
  [0xc7] = 0,
  [0xc8] = 0,
  [0xc9] = 0,
  [0xca] = 0,
  [0xcb] = 0,
  [0xcc] = 0,
  [0xcd] = 0,
  [0xce] = 0,
  [0xcf] = 0,
  [0xd0] = 0,
  [0xd1] = 0,
  [0xd2] = 0,
  [0xd3] = 0,
  [0xd4] = 0,
  [0xd5] = 0,
  [0xd6] = 0,
  [0xd7] = 0,
  [0xd8] = 0,
  [0xd9] = 0,
  [0xda] = 0,
  [0xdb] = 0,
  [0xdc] = 0,
  [0xdd] = 0,
  [0xde] = 0,
  [0xdf] = 0,
  [0xe0] = 0,
  [0xe1] = 0,
  [0xe2] = 0,
  [0xe3] = 0,
  [0xe4] = 0,
  [0xe5] = 0,
  [0xe6] = 0,
  [0xe7] = 0,
  [0xe8] = 0,
  [0xe9] = 0,
  [0xea] = 0,
  [0xeb] = 0,
  [0xec] = 0,
  [0xed] = 0,
  [0xee] = 0,
  [0xef] = 0,
  [0xf0] = 0,
  [0xf1] = 0,
  [0xf2] = 0,
  [0xf3] = 0,
  [0xf4] = 0,
  [0xf5] = 0,
  [0xf6] = 0,
  [0xf7] = 0,
  [0xf8] = 0,
  [0xf9] = 0,
  [0xfa] = 0,
  [0xfb] = 0,
  [0xfc] = 0,
  [0xfd] = 0,
  [0xfe] = 0,
  [0xff] = 0,
};

int
isalnum (int c)
{
  return table[c & 0xff] & ALNUM;
}

int
isalpha (int c)
{
  return table[c & 0xff] & ALPHA;
}

int
islower (int c)
{
  return table[c & 0xff] & LOWER;
}

int
isupper (int c)
{
  return table[c & 0xff] & UPPER;
}

int
isdigit (int c)
{
  return table[c & 0xff] & DIGIT;
}

int
isxdigit (int c)
{
  return table[c & 0xff] & XDIGIT;
}

int
iscntrl (int c)
{
  return table[c & 0xff] & CNTRL;
}

int
isgraph (int c)
{
  return table[c & 0xff] & GRAPH;
}

int
isspace (int c)
{
  return table[c & 0xff] & SPACE;
}

int
isblank (int c)
{
  return table[c & 0xff] & BLANK;
}

int
isprint (int c)
{
  return table[c & 0xff] & PRINT;
}

int
ispunct (int c)
{
  return table[c & 0xff] & PUNCT;
}
