#!/bin/bash

# Define the server URL and port


# Use a loop to make concurrent requests
for i in {1..200}; do

  curl --insecure -s -o /dev/null -X POST -d 'printf("test")' -H 'Content-Type: text/plain' https://localhost:30000/dir1/dir2/pyTest$i.py -w "Return Code: %{http_code}\n"
  curl --insecure -s -o /dev/null https://localhost:30000/dir1/dir2/pyTest$i.py -w "Return Code: %{http_code}\n"
  curl --insecure -s -o /dev/null -I https://localhost:30000/dir1/dir2/pyTest$i.py -w "Return Code: %{http_code}\n"
  curl --insecure -s -o /dev/null -X DELETE https://localhost:30000/dir1/dir2/pyTest$i.py -w "Return Code: %{http_code}\n"


done

# Wait for all requests to finish
wait

