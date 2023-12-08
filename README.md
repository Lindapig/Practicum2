# How to manipulate remote files on the server via client commands
1. Write a local file from the client to the server: `./rfs WRITE <local-file-path> <remote-file-path>`. If the remote file path is omitted, use the same path as the local file by default. If such a file name already exists on the server side, the server will automatically create a new version with the highest version number (e.g., writing to result.txt creates result_1.txt). 

e.g., './rfs WRITE local/write.txt remote_files/write.txt' (Question 1)
if update the content in file write.txt, then operate 'WRITE', the remote file will automatically update to another version with the new content will still keep the old content in the old file. (Question 5)

2. Retrieve a remote file from the server and write it to the client: `./rfs GET <remote-file-path> <local-file-path>`. If the local file path or name is omitted, use the current folder. You can also add "-v[number]" after "GET" (e.g., `./rfs GET -v2 <remote_file_path> <local-file-path>`) to request a specific version of a file. 

e.g., './rfs GET remote_files/write.tx local/get.txt'(Question 2)
e.g., './rfs GET -v1 remote_files/write.txt local/get.txt' (Question 7)

3. Delete a file or folder in the remote file system: `./rfs RM <remote-file-path>`. This will delete the file and its other versions. (Question 3)

4. Gets all versioning information about a file, i.e., the name of the file and all timestamps when the versions were last written to. e.g., `./rfs LS <remote-file-path>`. 
(`./rfs LS <remote-file-path> <local-file-path>` can output the result to a file.)

5. tests.sh: shell script designed for testing a set of functionalities in a client-server model. After 'make', input on terminal: 'chmod +x tests.sh', './tests.sh'.