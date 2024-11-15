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
#include "helper.h"

#define PORT_NUMBER 1500
#define MAX_BUFFER_SIZE 1024
#define VER_BUFFER_SIZE 256
#define VERSION_PATH ".file_VERSION"
#define LOCK_FILE ".file_LOCK"

// Helper function: 
// Send error message to client
void sendError(int client_sock, const char *msgs)
{
  perror(msgs);
  sendText(client_sock, msgs);
}

// Helper function (Question 4)
// Create a file lock for the file directory to avoid data corruption,
// also prevents multiple clients from writing to the same file simultaneously.
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
  if (isValidFile(*lock_path))
  {
    return 0;
  }

  // Create the lock
  FILE *lockfilePointer = fopen(*lock_path, "w");
  if (lockfilePointer == NULL)
  {
    return 0;
  }
  fclose(lockfilePointer);

  return 1;
}

// Function: retreive the latest version number from file name
int getNewVer(const char *file_name)
{
  FILE *filePointer = fopen(VERSION_PATH, "r");
  if (filePointer == NULL)
  {
    errorMsg("Error opening version info file");
  }
  char line[VER_BUFFER_SIZE];
  // reads the file line by line and store in array
  while (fgets(line, VER_BUFFER_SIZE, filePointer))
  {
    // split the line by '=' to get file name and version number
    char *equals = strchr(line, '=');
    if (equals != NULL)
    {
      *equals = '\0'; // replace '=' with a null character
      if (strcmp(line, file_name) == 0)
      {
        fclose(filePointer);
        return atoi(equals + 1); // update version number by 1
      }
    }
  }
  fclose(filePointer);
  return 0;
}

// Function: update the latest version by file name
// Either appends a new version entry if it's the first version
// or udpates the existing entry for subsequent versions.
void updateNewVer(const char *file_name, int versionNumber)
{
  FILE *filePointer = fopen(VERSION_PATH, versionNumber == 1 ? "a" : "r+");
  if (filePointer == NULL)
  {
    errorMsg("Fail to open the version file");
  }

  // append new verson info if first version
  if (versionNumber == 1)
  {
    fprintf(filePointer, "%s=%d\n", file_name, versionNumber);
    fclose(filePointer);
    return;
  }

  char fileKey[strlen(file_name) + 1];
  sprintf(fileKey, "%s=", file_name);
  char line[VER_BUFFER_SIZE];

  // read file line by line
  while (fgets(line, VER_BUFFER_SIZE, filePointer))
  {
    if (strncmp(line, fileKey, strlen(fileKey)) == 0)
    {
      // Rewind the file pointer to the beginning of the line
      fseek(filePointer, -strlen(line), SEEK_CUR);

      // Replace the old version with the new version
      fprintf(filePointer, "%s%d\n", fileKey, versionNumber);

      // Move the file pointer back to where it was
      fseek(filePointer, 0, SEEK_CUR);
      break;
    }
  }

  fclose(filePointer);
}

// Function: remove version info related to a file name
void removeVersionInfo(const char *file_name)
{
  FILE *filePointer = fopen(VERSION_PATH, "r");
  if (filePointer == NULL)
  {
    errorMsg("Fail to open version file");
  }

  FILE *temp = fopen(".temp", "w");
  if (temp == NULL)
  {
    errorMsg("Fail to create temp file");
  }

  char fileKey[strlen(file_name) + 1];
  sprintf(fileKey, "%s=", file_name);
  char line[VER_BUFFER_SIZE];

  while (fgets(line, VER_BUFFER_SIZE, filePointer))
  {
    if (strncmp(line, fileKey, strlen(fileKey)) != 0)
    {
      // Copy lines other than the one to be removed to the temporary file
      fprintf(temp, "%s", line);
    }
  }

  // Close files
  fclose(filePointer);
  fclose(temp);

  // Replace the original file with the temporary file
  if (rename(".temp", VERSION_PATH) != 0)
  {
    errorMsg("Fail to replace version info");
  }
}



