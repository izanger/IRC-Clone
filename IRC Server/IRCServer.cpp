
const char * usage =
"                                                               \n"
"IRCServer:                                                   \n"
"                                                               \n"
"Simple server program used to communicate multiple users       \n"
"                                                               \n"
"To use it in one window type:                                  \n"
"                                                               \n"
"   IRCServer <port>                                          \n"
"                                                               \n"
"Where 1024 < port < 65536.                                     \n"
"                                                               \n"
"In another window type:                                        \n"
"                                                               \n"
"   telnet <host> <port>                                        \n"
"                                                               \n"
"where <host> is the name of the machine where talk-server      \n"
"is running. <port> is the port number you used when you run    \n"
"daytime-server.                                                \n"
"                                                               \n";

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <vector>
#include <map>
#include <string>
#include <algorithm>

#include "IRCServer.h"

using namespace std;

int QueueLength = 5;
map<string,string> userMap;
vector<string> activeRooms;
map<string, vector<string> > roomUserMap;
map<string, vector<string> > roomMessageMap;
//test

int
IRCServer::open_server_socket(int port) {

	// Set the IP address and port for this server
	struct sockaddr_in serverIPAddress; 
	memset( &serverIPAddress, 0, sizeof(serverIPAddress) );
	serverIPAddress.sin_family = AF_INET;
	serverIPAddress.sin_addr.s_addr = INADDR_ANY;
	serverIPAddress.sin_port = htons((u_short) port);
  
	// Allocate a socket
	int masterSocket =  socket(PF_INET, SOCK_STREAM, 0);
	if ( masterSocket < 0) {
		perror("socket");
		exit( -1 );
	}

	// Set socket options to reuse port. Otherwise we will
	// have to wait about 2 minutes before reusing the sae port number
	int optval = 1; 
	int err = setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, 
			     (char *) &optval, sizeof( int ) );
	
	// Bind the socket to the IP address and port
	int error = bind( masterSocket,
			  (struct sockaddr *)&serverIPAddress,
			  sizeof(serverIPAddress) );
	if ( error ) {
		perror("bind");
		exit( -1 );
	}
	
	// Put socket in listening mode and set the 
	// size of the queue of unprocessed connections
	error = listen( masterSocket, QueueLength);
	if ( error ) {
		perror("listen");
		exit( -1 );
	}

	return masterSocket;
}

void
IRCServer::runServer(int port)
{
	int masterSocket = open_server_socket(port);

	initialize();
	
	while ( 1 ) {
		
		// Accept incoming connections
		struct sockaddr_in clientIPAddress;
		int alen = sizeof( clientIPAddress );
		int slaveSocket = accept( masterSocket,
					  (struct sockaddr *)&clientIPAddress,
					  (socklen_t*)&alen);
		
		if ( slaveSocket < 0 ) {
			perror( "accept" );
			exit( -1 );
		}
		
		// Process request.
		processRequest( slaveSocket );		
	}
}

int
main( int argc, char ** argv )
{
	// Print usage if not enough arguments
	if ( argc < 2 ) {
		fprintf( stderr, "%s", usage );
		exit( -1 );
	}
	
	// Get the port from the arguments
	int port = atoi( argv[1] );

	IRCServer ircServer;

	// It will never return
	ircServer.runServer(port);
	
}

