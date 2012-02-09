#ifndef TERMINAL_H
#define TERMINAL_H

#define TERMINAL_FOCUS 1
#define TERMINAL_DISPLAY 2
#define TERMINAL_VGA_OP 3

#define NUL 0X00
#define BS  0X08
#define HT  0X09
#define LF  0x0A
#define CR  0x0D
#define ESC 0x1B

#define CSI 0x5B

#define ICH 0x40
#define CUU 0x41
#define CUD 0x42
#define CUF 0x43
#define CUB 0x44
#define CUP 0x48

/* The ASCII delete character. */
#define DEL 0x7F

#endif /* TERMINAL_H */
