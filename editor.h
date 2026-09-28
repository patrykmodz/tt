#ifndef EDITOR_H
#define EDITOR_H

/* control key constants */
#define CTRL_A 0x01
#define CTRL_B 0x02
#define CTRL_C 0x03
#define CTRL_D 0x04
#define CTRL_E 0x05
#define CTRL_F 0x06
#define CTRL_G 0x07
#define CTRL_H 0x08
#define CTRL_I 0x09
#define CTRL_J 0x0A
#define CTRL_K 0x0B
#define CTRL_L 0x0C
#define CTRL_M 0x0D
#define CTRL_N 0x0E
#define CTRL_O 0x0F
#define CTRL_P 0x10
#define CTRL_Q 0x11
#define CTRL_R 0x12
#define CTRL_S 0x13
#define CTRL_T 0x14
#define CTRL_U 0x15
#define CTRL_V 0x16
#define CTRL_W 0x17
#define CTRL_X 0x18
#define CTRL_Y 0x19
#define CTRL_Z 0x1A

void editor_init(void);

void editor_enable_raw_mode(void);
void editor_disable_raw_mode(void);

int editor_read_key(char *key);

void editor_insert_row(int at);
void editor_row_insert_char(struct editor_row *row, int at, char c);

void editor_refresh_screen(void);
void editor_clear_screen(void);
void editor_draw_rows(void);

#endif
