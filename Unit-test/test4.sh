#!/bin/bash

curl  --insecure -s -o /dev/null -X POST -d 'Gif' -H 'Content-Type: text/plain'  https://localhost:30000/dir1/dir2/test4.gif -w "Return Code: %{http_code}\n"

curl --insecure -s -o /dev/null https://localhost:30000/dir1/dir2/test4.gif -w "Return Code: %{http_code}\n"

curl --insecure -s -o /dev/null -I https://localhost:30000/dir1/dir2/test4.gif -w "Return Code: %{http_code}\n"


curl --insecure -s -o /dev/null -X DELETE https://localhost:30000/dir1/dir2/test4.gif -w "Return Code: %{http_code}\n"
