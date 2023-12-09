/*
 * client.c -- TCP Socket Client
 *
 * adapted from:
 *   https://www.educative.io/answers/how-to-implement-tcp-sockets-in-c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "helper.h"

#define PORT_NUMBER 1500
#define MAX_BUFFER_SIZE 1024


// Function to create a socket and send the action
int createSocket(const char *action)
{
  int socket_desc;
  struct sockaddr_in server_addr;
  // Create socket:
  socket_desc = socket(AF_INET, SOCK_STREAM, 0);

  if (socket_desc < 0)
  {
    errorMsg("Unable to create socket");
  }

  // Set port and IP the same as server-side:
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT_NUMBER);
  char *ip_address = getConfig("IP_ADDRESS");
  if (ip_address == NULL)
  {
    errorMsg("Unable to retrieve IP address from .config");
  }
  server_addr.sin_addr.s_addr = inet_addr(ip_address);

  // Send connection request to server:
  if (connect(socket_desc, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
  {
    errorMsg("Unable to connect");
  }
  printf("Connected with server successfully\n");

  // Send action type to server
  if (!sendString(socket_desc, action))
  {
    exit(EXIT_FAILURE);
  }

  return socket_desc;
}

// Function to get and display operation response from the server
void getResponse(int sockD)
{
  char *response;
  if (receiveString(sockD, &response))
  {
    printf("Response from the server:\n\"%s\"\n", response);
    free(response);
  }
  else
  {
    exit(EXIT_FAILURE);
  }
}

// Question 1
// Write from the client side
void operateWrite(const char *local_file, const char *remote_file)
{
  // Open the local file
  FILE *filePointer = fopen(local_file, "r");
  if (filePointer == NULL)
  {
    errorMsg("Error opening local file for reading");
  }

  // Create a socket
  int sockD = createSocket("WRITE");
  if (!sendString(sockD, remote_file))
  {
    exit(EXIT_FAILURE);
  }

  // Read from the local file and write to the remote file
  char buffer[MAX_BUFFER_SIZE];
  size_t bytesRead = fread(buffer, 1, sizeof(buffer), filePointer);
  if (bytesRead == 0)
  {
    errorMsg("Error reading data from local file");
  }

  buffer[bytesRead] = '\0';
  if (!sendString(sockD, buffer))
  {
    exit(EXIT_FAILURE);
  }

  // Display the response from the server
  getResponse(sockD);

  // Close file and socket
  fclose(filePointer);
  close(sockD);
}

// Function to handle get operation from the client side
void operateGet(const char *local_file, const char *remote_file, int ver)
{
  // Open local file
  FILE *filePointer = fopen(local_file, "w");
  if (filePointer == NULL)
  {
    errorMsg("Error opening local file for writing");
  }

  // Create a socket
  int sockD = createSocket("GET");

  // Send remote file path
  if (!sendString(sockD, remote_file))
  {
    exit(EXIT_FAILURE);
  }

  // Send version number       
  if (send(sockD, &ver, sizeof(ver), 0) < 0) 
  {
    errorMsg("Error sending version number");
  }

  // Receive data from the server to save
  char *buffer;
  int len = receiveString(sockD, &buffer);
  if (len)
  {
    if (strncmp(buffer, "Error", strlen("Error")) == 0)
    {
      errorMsg(buffer);
    }
    fwrite(buffer, 1, len, filePointer);
  }
  else
  {
    exit(EXIT_FAILURE);
  }

  // Display the response from the server
  getResponse(sockD);

  // Close file and socket
  fclose(filePointer);
  close(sockD);
}

// Function to handle remove operation from the client side
void operateRemove(const char *remote_path)
{
  // Create a socket
  int sockD = createSocket("RM");
  if (!sendString(sockD, remote_path))
  {
    exit(EXIT_FAILURE);
  }

  // Receive a request response
  getResponse(sockD);

  // Close socket
  close(sockD);
}

// Function to handle list operation from the client side
void operateList(const char *remote_file, const char *record_address)
{
  // Create a socket
  int sockD = createSocket("LS");
  if (!sendString(sockD, remote_file))
  {
    exit(EXIT_FAILURE);
  }

  // Receive versioning information from the server
  char *response;
  if (!receiveString(sockD, &response))
  {
    exit(EXIT_FAILURE);
  }

  if (record_address == NULL) // No appointed address, print to stdout
  {
    printf("%s\n", response);
  }
  else
  {
    FILE *filePointer = fopen(record_address, "w");
    if (filePointer == NULL)
    {
      errorMsg("Error opening local file for writing");
    }

    // Redirect output to local file
    fprintf(filePointer, "%s", response);
    fclose(filePointer);
  }

  // Close socket and free memory
  free(response);
  close(sockD);
}

// Function: send a EXIT signal to the server
void operateExit()
{
  // Create a socket
  int sockD = createSocket("EXIT");

  // Receive a request response
  getResponse(sockD);

  // Close socket
  close(sockD);
}

int main(int argc, char *argv[])
{
  // Validate arguments
  if (argc < 2)
  {
    errorMsg("Insufficient arguments");
  }
  char *action = argv[1];

  if (strcmp(action, "WRITE") == 0) // Question 1
  { 
    if (argc == 4)
    {
      operateWrite(argv[2], argv[3]);
    }
    else if (argc == 3) // If remote file path is missing, defaults to local file path
    { 
      operateWrite(argv[2], argv[2]);
    }
    else
    {
      errorMsg("Usage: ./rfs WRITE <local-file-path> <remote-file-path>");
    }
  }
  else if (strcmp(action, "GET") == 0) // Question 2
  { 
    if (strncmp(argv[2], "-v", 2) == 0)
    {
      int v = atoi(argv[2] + 2);
      if (argc == 5)
      {
        operateGet(argv[4], argv[3], v);
      }
      else if (argc == 4)
      {
        operateGet(argv[3], argv[3], v);
      }
      else
      {
        errorMsg("Usage: ./rfs GET -v[number] <remote-file-path> <local-file-path>");
      }
    }
    else
    {
      if (argc == 4)
      {
        operateGet(argv[3], argv[2], -1);
      }
      else if (argc == 3)
      { // Missing local file name defaults to remote file name
        operateGet(argv[2], argv[2], -1);
      }
      else
      {
        errorMsg("Usage: ./rfs GET <remote-file-path> <local-file-path>");
      }
    }
  }
  else if (strcmp(action, "RM") == 0) // Question 3
  { 
    if (argc != 3)
    {
      errorMsg("Usage: ./rfs RM <remote-file-path>");
    }
    operateRemove(argv[2]);
  }
  else if (strcmp(action, "LS") == 0) 
  { 
    if (argc == 3)
    {
      operateList(argv[2], NULL);
    }
    else if (argc == 5)
    {
      if (strcmp(argv[3], ">") != 0)
      {
        errorMsg("Usage: ./rfs LS <remote-file-path> > <local_file_path>");
      }
      operateList(argv[2], argv[4]);
    }
    else
    {
      errorMsg("Usage: ./rfs LS <remote-file-path>");
    }
  }
  else if (strcmp(action, "EXIT") == 0)
  { // Turn off the server
    operateExit();
  }
  else
  {
    errorMsg("Invalid action");
  }

  return 0;
}