// Question 1
// Function: Write from the server side
void operateWrite(int client_sock)
{
  // Receive client's remote file path
  char *local_file;
  if (!receiveText(client_sock, &local_file))
  {
    sendError(client_sock, "Invalid local file path.");
    return;
  }

  // (Question 5) Find the latest version number 
  char *file_name = (char *)malloc(strlen(local_file) + 3); // 2 for versioning, 1 for null terminator
  if (file_name == NULL)
  {
    sendError(client_sock, "Fail allocating memory"); // send error message to client socket descriptor
    return;
  }

  if (!isValidFile(local_file))
  {
    createFileName(file_name, local_file, 0); // create new file if file invalid
  }
  else
  {
    int versionNumber = getNewVer(local_file) + 1;
    createFileName(file_name, local_file, versionNumber);
    updateNewVer(local_file, versionNumber);
  }

  // Lock the current directory to avoid concurrent modification 
  char *prefix = getFilePrefix(local_file, '/');
  char *lock_path;

  // if the lock already exists (another client is writing to the file), 
  // the function returns an error, thus preventing concurrent writes. (Question 4)
  if (!createLock(prefix, &lock_path))
  {
    sendError(client_sock, "Fail to create file lock");
    return;
  }

  // Open local file
  FILE *filePointer = fopen(file_name, "w");
  if (filePointer == NULL)
  {
    sendError(client_sock, "Error opening remote file for writing");
    return;
  }

  char *buffer;
  int len = receiveText(client_sock, &buffer);
  if (len)
  {
    fwrite(buffer, 1, len, filePointer);
  }
  else
  {
    return;
  }

  // Release the file lock
  if (remove(lock_path) != 0)
  {
    sendError(client_sock, "Error removing the lock");
    return;
  }

  char response[MAX_BUFFER_SIZE];
  sprintf(response, "Successfully writing to file '%s'", file_name);
  sendText(client_sock, response);

  // Free memory 
  free(buffer);
  free(lock_path);
  free(local_file);
  free(file_name);

  // Close the file
  fclose(filePointer);
}

// Question 2
// Function: Get operation from the server side
void operateGet(int client_sock)
{
  // Receive client's remote file path
  char *local_file;
  if (!receiveText(client_sock, &local_file))
  {
    sendError(client_sock, "Error receiving file path for reading");
    return;
  }

  // Get version number of file
  int versionNumber;
  if (recv(client_sock, &versionNumber, sizeof(versionNumber), 0) < 0)
  {
    sendError(client_sock, "Error receiving version number");
    return;
  }
  if (versionNumber == -1)
  {
    // No appointed version number -> use the latest version
    versionNumber = getNewVer(local_file);
  }

  // Get the corresponding version of the given file (Question 7)
  char *file_name = (char *)malloc(strlen(local_file) + 3);
  if (file_name == NULL)
  {
    sendError(client_sock, "Error allocating memory");
    return;
  }

  createFileName(file_name, local_file, versionNumber);

  // Open local file
  FILE *filePointer = fopen(file_name, "r");
  if (filePointer == NULL)
  {
    sendError(client_sock, "Error opening remote file for reading");
    return;
  }

  // Read from the local file and write to the remote file
  char buffer[MAX_BUFFER_SIZE];
  size_t bytesRead = fread(buffer, 1, sizeof(buffer), filePointer);
  if (bytesRead == 0)
  {
    sendError(client_sock, "Error reading data from remote file");
    return;
  }

  buffer[bytesRead] = '\0';
  if (!sendText(client_sock, buffer))
  {
    sendError(client_sock, "Error sending data to client");
    return;
  }

  // Send response to the client
  char response[VER_BUFFER_SIZE];
  sprintf(response, "Successfully reading from file '%s'", file_name);
  sendText(client_sock, response);

  // Free memory and close file
  free(local_file);
  free(file_name);
  fclose(filePointer);
}

