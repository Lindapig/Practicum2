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
#include "helpers.h"

#define MAX_BUFFER_SIZE 1024
#define PORT_NUMBER 2000

// Function to create a socket and send the action
int createSocket(const char *action)
{
  int socket_desc;
  struct sockaddr_in server_addr;
  // Create socket:
  socket_desc = socket(AF_INET, SOCK_STREAM, 0);

  if (socket_desc < 0)
  {
    error("Unable to create socket");
  }

  // Set port and IP the same as server-side:
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT_NUMBER);
  char *ip_address = getConfigVar("IP_ADDRESS");
  if (ip_address == NULL)
  {
    error("Unable to retrieve IP address from .config");
  }
  server_addr.sin_addr.s_addr = inet_addr(ip_address);

  // Send connection request to server:
  if (connect(socket_desc, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
  {
    error("Unable to connect");
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
void getResponse(int sockfd)
{
  char *response;
  if (receiveString(sockfd, &response))
  {
    printf("Response from the server:\n\"%s\"\n", response);
    free(response);
  }
  else
  {
    exit(EXIT_FAILURE);
  }
}

// Function to handle write operation from the client side
void handleWrite(const char *local_file, const char *remote_file)
{
  // Open local file
  FILE *fp = fopen(local_file, "r");
  if (fp == NULL)
  {
    error("Error opening local file for reading");
  }

  // Create a socket
  int sockfd = createSocket("WRITE");
  if (!sendString(sockfd, remote_file))
  {
    exit(EXIT_FAILURE);
  }

  // Read from the local file and write to the remote file
  char buffer[MAX_BUFFER_SIZE];
  size_t bytesRead = fread(buffer, 1, sizeof(buffer), fp);
  if (bytesRead == 0)
  {
    error("Error reading data from local file");
  }

  buffer[bytesRead] = '\0';
  if (!sendString(sockfd, buffer))
  {
    exit(EXIT_FAILURE);
  }

  // Display the response from the server
  getResponse(sockfd);

  // Close file and socket
  fclose(fp);
  close(sockfd);
}

// Function to handle get operation from the client side
void handleGet(const char *local_file, const char *remote_file, int ver)
{
  // Open local file
  FILE *fp = fopen(local_file, "w");
  if (fp == NULL)
  {
    error("Error opening local file for writing");
  }

  // Create a socket
  int sockfd = createSocket("GET");

  // Send remote file path
  if (!sendString(sockfd, remote_file))
  {
    exit(EXIT_FAILURE);
  }

  // Send version number       
  if (send(sockfd, &ver, sizeof(ver), 0) < 0) 
  {
    error("Error sending version number");
  }

  // Receive data from the server to save
  char *buffer;
  int len = receiveString(sockfd, &buffer);
  if (len)
  {
    if (strncmp(buffer, "Error", strlen("Error")) == 0)
    {
      error(buffer);
    }
    fwrite(buffer, 1, len, fp);
  }
  else
  {
    exit(EXIT_FAILURE);
  }

  // Display the response from the server
  getResponse(sockfd);

  // Close file and socket
  fclose(fp);
  close(sockfd);
}

// Function to handle remove operation from the client side
void handleRemove(const char *remote_path)
{
  // Create a socket
  int sockfd = createSocket("RM");
  if (!sendString(sockfd, remote_path))
  {
    exit(EXIT_FAILURE);
  }

  // Receive a request response
  getResponse(sockfd);

  // Close socket
  close(sockfd);
}

// Function to handle list operation from the client side
void handleList(const char *remote_file, const char *record_address)
{
  // Create a socket
  int sockfd = createSocket("LS");
  if (!sendString(sockfd, remote_file))
  {
    exit(EXIT_FAILURE);
  }

  // Receive versioning information from the server
  char *response;
  if (!receiveString(sockfd, &response))
  {
    exit(EXIT_FAILURE);
  }

  if (record_address == NULL) // No appointed address, print to stdout
  {
    printf("%s\n", response);
  }
  else
  {
    FILE *fp = fopen(record_address, "w");
    if (fp == NULL)
    {
      error("Error opening local file for writing");
    }

    // Redirect output to local file
    fprintf(fp, "%s", response);
    fclose(fp);
  }

  // Close socket and free memory
  free(response);
  close(sockfd);
}

// Function to send a STOP signal to the server
void handleStop()
{
  // Create a socket
  int sockfd = createSocket("STOP");

  // Receive a request response
  getResponse(sockfd);

  // Close socket
  close(sockfd);
}

int main(int argc, char *argv[])
{
  // Validate arguments
  if (argc < 2)
  {
    error("Insufficient arguments");
  }
  char *action = argv[1];

  if (strcmp(action, "WRITE") == 0)
  { // Question 1
    if (argc == 4)
    {
      handleWrite(argv[2], argv[3]);
    }
    else if (argc == 3)
    { // Missing remote file name defaults to local file name
      handleWrite(argv[2], argv[2]);
    }
    else
    {
      error("Usage: ./rfs WRITE <local-file-path> <remote-file-path>");
    }
  }
  else if (strcmp(action, "GET") == 0)
  { // Question 2
    if (strncmp(argv[2], "-v", 2) == 0)
    {
      int v = atoi(argv[2] + 2);
      if (argc == 5)
      {
        handleGet(argv[4], argv[3], v);
      }
      else if (argc == 4)
      {
        handleGet(argv[3], argv[3], v);
      }
      else
      {
        error("Usage: ./rfs GET -v[number] <remote-file-path> <local-file-path>");
      }
    }
    else
    {
      if (argc == 4)
      {
        handleGet(argv[3], argv[2], -1);
      }
      else if (argc == 3)
      { // Missing local file name defaults to remote file name
        handleGet(argv[2], argv[2], -1);
      }
      else
      {
        error("Usage: ./rfs GET <remote-file-path> <local-file-path>");
      }
    }
  }
  else if (strcmp(action, "RM") == 0)
  { // Question 3
    if (argc != 3)
    {
      error("Usage: ./rfs RM <remote-file-path>");
    }
    handleRemove(argv[2]);
  }
  else if (strcmp(action, "LS") == 0)
  { // Question 6
    if (argc == 3)
    {
      handleList(argv[2], NULL);
    }
    else if (argc == 5)
    {
      if (strcmp(argv[3], ">") != 0)
      {
        error("Usage: ./rfs LS <remote-file-path> > <local_file_path>");
      }
      handleList(argv[2], argv[4]);
    }
    else
    {
      error("Usage: ./rfs LS <remote-file-path>");
    }
  }
  else if (strcmp(action, "STOP") == 0)
  { // Turn off the server
    handleStop();
  }
  else
  {
    error("Invalid action");
  }

  return 0;
}
