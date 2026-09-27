#ifndef EDITOR_H
#define EDITOR_H

void editor_init(void);

void editor_enable_raw_mode(void);
void editor_disable_raw_mode(void);

char editor_read_key(void);

void editor_refresh_screen(void);
void editor_clear_screen(void);
void editor_draw_rows(void);

#endif
