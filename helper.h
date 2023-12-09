#ifndef HELPER_H
#define HELPER_H

char *getConfig(const char *target);
int sendText(int sockD, const char *str);
int receiveText(int sockD, char **str);
int isValidFile(const char *file_name);
char *getFilePrefix(const char *file_name, char delimiter);
char *getFileSuffix(const char *file_name, char delimiter);
void createFileName(char *new_file, char *prev_file, int versionNumber);
void errorMsg(const char *msg);

#endif