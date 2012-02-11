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
#define ESC_S "\x1b"

#define CSI 0x5B
#define CSI_S "["

#define ICH 0x40
#define ICH_S "@"
#define CUU 0x41
#define CUU_S "A"
#define CUD 0x42
#define CUD_S "B"
#define CUF 0x43
#define CUF_S "C"
#define CUB 0x44
#define CUB_S "D"
#define CUP 0x48
#define CUP_S "H"
#define ED 0x4A
#define ED_S "J"

/* The ASCII delete character. */
#define DEL 0x7F

#define ERASE_TO_PAGE_LIMIT 0
#define ERASE_TO_PAGE_LIMIT_S "0"
#define ERASE_TO_PAGE_HOME 1
#define ERASE_TO_PAGE_HOME_S "1"
#define ERASE_ALL 2
#define ERASE_ALL_S "2"

#endif /* TERMINAL_H */
