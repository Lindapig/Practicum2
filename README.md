# Instructions
1. Command line arguments to write the file to the server: `./rfs WRITE local-file-path remote-file-path`. If the remote file or path is omitted, use the values for the first argument (using the local file path for remote file path). 

e.g., './rfs WRITE local/write.txt remote_files/write.txt' (Question 1)
if update the content in file write.txt, then operate 'WRITE', the remote file will automatically update to higher numbered version with the new content will still keep the old content in the old file. (Question 5)

2. Implement a command that retrieves a new file from the remote file system, and writes the data read from the socket to a local file: `./rfs GET remote-file-path local-file-path`. If the local file path or name (the third command line argument) is omitted, use current folder. (Question 2)

Add "-v[digit]" after "GET"  to request a specific version of a file: `./rfs GET -v1 remote-file_path local_file_path` (Question 7)

e.g., './rfs GET remote_files/write.tx local/get.txt'(Question 2)
e.g., './rfs GET -v1 remote_files/write.txt local/get.txt' (Question 7)

3. Implement a command that deletes a file or folder in the remote file system: `./rfs RM remote-file-path`.(Question 3)

4. Gets all versioning information about a file, i.e., the name of the file and all timestamps when the versions were last written to: `./rfs LS remote-file-path`.  (Question 6)
(`./rfs LS remote-file-path local-file-path` can output the result to a file.)

5. tests.sh: shell script designed for testing a set of functionalities in a client-server model. After 
`make` and `./rfserver`, input on terminal: `chmod +x tests.sh`, `/tests.sh`.

6. When you stop a process with CTRL-C, it'll exit by default leaving ports open and potentially data unset. So, it is best to "catch" or "trap" the SIGINT signal and add your own behavior so you can do a "safe" exit:
`./rfs EXIT`