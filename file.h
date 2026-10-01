#ifndef FILE_H
#define FILE_H

int open_file(const char *filename);
int save_file(const char *filename);
extern const char *current_filename;

#endif