//
// Commands:
//   Commands are started y the client.
//
//   Request: ADD-USER <USER> <PASSWD>\r\n
//   Answer: OK\r\n or DENIED\r\n
//
//   REQUEST: GET-ALL-USERS <USER> <PASSWD>\r\n
//   Answer: USER1\r\n
//            USER2\r\n
//            ...
//            \r\n
//
//   REQUEST: CREATE-ROOM <USER> <PASSWD> <ROOM>\r\n
//   Answer: OK\n or DENIED\r\n
//
//   Request: LIST-ROOMS <USER> <PASSWD>\r\n
//   Answer: room1\r\n
//           room2\r\n
//           ...
//           \r\n
//
//   Request: ENTER-ROOM <USER> <PASSWD> <ROOM>\r\n
//   Answer: OK\n or DENIED\r\n
//
//   Request: LEAVE-ROOM <USER> <PASSWD>\r\n
//   Answer: OK\n or DENIED\r\n
//
//   Request: SEND-MESSAGE <USER> <PASSWD> <MESSAGE> <ROOM>\n
//   Answer: OK\n or DENIED\n
//
//   Request: GET-MESSAGES <USER> <PASSWD> <LAST-MESSAGE-NUM> <ROOM>\r\n
//   Answer: MSGNUM1 USER1 MESSAGE1\r\n
//           MSGNUM2 USER2 MESSAGE2\r\n
//           MSGNUM3 USER2 MESSAGE2\r\n
//           ...\r\n
//           \r\n
//
//    REQUEST: GET-USERS-IN-ROOM <USER> <PASSWD> <ROOM>\r\n
//    Answer: USER1\r\n
//            USER2\r\n
//            ...
//            \r\n
//

void
IRCServer::processRequest( int fd )
{
	// Buffer used to store the comand received from the client
	const int MaxCommandLine = 1024;
	char commandLine[ MaxCommandLine + 1 ];
	int commandLineLength = 0;
	int n;
	
	// Currently character read
	unsigned char prevChar = 0;
	unsigned char newChar = 0;
	
	//
	// The client should send COMMAND-LINE\n
	// Read the name of the client character by character until a
	// \n is found.
	//

	// Read character by character until a \n is found or the command string is full.
	while ( commandLineLength < MaxCommandLine &&
		read( fd, &newChar, 1) > 0 ) {

		if (newChar == '\n' && prevChar == '\r') {
			break;
		}
		
		commandLine[ commandLineLength ] = newChar;
		commandLineLength++;

		prevChar = newChar;
	}
	
	// Add null character at the end of the string
	// Eliminate last \r
	commandLineLength--;
        commandLine[ commandLineLength ] = 0;

	printf("RECEIVED: %s\n", commandLine);

//	printf("The commandLine has the following format:\n");
//	printf("COMMAND <user> <password> <arguments>. See below.\n");
//	printf("You need to separate the commandLine into those components\n");
//	printf("For now, command, user, and password are hardwired.\n");
///////////////parse real values here
	string s(commandLine);
	string com = s.substr(0, s.find_first_of(" "));
	s = s.substr(s.find_first_of(" ")+1);
	
	string usern = s.substr(0, s.find_first_of(" "));
	s = s.substr(s.find_first_of(" ")+1);

	string pw = s.substr(0, s.find_first_of(" "));
	s = s.substr(s.find_first_of(" ")+1);
	
	char * command = new char[com.length()+1];
	char * user = new char[usern.length()+1];
	char * password = new char[pw.length()+1];
	char * args = new char[s.length()+1];

	strcpy(command, com.c_str());
	strcpy(user, usern.c_str());
	strcpy(password, pw.c_str());
	strcpy(args, s.c_str());

	//delete stuff?

	/*
	char * command = new char[20];
	int i = 0;
	for(;commandLine[i] != ' '; i++){
		if(i > 16){
			command[0] = ' ';
			command[1] = '\0';
			break;
		}
		command[i] = commandLine[i];
	}
	command[i] = '\0';
	i++;
	
	
	
	
	char * user = new char[50];
	int k = i;
	for(;commandLine[i] != ' ';i++){
		user[i-k] = commandLine[i];
	}
	user[i-k] = '\0';
	i++;

	char * password = new char[50];
	k = i;
	for(;commandLine[i] != ' ';i++){
		password[i-k] = commandLine[i];
	}
	password[i-k] = '\0';
	i++;

	char * args = new char[100];
	k = i;
	for(;i < commandLineLength; i++){
		args[i-k] = commandLine[i];
	}
	args[i-k] = '\0';
	
	*/

//	printf("command=%s\n", command);
//	printf("user=%s\n", user);
//	printf( "password=%s\n", password );
//	printf("args=%s\n", args);

	if (!strcmp(command, "ADD-USER")) {
		addUser(fd, user, password, args);
	}
	else if (!strcmp(command, "ENTER-ROOM")) {
		enterRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "LEAVE-ROOM")) {
		leaveRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "SEND-MESSAGE")) {
		sendMessage(fd, user, password, args);
	}
	else if (!strcmp(command, "GET-MESSAGES")) {
		getMessages(fd, user, password, args);
	}
	else if (!strcmp(command, "GET-USERS-IN-ROOM")) {
		getUsersInRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "GET-ALL-USERS")) {
		getAllUsers(fd, user, password, args);
	}
	else if(!strcmp(command, "CREATE-ROOM")) {
		createRoom(fd, user, password, args);
	}
	else if(!strcmp(command, "LIST-ROOMS")) {
		listRooms(fd, user, password, args);
	}
	else {
		const char * msg =  "UNKNOWN COMMAND\r\n";
		write(fd, msg, strlen(msg));
	}

	// Send OK answer
	//const char * msg =  "OK\n";
	//write(fd, msg, strlen(msg));
	
	delete [] command;
	delete [] password;
	delete [] args;
	delete [] user;
	close(fd);	
}