// Function: remove operation from the server side
void operateRemove(int client_sock)
{
  // Receive client's remote file path
  char *local_path;
  if (!receiveText(client_sock, &local_path))
  {
    return;
  }

  char *file_name = (char *)malloc(strlen(local_path) + 3);
  if (file_name == NULL)
  {
    sendError(client_sock, "Error allocating memory");
    return;
  }

  // The response to be returned:
  char response[MAX_BUFFER_SIZE];

  // Find all versions of the file to remove
  int versionNumber = getNewVer(local_path);
  for (int i = 0; i <= versionNumber; i++)
  {
    createFileName(file_name, local_path, i);
    if (!isValidFile(file_name))
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
  if (versionNumber > 0)
  {
    removeVersionInfo(local_path);
  }

  // Trim new line character
  response[strlen(response) - 1] = '\0';

  // Send response to the client
  sendText(client_sock, response);

  // Free memory
  free(local_path);
  free(file_name);
}

// Function: list operation from the server side
void operateList(int client_sock)
{
  // Receive client's remote file path
  char *local_file;
  if (!receiveText(client_sock, &local_file))
  {
    return;
  }

  char *file_name = (char *)malloc(strlen(local_file) + 3);
  if (file_name == NULL)
  {
    sendError(client_sock, "Error allocating memory");
    return;
  }

  // The response to be returned:
  char response[MAX_BUFFER_SIZE];
  sprintf(response, "Versioning Information about %s:\n\n", local_file);

  // Find all versions of the file to list
  int versionNumber = getNewVer(local_file);
  for (int v = 0; v <= versionNumber; v++)
  {
    createFileName(file_name, local_file, v);
    if (!isValidFile(file_name))
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
  sendText(client_sock, response);

  // Free memory
  free(local_file);
  free(file_name);
}

// Function: Exit operation from the server side
void operateExit(int client_sock, int socket_desc)
{
  sendText(client_sock, "Server terminated by client");
  shutdown(client_sock, SHUT_RDWR);
  close(client_sock);
  close(socket_desc);
  exit(EXIT_SUCCESS);
}

// Functions: handles each client's request in a multi-threaded TCP server
void *clientTaskExecutor(void *arg)
{
  int *sockets = (int *)arg;
  int client_sock = sockets[0], socket_desc = sockets[1];

  // Receive client's action string
  char *action;
  if (!receiveText(client_sock, &action))
  {
    return NULL;
  }

  if (strcmp(action, "WRITE") == 0)
  { // Question 1
    operateWrite(client_sock);
  }
  else if (strcmp(action, "GET") == 0)
  { // Question 2
    operateGet(client_sock);
  }
  else if (strcmp(action, "RM") == 0)
  { // Question 3
    operateRemove(client_sock);
  }
  else if (strcmp(action, "LS") == 0)
  { // Question 6
    operateList(client_sock);
  }
  else if (strcmp(action, "EXIT") == 0)
  { // Turn off the server
    free(action);
    operateExit(client_sock, socket_desc);
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

// Main function
int main(void)
{
  int socket_desc, client_sock;
  socklen_t client_size;
  struct sockaddr_in server_addr, client_addr;

  // Create socket:
  socket_desc = socket(AF_INET, SOCK_STREAM, 0);

  if (socket_desc < 0)
  {
    errorMsg("Error while creating socket");
  }

  // Set port and IP:
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT_NUMBER);
  char *ip_address = getConfig("IP_ADDRESS");
  if (ip_address == NULL)
  {
    errorMsg("Error retrieving IP address from .config");
  }
  server_addr.sin_addr.s_addr = inet_addr(ip_address);

  // Bind to the set port and IP:
  if (bind(socket_desc, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
  {
    errorMsg("Couldn't bind to the port");
  }
  printf("Done with binding\n");

  // Listen for clients:
  if (listen(socket_desc, 1) < 0)
  {
    errorMsg("Error while listening");
  }
  printf("\nListening for incoming connections.....\n");

  // Accept multiple incoming connections:
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

    // Question 4
    // A new thread is created whenever a new client connection is accepted above (accept function),
    // serving multiple clients simultaneously.
    pthread_t tid;
    int sockets[] = {client_sock, socket_desc};
    if (pthread_create(&tid, NULL, clientTaskExecutor, (void *)sockets) != 0)
    {
      perror("Fail to create thread");
      close(client_sock);
      continue; 
    }

    // Set the thread to a detached state so it can release its 
    // resources automatically upon completion
    pthread_detach(tid);
  }

  close(socket_desc);

  return 0;
}
