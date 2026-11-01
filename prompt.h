#ifndef PROMPT_H
#define PROMPT_H

char *prompt(const char *message); /* middleman */
void prompt_message(const char *message, int seconds);
void prompt_refresh(void);

#endif
