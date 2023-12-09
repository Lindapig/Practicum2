#!/bin/bash

# Clear previous testing data
local_dir="local"
remote_dir="remote_files"
file_version=".file_VERSION"
rm -rf "$local_dir" "$remote_dir"
mkdir "$local_dir"
mkdir "$remote_dir"
truncate -s 0 "$file_version"

# Compile and initiate server
make
./rfserver &

# Test 1: Initial write test
echo -e "\n----Test 1: Initial Write Operation----"

# Setup test file
file_name="write.txt"
local_text="First time write to $file_name"
local_file="$local_dir/$file_name"
remote_file="$remote_dir/$file_name"
printf "%s" "$local_text" >"$local_file"
[ $? -ne 0 ] && echo "Error - Failed to create $local_file"

# Execute write command
./rfs WRITE "$local_file" "$remote_file"
if [ $? -ne 0 ]; then
    echo "Failed: Write operation"
else
    # Validate remote file creation and content (same as local file)
    if [ -e "$remote_file" ]; then
        server_text=$(cat $remote_file)
        if [ "$server_text" == "$local_text" ]; then
            echo "Passed: Remote file content match with local file"
        else
            echo "Failed: Remote file content mismatches with local file"
        fi
    else
        echo "Failed: $remote_file not created on server"
    fi
fi

# Test 2: Second write test for versioning
echo -e "\n----Test 2: Second Write Test----"

# Update local file content 
local_text="Updated $file_name"
printf "%s" "$local_text" >"$local_file"
[ $? -ne 0 ] && echo "Error - Failed to write to $local_file"

# Execute write command again
./rfs WRITE "$local_file" "$remote_file"
if [ $? -ne 0 ]; then
    echo "Failed: Second write operation"
else
    # Check if the original version exists
    ! [ -e "$remote_file" ] && echo "Error - original version not exists"

    # Check remote file is created as another version and if content is matches the local file
    file_name="write_1.txt"
    remote_file="$remote_dir/$file_name"
    if [ -e "$remote_file" ]; then
        server_text=$(cat $remote_file)
        if [ "$server_text" == "$local_text" ]; then
            echo "Passed: Versioned remote file content matches local file"
        else
            echo "Failed: Versioned remote file content mismatches local file"
        fi
    else
        echo "Failed: Versioned $remote_file not created"
    fi
fi

# Test 3: GET operation without specifying version
echo -e "\n----Test 3: GET Operation without Version----"

# Define destination for GET
local_file="$local_dir/get.txt"

# Execute GET command
./rfs GET "$remote_file" "$local_file"
if [ $? -ne 0 ]; then
    echo "Failed: GET operation"
else
    # Check if the local file exists and if its content matches the latest remote file
    remote_file="$remote_dir/write_1.txt"
    if [ -e "$local_file" ]; then
        local_text=$(cat $local_file)
        server_text=$(cat $remote_file)
        if [ "$local_text" == "$server_text" ]; then
            echo "Passed: GET operation content match"
        else
            echo "Failed: GET operation content mismatch"
        fi
    else
        echo "Failed: $local_file not created"
    fi
fi

# Test 4: GET operation with specified version
echo -e "\n----Test 4: GET Operation with Version----"

# Find the original remote version
remote_file="$remote_dir/write.txt"

# Execute GET command with version 0
./rfs GET -v0 "$remote_file" "$local_file"
if [ $? -ne 0 ]; then
    echo "Failed: GET with version operation"
else
    # Check if the local file exists and if its content matches the previous remote file
    if [ -e "$local_file" ]; then
        local_text=$(cat $local_file)
        server_text=$(cat $remote_file)
        if [ "$local_text" == "$server_text" ]; then
            echo "Passed: GET with version content match"
        else
            echo "Failed: GET with version content mismatch"
        fi
    else
        echo "Failed: Versioned file not retrieved"
    fi
fi

# Test 5: Listing versions of a file
echo -e "\n----Test 5: Listing File Versions (LS)----"

# Define local output
local_file="$local_dir/ls.txt"

# Run the rfs command
./rfs LS "$remote_file" >"$local_file"
if [ $? -ne 0 ]; then
    echo "Failed: LS operation"
else
    # Check if the local output exists
    if [ -e "$local_file" ]; then
        if grep -q "v0" "$local_file" && grep -q "v1" "$local_file"; then
            echo "Passed: Correct version details listed"
        else
            echo "Failed: Incorrect version details"
        fi
    else
        echo "Failed: $local_file not created"
    fi
fi

# Test 6: Remove versioned file
echo -e "\n----Test 6: Remove versioned file (RM)----"

remote_file="$remote_dir/write.txt"

# Execute RM command
./rfs RM "$remote_file"
if [ $? -ne 0 ]; then
    echo "Failed: RM operation"
else
    # Verify remote file removal
    if [ -e "$remote_file" ]; then
        echo "Failed: The remote file still exists at $remote_file"
    else
        echo "Passed: The original version of remote file is removed by rfs RM"
        remote_file="$remote_dir/write_1.txt"
        if [ -e "$remote_file" ]; then
            echo "Failed: Some versions still exist"
        else
            echo "Passed: All versions successfully removed"
        fi
    fi
fi

# Test 7: Folder Removal Test
echo -e "\n----Test 7: Folder Removal Test(RM)----"

# Setup folder and file for removal test
folder_name="rm_folder"
file_name="test_rm.txt"
remote_path="$remote_dir/$folder_name"
mkdir "$remote_path"
touch "$remote_path/$file_name"
! [ -d "$remote_path" ] && echo "Failed: $remote_path not created"

# Execute RM command for folder
./rfs RM "$remote_path"
if [ $? -ne 0 ]; then
    echo "Failed: Folder RM operation"
else
    # Check if the remote folder is successfully removed
    if [ -d "$remote_path" ]; then
        echo "Failed: Remote folder still exists at $remote_path"
    else
        echo "Passed: Remote folder is removed by rfs RM"
    fi
fi

# Test 8: Server EXIT
echo -e "\n----Test 8: Server EXIT Test----"

# Execute EXIT command
./rfs EXIT
if [ $? -eq 0 ]; then
    echo "Passed: Server is terminated by rfs EXIT"
else
    echo "Failed: Fail to perform rfs EXIT"
fi

make clean
