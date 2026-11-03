#ifndef PROMPT_H
#define PROMPT_H

char *prompt(const char *message); /* middleman */
void prompt_message(const char *message);
void prompt_clear(void);
void prompt_refresh(void);

#endif
