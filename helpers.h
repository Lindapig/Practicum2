#ifndef HELPERS_H
#define HELPERS_H

int sendString(int sockD, const char *str);
int receiveString(int sockD, char **str);
int isValidFile(const char *file_name);
char *getFilePrefix(const char *file_name, char delimiter);
char *getFileSuffix(const char *file_name, char delimiter);
void createFileName(char *new_file, char *prev_file, int ver_num);
char *getConfig(const char *target);
void errorMsg(const char *msg);

#endif