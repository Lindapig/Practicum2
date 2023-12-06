#!/bin/bash

# Remove previous test results
local_folder="local_files"
remote_folder="remote_files"
version_file=".file_VERSION"
rm -rf "$local_folder" "$remote_folder"
mkdir "$local_folder"
mkdir "$remote_folder"
truncate -s 0 "$version_file"

# Make and start the server
make
./rfserver &

# Test 1: rfs WRITE for 1st time
echo -e "\n----Test 1: Testing rfs WRITE 1st time----"

# Create the file used for testing
file_name="test_write.txt"
local_content="This is the content of $file_name"
local_file="$local_folder/$file_name"
remote_file="$remote_folder/$file_name"
printf "%s" "$local_content" >"$local_file"
[ $? -ne 0 ] && echo "Error - Failed to create $local_file"

# Run the rfs command
./rfs WRITE "$local_file" "$remote_file"
if [ $? -ne 0 ]; then
    echo "(X)Error - Failed to perform rfs WRITE"
else
    # Check if the remote file exists and if its content matches the local file
    if [ -e "$remote_file" ]; then
        remote_content=$(cat $remote_file)
        if [ "$remote_content" == "$local_content" ]; then
            echo "(√)Success - The remote file has the same content as the local file"
        else
            echo "(X)Failure - The content of remote file does not match the local file"
        fi
    else
        echo "(X)Error - Failed to create $remote_file"
    fi
fi

# Test 2: rfs WRITE for 2nd time to test versioning
echo -e "\n----Test 2: Testing rfs WRITE 2nd time----"

# Update the content of local file for testing
local_content="This is the updated content of $file_name"
printf "%s" "$local_content" >"$local_file"
[ $? -ne 0 ] && echo "Error - Failed to write to $local_file"

# Run the rfs command
./rfs WRITE "$local_file" "$remote_file"
if [ $? -ne 0 ]; then
    echo "(X)Error - Failed to perform rfs WRITE"
else
    # Check if the original version still exists
    ! [ -e "$remote_file" ] && echo "Error - The original version no longer exists"

    # Check if the remote file exists as another version and if its content matches the local file
    file_name="test_write_1.txt"
    remote_file="$remote_folder/$file_name"
    if [ -e "$remote_file" ]; then
        remote_content=$(cat $remote_file)
        if [ "$remote_content" == "$local_content" ]; then
            echo "(√)Success - The remote file has the same content as the local file"
        else
            echo "(X)Failure - The content of remote file does not match the local file"
        fi
    else
        echo "(X)Error - Failed to create $remote_file"
    fi
fi

# Test 3: rfs GET without a version
echo -e "\n----Test 3: Testing rfs GET without a version----"

# Define local output
local_file="$local_folder/test_get.txt"

# Run the rfs command
./rfs GET "$remote_file" "$local_file"
if [ $? -ne 0 ]; then
    echo "(X)Error - Failed to perform rfs GET"
else
    # Check if the local file exists and if its content matches the latest remote file
    remote_file="$remote_folder/test_write_1.txt"
    if [ -e "$local_file" ]; then
        local_content=$(cat $local_file)
        remote_content=$(cat $remote_file)
        if [ "$local_content" == "$remote_content" ]; then
            echo "(√)Success - The local file has the same content as the updated remote file"
        else
            echo "(X)Failure - The content of local file does not match the updated remote file"
        fi
    else
        echo "(X)Error - Failed to create $local_file"
    fi
fi

# Test 4: rfs GET with a specific version
echo -e "\n----Test 4: Testing rfs GET with a specific version----"

# Find the original remote version
remote_file="$remote_folder/test_write.txt"

# Run the rfs command
./rfs GET -v0 "$remote_file" "$local_file"
if [ $? -ne 0 ]; then
    echo "(X)Error - Failed to perform rfs GET"
else
    # Check if the local file exists and if its content matches the previous remote file
    if [ -e "$local_file" ]; then
        local_content=$(cat $local_file)
        remote_content=$(cat $remote_file)
        if [ "$local_content" == "$remote_content" ]; then
            echo "(√)Success - The local file has the same content as the previous remote file"
        else
            echo "(X)Failure - The content of local file does not match the previous remote file"
        fi
    else
        echo "(X)Error - Failed to create $local_file"
    fi
fi

# Test 5: rfs LS with a versioned file
echo -e "\n----Test 5: Testing rfs LS with a versioned file----"

# Define local output
local_file="$local_folder/test_ls.txt"

# Run the rfs command
./rfs LS "$remote_file" >"$local_file"
if [ $? -ne 0 ]; then
    echo "(X)Error - Failed to perform rfs LS"
else
    # Check if the local output exists
    if [ -e "$local_file" ]; then
        if grep -q "v0" "$local_file" && grep -q "v1" "$local_file"; then
            echo "(√)Success - The LS file output has up-to-date version info"
        else
            echo "(X)Failure - The LS file output doesn't have up-to-date version info"
        fi
    else
        echo "(X)Error - Failed to create $local_file"
    fi
fi

# Test 6: rfs RM with a versioned file
echo -e "\n----Test 6: Testing rfs RM with a versioned file----"

remote_file="$remote_folder/test_write.txt"

# Run the rfs command
./rfs RM "$remote_file"
if [ $? -ne 0 ]; then
    echo "(X)Error - Failed to perform rfs RM"
else
    # Check if the remote file is successfully removed
    if [ -e "$remote_file" ]; then
        echo "(X)Failure - The remote file still exists at $remote_file"
    else
        echo "(√)Success - The original version of remote file is removed by rfs RM"
        remote_file="$remote_folder/test_write_1.txt"
        if [ -e "$remote_file" ]; then
            echo "(X)Failure - The remote file still exists at $remote_file"
        else
            echo "(√)Success - The latest version of remote file is removed by rfs RM"
        fi
    fi
fi

# Test 7: rfs RM with folder
echo -e "\n----Test 7: Testing rfs RM with folder----"

# Create the folder and file used for testing
folder_name="rm_folder"
file_name="test_rm.txt"
remote_path="$remote_folder/$folder_name"
mkdir "$remote_path"
touch "$remote_path/$file_name"
! [ -d "$remote_path" ] && echo "(X)Error - Failed to create $remote_path"

# Run the rfs command
./rfs RM "$remote_path"
if [ $? -ne 0 ]; then
    echo "(X)Error - Failed to perform rfs RM"
else
    # Check if the remote folder is successfully removed
    if [ -d "$remote_path" ]; then
        echo "(X)Failure  - The remote folder still exists at $remote_path"
    else
        echo "(√)Success - The remote folder is removed by rfs RM"
    fi
fi

# Test 8: rfs STOP
echo -e "\n----Test 8: Testing rfs STOP----"

# Run the rfs command
./rfs STOP
if [ $? -eq 0 ]; then
    echo "(√)Success - The server is terminated by rfs STOP"
else
    echo "(X)Error - Unable to perform rfs STOP"
fi

make clean
