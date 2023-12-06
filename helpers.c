#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

// Function to exit the program with the given message
void error(const char *msg)
{
  perror(msg);
  exit(EXIT_FAILURE);
}

// Function to send a string from socket
int sendString(int sockfd, const char *str)
{
  size_t len = strlen(str);
  if (send(sockfd, &len, sizeof(len), 0) < 0)
  {
    perror("Error sending string length");
    return 0;
  }
  if (send(sockfd, str, len, 0) < 0)
  {
    perror("Error sending string data");
    return 0;
  }
  return (int)len;
}

// Function to receive a string from socket
int receiveString(int sockfd, char **str)
{
  size_t len;
  if (recv(sockfd, &len, sizeof(len), 0) < 0)
  {
    perror("Error receiving string length");
    return 0;
  }

  *str = (char *)malloc(len + 1); // +1 for null terminator
  if (*str == NULL)
  {
    perror("Error allocating memory for string");
    return 0;
  }

  if (recv(sockfd, *str, len, 0) < 0)
  {
    perror("Error receiving string data");
    return 0;
  }

  (*str)[len] = '\0'; // Null-terminate the received string
  return (int)len;
}

// Function to determine whether a file name already exists
int isFileExist(const char *file_name)
{
  FILE *fp = fopen(file_name, "r");
  if (fp == NULL)
  {
    return 0;
  }
  fclose(fp);
  return 1;
}

// Function to find and return the prefix of a file name by delimiter
char *findFilePrefix(const char *file_name, char delimiter)
{
  const char *dot = strrchr(file_name, delimiter);
  if (dot == NULL || dot == file_name)
  {
    return NULL;
  }
  size_t len = dot - file_name;
  char *prefix = (char *)malloc(len + 1);
  if (prefix == NULL)
  {
    perror("Error allocating memory");
    return NULL;
  }
  strncpy(prefix, file_name, len);
  prefix[len] = '\0';
  return prefix;
}

// Function to find and return the suffix of a file name by delimiter
char *findFileSuffix(const char *file_name, char delimiter)
{
  const char *dot = strrchr(file_name, delimiter);
  if (dot == NULL || dot == file_name)
  {
    return NULL;
  }
  char *suffix = (char *)malloc(strlen(dot + 1) + 1);
  if (suffix == NULL)
  {
    perror("Error allocating memory");
    return NULL;
  }
  strcpy(suffix, dot + 1);
  suffix[strlen(dot + 1)] = '\0';
  return suffix;
}

// Function to create a versioned file name by name (prefix) and version number (suffix)
void createFileName(char *new_file, char *prev_file, int ver_num)
{
  if (ver_num == 0)
  {
    sprintf(new_file, "%s", prev_file);
    new_file[strlen(prev_file) + 1] = '\0';
    return;
  }
  char *prefix = findFilePrefix(prev_file, '.');
  if (prefix == NULL)
  { // When there is no file suffix
    sprintf(new_file, "%s_%d", prev_file, ver_num);
  }
  else
  { // When there is a file suffix
    char *suffix = findFileSuffix(prev_file, '.');
    sprintf(new_file, "%s_%d.%s", prefix, ver_num, suffix);
    free(prefix);
    free(suffix);
  }
}

char *getConfigVar(const char *target)
{
  // Open .config and locate global variables
  FILE *file = fopen(".config", "r");
  if (file == NULL)
  {
    perror("Error opening .config file");
    return NULL;
  }
  char line[BUFSIZ];
  while (fgets(line, BUFSIZ, file))
  {
    // parse the input line by '=' to read key and value
    char *equals = strchr(line, '=');
    if (equals != NULL)
    {
      *equals = '\0';
      if (strcmp(line, target) == 0)
      {
        fclose(file);
        return equals + 1;
      }
    }
  }
  fclose(file);
  return NULL;
}