//-------------------------Compile-------------------------//

gcc -o test_server test_server.c helper_functions.c queue.c  -lssl -lcrypto -lpthread -Wno-deprecated-declarations

//-------------------------Configue------------------------//

Inside config.txt you can configure the number of threads, server port, home folder and the hostname of the https server.

//-------------------------Run------------------------//

./tls_server

//-------------------------Requests------------------------//

Supported request types:
	GET , POST, DELETE, HEAD

Request message structure:

---Header---
	‘Type’ ‘file/path.extention’ HTTP/1.1
Connection: keep-alive/close
User-Agent: *
Host: <server_addr>:<serv_port>

--Body(if any)—
	
Reply message structure:

---Header---
HTTP/1.1 200/404/501 OK/Not Found/Not Implemented
Server: My_test_server
Content-Length: *body size(int)
Connection: keep-alive/lose
Content-Type: (text/x-php , text/plain, text/html, text/x-php, application/x-python-code, image/jpeg, image/jpeg,  application/pdf)

--Body(if any)--	

//-------------------------Clients------------------------//

Server allows requests from all kind of clients as long as they follow the messaging structure.

Tls_client

Postman

Curl