void
IRCServer::initialize()
{
	// Open password file

	// Initialize users in room

	// Initalize message list

}

bool
IRCServer::checkPassword(int fd, const char * user, const char * password) {
	// Here check the password
//	dprintf(fd, "pw a\n");
	string username(user);
	string pw(password);
//	dprintf(fd, "pw b\n");

	if((userMap.find(username) == userMap.end()) || (userMap[username] != pw)){
//		dprintf(fd, "pw c");
		const char * msg = "ERROR (Wrong password)\r\n";

//		dprintf(fd, "pw d");
		write(fd, msg, strlen(msg));

//		dprintf(fd, "pw e");
		//delete [] msg;

//		dprintf(fd, "pw f");
		return false;
	}else{
//		dprintf(fd, "pw g\n");
		
		
		return true;
	}
	
}

void
IRCServer::addUser(int fd, const char * user, const char * password, const char * args)
{
	// Here add a new user. For now always return OK.
	string username(user);
	string pw(password);
	if(userMap.find(username) != userMap.end()){
		const char * msg = "DENIED\r\n";
		write(fd, msg, strlen(msg));
		
		return;
	}
	userMap[username] = pw;

	const char * msg =  "OK\r\n";
	write(fd, msg, strlen(msg));
	
	return;		
}

void
IRCServer::enterRoom(int fd, const char * user, const char * password, const char * args)
{
	if(!(checkPassword(fd, user, password)))
		return;
	string room(args);
	if(find(activeRooms.begin(), activeRooms.end(), args) == activeRooms.end()){
		dprintf(fd, "ERROR (No room)\r\n");		
		return;
	}

	string username(user);
	
	if(!(find(roomUserMap[room].begin(), roomUserMap[room].end(), user) == roomUserMap[room].end())){
		dprintf(fd, "OK\r\n");
		return;
	}


	roomUserMap[room].push_back(username);
	
	sort(roomUserMap[room].begin(), roomUserMap[room].end() );

	dprintf(fd, "OK\r\n");
}

void
IRCServer::leaveRoom(int fd, const char * user, const char * password, const char * args)
{
	if(!(checkPassword(fd, user, password)))
		return;
	string room(args);
	string username(user);

	if(find(roomUserMap[room].begin(), roomUserMap[room].end(), user) == roomUserMap[room].end()){
		dprintf(fd, "ERROR (No user in room)\r\n");
		return;
	}

	int i;
	for(i = 0; i < roomUserMap[room].size(); i++){
		if(username == roomUserMap[room].at(i)){
			roomUserMap[room].erase(roomUserMap[room].begin()+i);
		}
	}



	dprintf(fd, "OK\r\n");
}

