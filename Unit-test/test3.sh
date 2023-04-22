#!/bin/bash

curl  --insecure -s -o /dev/null -X POST -d 'html' -H 'Content-Type: text/plain'  https://localhost:30000/dir1/dir2/test3.html -w "Return Code: %{http_code}\n"

curl --insecure -s -o /dev/null https://localhost:30000/dir1/dir2/test3.html -w "Return Code: %{http_code}\n"

curl --insecure -s -o /dev/null -I https://localhost:30000/dir1/dir2/test3.html -w "Return Code: %{http_code}\n"


curl --insecure -s -o /dev/null -X DELETE https://localhost:30000/dir1/dir2/test3.html -w "Return Code: %{http_code}\n"
