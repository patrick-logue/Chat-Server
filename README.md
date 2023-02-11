# Chat-Server

Reverse engineered chat server using tcpdumbp and WireShark. 

The software allows for private messages as well as chatrooms.

### Installation
Run `make` to compile the server

### Usage
To start a server: `./rserver -p <Port Number>`

To start a client: `./client`

Client commands:

`\connect <IP Address>:<Port>`

`\disconnect`

`\join <Room> <Password>`

`\leave`

`\list users`

`\list rooms`

`\msg <User> <Message>`

`\nick <Name>`

`\quit`
