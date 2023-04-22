#!/bin/bash

# Define the server URL and port
SERVER="localhost"
PORT="30000"

# Define the URL to be used in the requests
URL="https://${SERVER}:${PORT}/dir1/dir2/test.txt"

# Use a loop to make 40 concurrent requests
for i in {1..40}; do
  curl --insecure -s -o /dev/null -w "%{http_code}\n" $URL &
done

# Wait for all requests to finish
wait

