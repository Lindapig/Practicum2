/*
 * server.c -- TCP Socket Server
 *
 * adapted from:
 *   https://www.educative.io/answers/how-to-implement-tcp-sockets-in-c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include "helpers.h"

#define MAX_BUFFER_SIZE 1024
#define VER_BUFFER_SIZE 256
#define PORT_NUMBER 2000
#define VERSION_PATH ".file_VERSION"
#define LOCK_FILE ".file_LOCK"

// Function to get the latest version number by file name
int getLatestVersion(const char *file_name)
{
  FILE *fp = fopen(VERSION_PATH, "r");
  if (fp == NULL)
  {
    error("Error opening version info file");
  }
  char line[VER_BUFFER_SIZE];
  while (fgets(line, VER_BUFFER_SIZE, fp))
  {
    // parse the input line by '=' to read key and value
    char *equals = strchr(line, '=');
    if (equals != NULL)
    {
      *equals = '\0';
      if (strcmp(line, file_name) == 0)
      {
        fclose(fp);
        return atoi(equals + 1);
      }
    }
  }
  fclose(fp);
  return 0;
}

// Function to update the latest version by file name
void updateLatestVersion(const char *file_name, int ver_num)
{
  FILE *fp = fopen(VERSION_PATH, ver_num == 1 ? "a" : "r+");
  if (fp == NULL)
  {
    error("Error opening version info file");
  }

  if (ver_num == 1)
  {
    fprintf(fp, "%s=%d\n", file_name, ver_num);
    fclose(fp);
    return;
  }

  char search_key[strlen(file_name) + 1];
  sprintf(search_key, "%s=", file_name);
  char line[VER_BUFFER_SIZE];
  while (fgets(line, VER_BUFFER_SIZE, fp))
  {
    if (strncmp(line, search_key, strlen(search_key)) == 0)
    {
      // Rewind the file pointer to the beginning of the line
      fseek(fp, -strlen(line), SEEK_CUR);

      // Replace the old version with the new version
      fprintf(fp, "%s%d\n", search_key, ver_num);

      // Move the file pointer back to where it was
      fseek(fp, 0, SEEK_CUR);
      break;
    }
  }

  fclose(fp);
}

// Function to remove version info related to a file name
void removeVersionInfo(const char *file_name)
{
  FILE *fp = fopen(VERSION_PATH, "r");
  if (fp == NULL)
  {
    error("Error opening version info file");
  }

  FILE *temp = fopen(".temp", "w");
  if (temp == NULL)
  {
    error("Error creating temp file");
  }

  char search_key[strlen(file_name) + 1];
  sprintf(search_key, "%s=", file_name);
  char line[VER_BUFFER_SIZE];

  while (fgets(line, VER_BUFFER_SIZE, fp))
  {
    if (strncmp(line, search_key, strlen(search_key)) != 0)
    {
      // Copy lines other than the one to be removed to the temporary file
      fprintf(temp, "%s", line);
    }
  }

  // Close both files
  fclose(fp);
  fclose(temp);

  // Replace the original file with the temporary file
  if (rename(".temp", VERSION_PATH) != 0)
  {
    error("Error replacing version info");
  }
}

// Print a failure message and send it to client
void handleFailure(int client_sock, const char *message)
{
  perror(message);
  sendString(client_sock, message);
}

// Function to create a file lock in the given directory
int createLock(char *directory, char **lock_path)
{
  if (directory == NULL)
  {
    *lock_path = (char *)malloc(strlen(LOCK_FILE));
    strcpy(*lock_path, LOCK_FILE);
  }
  else
  {
    *lock_path = (char *)malloc(strlen(directory) + strlen(LOCK_FILE) + 1);
    sprintf(*lock_path, "%s/%s", directory, LOCK_FILE);
    free(directory);
  }

  // When lock already exists in current folder
  if (isFileExist(*lock_path))
  {
    return 0;
  }

  // Create the lock
  FILE *lockfp = fopen(*lock_path, "w");
  if (lockfp == NULL)
  {
    return 0;
  }
  fclose(lockfp);

  return 1;
}

// Function to handle write operation from the server side
void handleWrite(int client_sock)
{
  // Receive client's remote file path
  char *local_file;
  if (!receiveString(client_sock, &local_file))
  {
    handleFailure(client_sock, "Error receiving file path for writing");
    return;
  }

  // Find the latest version number (Question 5)
  char *file_name = (char *)malloc(strlen(local_file) + 3);
  if (file_name == NULL)
  {
    handleFailure(client_sock, "Error allocating memory");
    return;
  }

  if (!isFileExist(local_file))
  {
    createFileName(file_name, local_file, 0);
  }
  else
  {
    int ver_num = getLatestVersion(local_file) + 1;
    createFileName(file_name, local_file, ver_num);
    updateLatestVersion(local_file, ver_num);
  }

  // Lock the current directory to avoid concurrent modification
  char *prefix = findFilePrefix(local_file, '/');
  char *lock_path;

  if (!createLock(prefix, &lock_path))
  {
    handleFailure(client_sock, "Error creating file lock");
    return;
  }

  // Open local file
  FILE *fp = fopen(file_name, "w");
  if (fp == NULL)
  {
    handleFailure(client_sock, "Error opening remote file for writing");
    return;
  }

  char *buffer;
  int len = receiveString(client_sock, &buffer);
  if (len)
  {
    fwrite(buffer, 1, len, fp);
  }
  else
  {
    return;
  }

  // Release the file lock
  if (remove(lock_path) != 0)
  {
    handleFailure(client_sock, "Error removing the lock");
    return;
  }

  char response[MAX_BUFFER_SIZE];
  sprintf(response, "Successfully writing to file '%s'", file_name);
  sendString(client_sock, response);

  // Free memory and close file
  free(buffer);
  free(lock_path);
  free(local_file);
  free(file_name);
  fclose(fp);
}

// Function to handle get operation from the server side
void handleGet(int client_sock)
{
  // Receive client's remote file path
  char *local_file;
  if (!receiveString(client_sock, &local_file))
  {
    handleFailure(client_sock, "Error receiving file path for reading");
    return;
  }

  // Receive the version number of file
  int ver_num;
  if (recv(client_sock, &ver_num, sizeof(ver_num), 0) < 0)
  {
    handleFailure(client_sock, "Error receiving version number");
    return;
  }
  if (ver_num == -1)
  {
    // No appointed version number -> use the latest version
    ver_num = getLatestVersion(local_file);
  }

  // Get the corresponding version of the given file (Question 7)
  char *file_name = (char *)malloc(strlen(local_file) + 3);
  if (file_name == NULL)
  {
    handleFailure(client_sock, "Error allocating memory");
    return;
  }

  createFileName(file_name, local_file, ver_num);

  // Open local file
  FILE *fp = fopen(file_name, "r");
  if (fp == NULL)
  {
    handleFailure(client_sock, "Error opening remote file for reading");
    return;
  }

  // Read from the local file and write to the remote file
  char buffer[MAX_BUFFER_SIZE];
  size_t bytesRead = fread(buffer, 1, sizeof(buffer), fp);
  if (bytesRead == 0)
  {
    handleFailure(client_sock, "Error reading data from remote file");
    return;
  }

  buffer[bytesRead] = '\0';
  if (!sendString(client_sock, buffer))
  {
    handleFailure(client_sock, "Error sending data to client");
    return;
  }

  // Send response to the client
  char response[VER_BUFFER_SIZE];
  sprintf(response, "Successfully reading from file '%s'", file_name);
  sendString(client_sock, response);

  // Free memory and close file
  free(local_file);
  free(file_name);
  fclose(fp);
}

// Function to handle remove operation from the server side
void handleRemove(int client_sock)
{
  // Receive client's remote file path
  char *local_path;
  if (!receiveString(client_sock, &local_path))
  {
    return;
  }

  char *file_name = (char *)malloc(strlen(local_path) + 3);
  if (file_name == NULL)
  {
    handleFailure(client_sock, "Error allocating memory");
    return;
  }

  // The response to be returned:
  char response[MAX_BUFFER_SIZE];

  // Find all versions of the file to remove
  int ver_num = getLatestVersion(local_path);
  for (int i = 0; i <= ver_num; i++)
  {
    createFileName(file_name, local_path, i);
    if (!isFileExist(file_name))
    {
      char warning[VER_BUFFER_SIZE];
      sprintf(warning, "File '%s' not exist\n", file_name);
      strcat(response, warning);
      continue;
    }
    // Use the system command to execute the remove operation
    char command[VER_BUFFER_SIZE];
    sprintf(command, "rm -rf %s", file_name);
    char message[VER_BUFFER_SIZE];
    if (system(command) == 0)
    {
      sprintf(message, "File '%s' is removed successfully\n", file_name);
    }
    else
    {
      sprintf(message, "Error removing file '%s'\n", file_name);
    }
    strcat(response, message);
  }

  // Remove related version info
  if (ver_num > 0)
  {
    removeVersionInfo(local_path);
  }

  // Trim new line character
  response[strlen(response) - 1] = '\0';

  // Send response to the client
  sendString(client_sock, response);

  // Free memory
  free(local_path);
  free(file_name);
}

// Function to handle list operation from the server side
void handleList(int client_sock)
{
  // Receive client's remote file path
  char *local_file;
  if (!receiveString(client_sock, &local_file))
  {
    return;
  }

  char *file_name = (char *)malloc(strlen(local_file) + 3);
  if (file_name == NULL)
  {
    handleFailure(client_sock, "Error allocating memory");
    return;
  }

  // The response to be returned:
  char response[MAX_BUFFER_SIZE];
  sprintf(response, "Versioning Information about %s:\n\n", local_file);

  // Find all versions of the file to list
  int ver_num = getLatestVersion(local_file);
  for (int v = 0; v <= ver_num; v++)
  {
    createFileName(file_name, local_file, v);
    if (!isFileExist(file_name))
    {
      char warning[VER_BUFFER_SIZE];
      sprintf(warning, "File '%s' not exist\n", file_name);
      perror(warning);
      strcat(response, warning);
      continue;
    }

    // Get file information
    struct stat file_stat;
    if (stat(file_name, &file_stat) < 0)
    {
      char warning[VER_BUFFER_SIZE];
      sprintf(warning, "Error getting information about file '%s'", file_name);
      perror(warning);
      strcat(response, warning);
      continue;
    }

    // Construct versioning information
    char version_info[MAX_BUFFER_SIZE];
    snprintf(version_info, sizeof(version_info), "File: %s\nVersion: v%d\nLast modified: %s\n",
             file_name,
             v,
             ctime(&file_stat.st_mtime));

    strcat(response, version_info);
  }

  // Trim new line character
  response[strlen(response) - 1] = '\0';

  // Send versioning information to the client
  sendString(client_sock, response);

  // Free memory
  free(local_file);
  free(file_name);
}

// Function to handle stop operation from the server side
void handleStop(int client_sock, int socket_desc)
{
  sendString(client_sock, "Server terminated by client");
  shutdown(client_sock, SHUT_RDWR);
  close(client_sock);
  close(socket_desc);
  exit(EXIT_SUCCESS);
}

// Thread function to handle each client connection
void *handleClient(void *arg)
{
  int *sockets = (int *)arg;
  int client_sock = sockets[0], socket_desc = sockets[1];

  // Receive client's action string
  char *action;
  if (!receiveString(client_sock, &action))
  {
    return NULL;
  }

  if (strcmp(action, "WRITE") == 0)
  { // Question 1
    handleWrite(client_sock);
  }
  else if (strcmp(action, "GET") == 0)
  { // Question 2
    handleGet(client_sock);
  }
  else if (strcmp(action, "RM") == 0)
  { // Question 3
    handleRemove(client_sock);
  }
  else if (strcmp(action, "LS") == 0)
  { // Question 6
    handleList(client_sock);
  }
  else if (strcmp(action, "STOP") == 0)
  { // Turn off the server
    free(action);
    handleStop(client_sock, socket_desc);
  }
  else
  {
    perror("Invalid action");
  }

  free(action);

  // Close the client socket before exiting the thread
  close(client_sock);

  // Exit the thread
  pthread_exit(NULL);
}

int main(void)
{
  int socket_desc, client_sock;
  socklen_t client_size;
  struct sockaddr_in server_addr, client_addr;

  // Create socket:
  socket_desc = socket(AF_INET, SOCK_STREAM, 0);

  if (socket_desc < 0)
  {
    error("Error while creating socket");
  }

  // Set port and IP:
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT_NUMBER);
  char *ip_address = getConfigVar("IP_ADDRESS");
  if (ip_address == NULL)
  {
    error("Error retrieving IP address from .config");
  }
  server_addr.sin_addr.s_addr = inet_addr(ip_address);

  // Bind to the set port and IP:
  if (bind(socket_desc, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
  {
    error("Couldn't bind to the port");
  }
  printf("Done with binding\n");

  // Listen for clients:
  if (listen(socket_desc, 1) < 0)
  {
    error("Error while listening");
  }
  printf("\nListening for incoming connections.....\n");

  // Accept incoming connections:
  while (1)
  {
    client_size = sizeof(client_addr);
    client_sock = accept(socket_desc, (struct sockaddr *)&client_addr, &client_size);

    if (client_sock < 0)
    {
      perror("Can't accept");
      continue;
    }
    printf("\nClient connected at IP: %s and port: %i\n",
           inet_ntoa(client_addr.sin_addr),
           ntohs(client_addr.sin_port));

    // Create a thread for the client connection
    pthread_t tid;
    int sockets[] = {client_sock, socket_desc};
    if (pthread_create(&tid, NULL, handleClient, (void *)sockets) != 0)
    {
      perror("Error creating thread");
      close(client_sock);
      continue; // Continue to the next iteration
    }

    // Detach the thread to allow it to clean up resources on exit
    pthread_detach(tid);
  }

  close(socket_desc);

  return 0;
}
