# Online-Messenger

* Computer Networks Final Project [[Report]](/Report.pdf)

* Lecturer: Prof. Cheng-Fu Chou

## Introduction
This Project contains two executable files: `client` and `server`, the server will listen to any connection request through TCP socket and handle the message between clients. The client will register on the client first, and then send messages to any other clients.

There are other features: 
* The offline user's `unread messages` would be auto-saved by the server and will be restored when the user re-login.
* Support `multiple file transfer`.
* All messages and files will be encrypted by `AES algorithm` when transferring. 
* When connection breaks, the client would auto reconnect, not having to be restarted.

## Techniques
C++, Linux, TCP socket programming, AES encryption

## Build and Clean

### Build all
```
make
```
### Only compile the client or server
```
make client

make server
```

### Clean all executable files
```
make clean
```

## Execution
All executable files will be in the `./bin` folder

### Run server
```
make runs
```
Then the server will listen to the connection.

### Run client (on another machine)
Before running, open the `Makefile` to change the `IP` variable to the IP address of the server:
```Makefile
IP=127.0.0.1 #Change this 
```
and then run the client
```
make runs
```
Then the client will send a connection request to the server.


> **Change the port number**
> 
>  The default port number is 19955, defined in the file`./src/connect.h`
> 
> 
> `#define PORT_NUM 19955`
> 
> 
> After changing , please `make clean` and re-compile.

## Usage

### Login or Register
* `Left`: Register a new user / `Right`: Login to an exist account

  <img src="/img/login_register.PNG" alt="AES" height="250">

* Choose a user to send messages. (`Status = 1` : online / `Status = 0` : offline)
  
  <img src="/img/choose_client.PNG" alt="AES" height="200">

### Messaging

#### Single Line Message (Text Mode)
* After the dialogue opens, we can type a message and press `Enter` to send it.

  <img src="/img/send_message.PNG" alt="AES" width="500">

* And the other user will receive the message.

  <img src="/img/receive_message.PNG" alt="AES" width="500">

#### Multiple Line Message (Long Text Mode)
* Press `F1` to switch between `Text Mode` and `Long Text Mode`.

  <img src="/img/long_text_mode.PNG" alt="AES" width="500">

* After typing messages in the `Long Text Mode`, press `F1` again to back to `Text Mode` then press `enter` to send it.

#### Message Log and Restore
* If sending messages to the offline user, the server will save them in the message queue.

  <img src="/img/message_log.PNG" alt="AES" width="500">

* After the user re-login, the queued messages will be restored on its screen.

* On the server side, the server will display the restored message

  <img src="/img/restore_message.PNG" alt="AES" width="500">
  
* If the server closes, it will save the unsent messages to a file so they won't disappear.

  <img src="/img/save_message_log.PNG" alt="AES" width="500">

### File Transfer
#### Single File Transfer
* Press `F2` to switch to the `File Mode`, and enter the name of the file.
* The client will upload the file in the `./upload` folder.

	<img src="/img/file_mode.PNG" alt="AES" width="500">
    
* And the other user will receive the file which will be saved in the `./download` folder.

 	<img src="/img/receive_file.PNG" alt="AES" height="500">

#### Multiple File Transfer
* In the `File Mode` we can also send multiple files, separated by a single space.

 	<img src="/img/multiple_file_mode.PNG" alt="AES" width="300">

* And the other user will receive all the files which will be saved in the `./download` folder.

	<img src="/img/receive_multiple_file.PNG" alt="AES" height="500">

### Auto Reconnect (Bonus)
* When the connection breaks, the client will ask if send connection request again to the server.

	<img src="/img/auto_reconnect.PNG" alt="AES" width="500">
    
### AES Encryption (Bonus)
* All messages and files will be encrypted by AES-256 algorithm before sending and be decrypted when receiving to the other user.

	<img src="/img/aes.PNG" alt="AES" width="500">