void
IRCServer::sendMessage(int fd, const char * user, const char * password, const char * args)
{
	if(!(checkPassword(fd, user, password)))
		return;
	
	//parse args first
	string s(args);
	string room = s.substr(0, s.find_first_of(" "));
	s = s.substr(s.find_first_of(" ")+1);
	string messageString = s;

	char * messageC = new char[messageString.length()+1];
	strcpy(messageC, messageString.c_str());
	
	//construct full message to be stored
	char * fullMessage = new char[strlen(user) + 1 + strlen(messageC) + 3];
	fullMessage[0] = '\0';
	char space[2] = " ";
	char end[3] = "\r\n";

	strcat(fullMessage, user);
	strcat(fullMessage, space);
	strcat(fullMessage, messageC);
	strcat(fullMessage, end);

	//check that user is in room
	if(find(roomUserMap[room].begin(), roomUserMap[room].end(), user) == roomUserMap[room].end()){
		dprintf(fd, "ERROR (user not in room)\r\n");
		return;
	}

	//construct message string and add to messages vector
	string fullMessageString(fullMessage);
	roomMessageMap[room].push_back(fullMessageString);

	dprintf(fd, "OK\r\n");
	delete [] messageC;
	delete [] fullMessage;
	//delete end and space?
}

void
IRCServer::getMessages(int fd, const char * user, const char * password, const char * args)
{

	if(!(checkPassword(fd,user,password)))
		return;
	string s(args);
	string messageNumString = s.substr(0, s.find_first_of(" "));
	s = s.substr(s.find_first_of(" ")+1);
	string room = s;
	int messageNum = atoi(messageNumString.c_str());
	
	long int size = (long int) roomMessageMap[room].size();

	if(find(roomUserMap[room].begin(), roomUserMap[room].end(), user) == roomUserMap[room].end()){
		dprintf(fd, "ERROR (User not in room)\r\n");
		return;
	}

	if(messageNum >= size){
		dprintf(fd, "NO-NEW-MESSAGES\r\n");
		return;
	}

	int i = messageNum+1;

	
	for(;i < size; i++){
		dprintf(fd, "%d %s", i, roomMessageMap[room].at(i).c_str());
	}
	dprintf(fd, "\r\n");
	
	
}

void
IRCServer::getUsersInRoom(int fd, const char * user, const char * password, const char * args)
{
	if(!(checkPassword(fd,user,password)))
		return;
	string room(args);
	int i;
	for(i = 0; i < roomUserMap[room].size(); i++){
		dprintf(fd, "%s\r\n", roomUserMap[room].at(i).c_str());
	}
	dprintf(fd, "\r\n");


}

void
IRCServer::getAllUsers(int fd, const char * user, const char * password,const  char * args)
{
	//dprintf(fd, "AT PASSWORD CALL\n");	
	if(!(checkPassword(fd, user, password))){
		
		return;
	}
	//dprintf(fd, "PAST CHECKPASSWORD\n");
	for(map<string,string>::iterator it = userMap.begin(); it != userMap.end(); ++it){
		
		dprintf(fd, "%s\r\n", it->first.c_str());
		
	}
	
	dprintf(fd, "\r\n");
	

}

void
IRCServer::createRoom(int fd, const char * user, const char * password, const char * args)
{
	if(!(checkPassword(fd, user, password))){
		return;
	}
	if(find(activeRooms.begin(), activeRooms.end(), args) != activeRooms.end()){ // might run into issue with args, c string and all
		dprintf(fd, "DENIED\r\n");
		return;
	}

	string roomName(args);
	activeRooms.push_back(roomName);

	vector<string> activeUsers;
	roomUserMap[roomName] = activeUsers;

	vector<string> messages;
	roomMessageMap[roomName] = messages;
		
	dprintf(fd, "OK\r\n");

}

void
IRCServer::listRooms(int fd, const char * user, const char * password, const char * args)
{
	if(!(checkPassword(fd, user, password))){
		return;
	}

	int i;
	for(i = 0; i < activeRooms.size(); i++){
		dprintf(fd, "%s\r\n", activeRooms.at(i).c_str() );
	}
	dprintf(fd, "\r\n");


}
