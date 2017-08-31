
#include <stdio.h>
#include <gtk/gtk.h>	
#include <time.h>
//#include <curses.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <cairo.h>

static char buffer[256];
char * username = (char *) malloc(51);
char * password = (char *) malloc(51);
char * errorMsg = (char *) malloc(20);
char * newRoomName = (char *) malloc(51);
char * sport;
char * host;
char * currentRoom = (char *) malloc(51);
char * enteredMessage = (char *) malloc(1001);

int port;
int loggedIn = 0;
int inRoom = 0;
#define MAX_RESPONSE (10 * 1024)

GtkListStore * list_rooms;
GtkListStore * list_users;
GtkWidget * loginWindow;
GtkWidget * roomWindow;
GtkWidget * usersListTreeView;
GtkWidget * roomsListTreeView;
GtkWidget *messages;
GtkWidget *myMessage;
GtkWidget *window;
GtkWidget *view;

GtkTreeSelection * roomSelection;
//GtkListStore * messages;


int open_client_socket() {
	// Initialize socket address structure
	struct  sockaddr_in socketAddress;
	
	// Clear sockaddr structure
	memset((char *)&socketAddress,0,sizeof(socketAddress));
	
	// Set family to Internet 
	socketAddress.sin_family = AF_INET;
	
	// Set port
	socketAddress.sin_port = htons((u_short)port);
	
	// Get host table entry for this host
	struct  hostent  *ptrh = gethostbyname(host);
	if ( ptrh == NULL ) {
		perror("gethostbyname");
		exit(1);
	}
	
	// Copy the host ip address to socket address structure
	memcpy(&socketAddress.sin_addr, ptrh->h_addr, ptrh->h_length);
	
	// Get TCP transport protocol entry
	struct  protoent *ptrp = getprotobyname("tcp");
	if ( ptrp == NULL ) {
		perror("getprotobyname");
		exit(1);
	}
	
	// Create a tcp socket
	int sock = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (sock < 0) {
		perror("socket");
		exit(1);
	}
	
	// Connect the socket to the specified server
	if (connect(sock, (struct sockaddr *)&socketAddress,
		    sizeof(socketAddress)) < 0) {
		perror("connect");
		exit(1);
	}
	
	return sock;
}


int sendCommand(char * command, char * args, char * response) {
	int sock = open_client_socket();

	// Send command
	///
	/////
	/////////
	//////////////////MAYBE just write a single time, using one big long string? if it doesn't work
	memset(response, 0, MAX_RESPONSE);
	write(sock, command, strlen(command));
	write(sock, " ", 1);
	write(sock, username, strlen(username));
	write(sock, " ", 1);
	write(sock, password, strlen(password));
	if(strlen(args) > 0){
		write(sock, " ", 1);
		write(sock, args, strlen(args));
	}
	write(sock, "\r\n",2);

	// Keep reading until connection is closed or MAX_REPONSE
	int n = 0;
	int len = 0;
	while ((n=read(sock, response+len, MAX_RESPONSE - len))>0) {
		len += n;
	}

	printf("response:%s\n", response);

	close(sock);
}

void printUsage(){
	printf("Usage: IRCClient <host> <port>\n");
	exit(1);
}

void update_list_users() {
	if(inRoom == 0)
		return;
	GtkTreeIter iter;
	char response[MAX_RESPONSE];
	gchar * name;
	char * userString;
	userString = (char *) malloc(51);
	sendCommand("GET-USERS-IN-ROOM", currentRoom, response);
	
	if(!strcmp(response, "\r\n")){
		return;	
	}
	gtk_list_store_clear(list_users);	
	userString = strtok(response, "\r\n");
	name = g_strdup_printf ("%s", userString);
	gtk_list_store_append (GTK_LIST_STORE (list_users), &iter);
	gtk_list_store_set (GTK_LIST_STORE (list_users), &iter, 0, name, -1);

	userString = strtok(NULL, "\r\n");
	while( userString != NULL){
		
		name = g_strdup_printf ("%s", userString);
		gtk_list_store_append (GTK_LIST_STORE (list_users), &iter);
		gtk_list_store_set (GTK_LIST_STORE (list_users), &iter, 0, name, -1);

		userString = strtok(NULL, "\r\n");
	
	}
}

void update_list_rooms() {
    GtkTreeIter iter;
    /*
    for (i = 0; i < 10; i++) {
        gchar *msg = g_strdup_printf ("Room %d", i);
        gtk_list_store_append (GTK_LIST_STORE (list_rooms), &iter);
        gtk_list_store_set (GTK_LIST_STORE (list_rooms), 
	                    &iter,
                            0, msg,
	                    -1);
	g_free (msg);
    }
    */
    if(loggedIn == 1){
	char response[MAX_RESPONSE];
	gchar * roomName;
	char * roomString;
	roomString = (char *) malloc(51);
	sendCommand("LIST-ROOMS", "", response);
	
	if(!strcmp(response, "\r\n")){
		return;	
	}
	gtk_list_store_clear(list_rooms);	
	roomString = strtok(response, "\r\n");
	roomName = g_strdup_printf ("%s", roomString);
	gtk_list_store_append (GTK_LIST_STORE (list_rooms), &iter);
	gtk_list_store_set (GTK_LIST_STORE (list_rooms), &iter, 0, roomName, -1);

	roomString = strtok(NULL, "\r\n");
	while( roomString != NULL){
		
		roomName = g_strdup_printf ("%s", roomString);
		gtk_list_store_append (GTK_LIST_STORE (list_rooms), &iter);
		gtk_list_store_set (GTK_LIST_STORE (list_rooms), &iter, 0, roomName, -1);

		roomString = strtok(NULL, "\r\n");
	}
    }

}

void update_messages(){
	if(inRoom == 0 || loggedIn == 0)
		return;
	GtkTextBuffer * buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW (view));
	char response[MAX_RESPONSE];
	gchar * allMessages;
	//char * allMessagesString;
	//allMessagesString = (char *) malloc(MAX_RESPONSE);
	
	char * args = (char *) malloc(55);
	args[0] = '\0';
	strcat(args, "-1 ");
	strcat(args, currentRoom);

	sendCommand("GET-MESSAGES", args, response);
	if(!strcmp(response, "\r\n"))
		return;
	allMessages = g_strdup_printf ("%s", response);



	gtk_text_buffer_set_text (buf, allMessages, -1);
}
void update_all(){
	update_messages();
	update_list_rooms();
	update_list_users();
	
}
/* Create the list of "messages" */
//static GtkWidget *create_list( const char * titleColumn, GtkListStore *model, GtkWidget *tree_view )
static GtkWidget *create_rooms_list(const char * titleColumn, GtkListStore *model)
{
    GtkWidget *scrolled_window;
    //GtkListStore *model;
    GtkCellRenderer *cell;
    GtkTreeViewColumn *column;

    int i;
   
    /* Create a new scrolled window, with scrollbars only if needed */
    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				    GTK_POLICY_AUTOMATIC, 
				    GTK_POLICY_AUTOMATIC);
   
    //model = gtk_list_store_new (1, G_TYPE_STRING);
    roomsListTreeView = gtk_tree_view_new ();
    gtk_container_add (GTK_CONTAINER (scrolled_window), roomsListTreeView);
    gtk_tree_view_set_model (GTK_TREE_VIEW (roomsListTreeView), GTK_TREE_MODEL (model));
    gtk_widget_show (roomsListTreeView);
   
    cell = gtk_cell_renderer_text_new ();

    column = gtk_tree_view_column_new_with_attributes (titleColumn,
                                                       cell,
                                                       "text", 0,
                                                       NULL);
  
    gtk_tree_view_append_column (GTK_TREE_VIEW (roomsListTreeView),
	  		         GTK_TREE_VIEW_COLUMN (column));

    return scrolled_window;
}
   
static GtkWidget *create_users_list(const char * titleColumn, GtkListStore *model)
{
    GtkWidget *scrolled_window;
    //GtkListStore *model;
    GtkCellRenderer *cell;
    GtkTreeViewColumn *column;

    int i;
   
    /* Create a new scrolled window, with scrollbars only if needed */
    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				    GTK_POLICY_AUTOMATIC, 
				    GTK_POLICY_AUTOMATIC);
   
    //model = gtk_list_store_new (1, G_TYPE_STRING);
    usersListTreeView = gtk_tree_view_new ();
    gtk_container_add (GTK_CONTAINER (scrolled_window), usersListTreeView);
    gtk_tree_view_set_model (GTK_TREE_VIEW (usersListTreeView), GTK_TREE_MODEL (model));
    gtk_widget_show (usersListTreeView);
   
    cell = gtk_cell_renderer_text_new ();

    column = gtk_tree_view_column_new_with_attributes (titleColumn,
                                                       cell,
                                                       "text", 0,
                                                       NULL);
  
    gtk_tree_view_append_column (GTK_TREE_VIEW (usersListTreeView),
	  		         GTK_TREE_VIEW_COLUMN (column));

    return scrolled_window;
}

/* Add some text to our text widget - this is a callback that is invoked
when our window is realized. We could also force our window to be
realized with gtk_widget_realize, but it would have to be part of
a hierarchy first */

static void insert_text( GtkTextBuffer *buffer, const char * initialText )
{
   GtkTextIter iter;
 
   gtk_text_buffer_get_iter_at_offset (buffer, &iter, 0);
   gtk_text_buffer_insert (buffer, &iter, initialText,-1);
}
   
/* Create a scrolled text area that displays a "message" */
static GtkWidget *create_text( const char * initialText )
{
   GtkWidget *scrolled_window;
   GtkTextBuffer *buffer;

   view = gtk_text_view_new ();
   buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

   scrolled_window = gtk_scrolled_window_new (NULL, NULL);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
		   	           GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);

   gtk_container_add (GTK_CONTAINER (scrolled_window), view);
   insert_text (buffer, initialText);

   gtk_widget_show_all (scrolled_window);

   return scrolled_window;
}


static void show_event(GtkWidget *button, GtkWidget *w){
	gtk_widget_show(w);
}

static void show_room(GtkWidget *button, GtkWidget *w){
	gtk_widget_show(w);
}
	
void pw_event(GtkWidget *button, GtkWidget *w){
	const gchar *text = gtk_entry_get_text (GTK_ENTRY (w) );
	password = strdup(text);

	///////watch out here later if there's a segfault - no null byte?
}


//fix this if there's time - you can only log in as a new user - would need to change server
int addRegisterUser(){
	char response[MAX_RESPONSE];
	sendCommand("ADD-USER", "", response);
	if(!strcmp(response, "OK\r\n")){
		return 1;
	}else{
		if(!strcmp(response, "DENIED\r\n")){
			errorMsg = "User already exists.";
		} else{
			errorMsg = "Wrong password.";
		}
		return 0;
	}



}

int createNewRoom(){
	char response[MAX_RESPONSE];
	sendCommand("CREATE-ROOM", newRoomName, response);
	if(!strcmp(response, "OK\r\n")){
		return 1;
	} else if(loggedIn == 0){
		errorMsg = "You must be logged in to create a room.";
		return -1;
	} else{
		errorMsg = "Room already exists.";
		return 0;
	}
}
void username_event(GtkWidget *button, GtkWidget *w){
	const gchar *text = gtk_entry_get_text (GTK_ENTRY (w) );
	username = strdup(text);
	int result = addRegisterUser();

	if(result == 1){
		loggedIn = 1;
		update_list_rooms();
		gtk_widget_hide (loginWindow);
	}else{
		//GtkWidget * msgWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
		//gtk_widget_show (msgWindow);
		GtkWidget * msg = gtk_message_dialog_new (GTK_WINDOW (loginWindow) , 
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, errorMsg);
		gtk_window_set_title(GTK_WINDOW(msg), "Error");
		gtk_widget_show (msg);
		gtk_dialog_run(GTK_DIALOG(msg));
		gtk_widget_destroy(msg);
		//gtk_widget_hide (loginWindow);
		
	}
	gtk_widget_hide (loginWindow);
}


void send_message_event(GtkWidget *button, GtkWidget *w){
	if(loggedIn == 0 || inRoom == 0){
		return;
	}
	const gchar *text = gtk_entry_get_text (GTK_ENTRY (w) );
	enteredMessage = strdup(text);
	char response[MAX_RESPONSE];
	
	char * toSend = (char *) malloc(strlen(enteredMessage) + 200);
	toSend[0] = '\0';
	strcat(toSend, currentRoom);
	strcat(toSend, " ");
	strcat(toSend, enteredMessage);

	sendCommand("SEND-MESSAGE", toSend, response);

        gtk_entry_set_text (GTK_ENTRY (w), "");
	update_all();
	
}

void new_room_event(GtkWidget *button, GtkWidget *w){
	const gchar *text = gtk_entry_get_text (GTK_ENTRY (w));
	newRoomName = strdup(text);

	int i = 0;
	for(;i < strlen(newRoomName); i++){
		if(newRoomName[i] == ' '){
			errorMsg = "Room names may not contain spaces.";

			GtkWidget * msg = gtk_message_dialog_new (GTK_WINDOW (roomWindow) , 
				GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, errorMsg);
			gtk_window_set_title(GTK_WINDOW(msg), "Error");
			gtk_widget_show (msg);
			gtk_dialog_run(GTK_DIALOG(msg));
			gtk_widget_destroy(msg);
			gtk_widget_hide (roomWindow);
			return;
		}
	}
	int result = createNewRoom();
	
	if(result == 1){
	
		update_list_rooms();
		gtk_widget_hide (roomWindow);
	}else{		
		GtkWidget * msg = gtk_message_dialog_new (GTK_WINDOW (roomWindow) , 
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, errorMsg);
		gtk_window_set_title(GTK_WINDOW(msg), "Error");
		gtk_widget_show (msg);
		gtk_dialog_run(GTK_DIALOG(msg));
		gtk_widget_destroy(msg);
	}
	gtk_widget_hide (roomWindow);
}

void join_room_event(GtkWidget *button){
	if(loggedIn == 0){
		errorMsg = "You must be logged in to join a room.";

		GtkWidget * msg = gtk_message_dialog_new (GTK_WINDOW (window) , 
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, errorMsg);
		gtk_window_set_title(GTK_WINDOW(msg), "Error");
		gtk_widget_show (msg);
		gtk_dialog_run(GTK_DIALOG(msg));
		gtk_widget_destroy(msg);

		return;
	}
	GtkTreeIter i;
	GtkTreeModel * m;

	//roomSelection = gtk_tree_view_get_selection(GTK_TREE_VIEW (roomsListTreeView));
	
	roomSelection = gtk_tree_view_get_selection(GTK_TREE_VIEW (roomsListTreeView));

	if(gtk_tree_selection_get_selected(GTK_TREE_SELECTION(roomSelection), &m, &i) == NULL){
		errorMsg = "Please select a room.";		
		GtkWidget * msg = gtk_message_dialog_new (GTK_WINDOW (window) , 
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, errorMsg);
		gtk_window_set_title(GTK_WINDOW(msg), "Error");
		gtk_widget_show (msg);
		gtk_dialog_run(GTK_DIALOG(msg));
		gtk_widget_destroy(msg);
		return;
	}
	char * chosen;
	gtk_tree_model_get(m, &i, 0, &chosen, -1);
	
	char response[MAX_RESPONSE];

	if(inRoom == 1){

//////////watch for segfaults here
		if(!strcmp(chosen, currentRoom)){
			return;
		}

		char * msg = " left the room.";
		char * together = (char *) malloc(100);
		together[0] = '\0';
		strcat(together, currentRoom);
		strcat(together, msg);
///////////watch for segfaults here

		sendCommand("SEND-MESSAGE", together, response);
		sendCommand("LEAVE-ROOM", currentRoom, response);
		currentRoom = strdup(chosen);
		
	}else{
		inRoom = 1;	
		currentRoom = strdup(chosen);
	}
	
	sendCommand("ENTER-ROOM", currentRoom, response);
	
	char * tog = (char *) malloc(100);
	tog[0] = '\0';
	char * message = " entered the room.";
	strcat(tog, currentRoom);
	strcat(tog, message);
	
	sendCommand("SEND-MESSAGE", tog , response);

	
	update_list_rooms();
	update_list_users();
	update_messages();
}


static gboolean time_handler(GtkWidget *widget){
	if (widget->window == NULL)
		return FALSE;
	time_t curtime;
	struct tm *loctime;

	curtime = time(NULL);
	loctime = localtime(&curtime);
	strftime(buffer, 256, "%T", loctime);

	gtk_widget_queue_draw(widget);
	update_all();
	return TRUE;
}

void cleanup_event(){
	if(inRoom == 1 && loggedIn == 1){
		
		char * msg = " left the room.";
		char * together = (char *) malloc(100);
		together[0] = '\0';
		strcat(together, currentRoom);
		strcat(together, msg);
		
		char response[MAX_RESPONSE];

		sendCommand("SEND-MESSAGE", together, response);
		sendCommand("LEAVE-ROOM", currentRoom, response);
	}
}

int main( int   argc,
          char *argv[] )
{	
	char * command;
	if (argc < 3) {
		printUsage();
	}
	host = argv[1];
	sport = argv[2];
	sscanf(sport, "%d", &port);
    
    GtkWidget *list;
    GtkWidget *usersList;
    GtkWidget *pwEntry;
    GtkWidget *userEntry;
    GtkWidget *vbox;
    GtkWidget *roomBox;
    GtkWidget *roomEntry;  

    gtk_init (&argc, &argv);
    //GtkWidget *loginWindow;
   
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (window), "Paned Windows");
    g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
    
    g_signal_connect (window, "destroy", G_CALLBACK (cleanup_event), NULL);
    
    gtk_container_set_border_width (GTK_CONTAINER (window), 10);
    gtk_widget_set_size_request (GTK_WIDGET (window), 450, 400);

    //create separate login window
    loginWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (loginWindow), "Login/Create Account");
    g_signal_connect (loginWindow, "destroy", G_CALLBACK (gtk_widget_hide_on_delete), NULL);
    gtk_container_set_border_width (GTK_CONTAINER (loginWindow), 10);
    gtk_widget_set_size_request (GTK_WIDGET (loginWindow), 200, 200);

    //create separate newRoom window
    roomWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (roomWindow), "Create Room");
    g_signal_connect (roomWindow, "destroy", G_CALLBACK (gtk_widget_hide_on_delete), NULL);
    gtk_container_set_border_width (GTK_CONTAINER (roomWindow), 10);
    gtk_widget_set_size_request (GTK_WIDGET (roomWindow), 200, 200);

    //create and add verticle box, room name entry box
    roomBox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (roomWindow), roomBox);
    gtk_widget_show (roomBox);
    
    roomEntry = gtk_entry_new();
    gtk_entry_set_max_length (GTK_ENTRY (roomEntry), 50);
    gtk_entry_set_text (GTK_ENTRY (roomEntry), "Room Name");
    gtk_box_pack_start (GTK_BOX (roomBox), roomEntry, TRUE, TRUE, 0);
    gtk_widget_show (roomEntry);
    //create "Create" button for room creation
    GtkWidget * create_button = gtk_button_new_with_label ("Create");
    gtk_box_pack_start (GTK_BOX (roomBox), create_button, TRUE, TRUE, 0);
    gtk_widget_show (create_button);
    g_signal_connect (create_button, "clicked", G_CALLBACK (new_room_event), roomEntry);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (loginWindow), vbox);
    gtk_widget_show (vbox);

    //create and add username entry box
    userEntry = gtk_entry_new();
    gtk_entry_set_max_length (GTK_ENTRY (userEntry), 50);
    gtk_entry_set_text (GTK_ENTRY (userEntry), "Username");
    //text stuff
    gtk_box_pack_start (GTK_BOX (vbox), userEntry, TRUE, TRUE, 0);
    gtk_widget_show (userEntry);

    //create and add password entry box
    pwEntry = gtk_entry_new();
    gtk_entry_set_max_length (GTK_ENTRY (pwEntry), 50);
    gtk_entry_set_text (GTK_ENTRY (pwEntry), "Password");
    //
    gtk_entry_set_visibility (GTK_ENTRY (pwEntry), FALSE);
    gtk_box_pack_start (GTK_BOX (vbox), pwEntry, TRUE, TRUE, 0);
    gtk_widget_show (pwEntry);


    //add enter button for login
    GtkWidget * enter_button = gtk_button_new_with_label ("Enter");
    gtk_box_pack_start (GTK_BOX (vbox), enter_button, TRUE, TRUE, 0);
    gtk_widget_show (enter_button);
    g_signal_connect (enter_button, "clicked", G_CALLBACK (pw_event), pwEntry);
    g_signal_connect (enter_button, "clicked", G_CALLBACK (username_event), userEntry);


    // Create a table to place the widgets. Use a 7x4 Grid (7 rows x 4 columns)
    GtkWidget *table = gtk_table_new (7, 4, TRUE);
    gtk_container_add (GTK_CONTAINER (window), table);
    gtk_table_set_row_spacings(GTK_TABLE (table), 5);
    gtk_table_set_col_spacings(GTK_TABLE (table), 5);
    gtk_widget_show (table);

    // Add list of rooms. Use columns 0 to 4 (exclusive) and rows 0 to 4 (exclusive)
    list_rooms = gtk_list_store_new (1, G_TYPE_STRING);
    update_list_rooms();
   
    list = create_rooms_list ("Rooms", list_rooms);
    gtk_table_attach_defaults (GTK_TABLE (table), list, 2, 4, 0, 2);
    gtk_widget_show (list);

    //add list of users in room.
    list_users = gtk_list_store_new (1, G_TYPE_STRING);
    usersList = create_users_list ("Users in Room", list_users);
      
    gtk_table_attach_defaults (GTK_TABLE (table), usersList, 0, 2, 0, 2);
    gtk_widget_show (usersList);


    // Add messages text. Use columns 0 to 4 (exclusive) and rows 4 to 7 (exclusive) 
    messages = create_text ("");
    gtk_table_attach_defaults (GTK_TABLE (table), messages, 0, 4, 2, 5);
    gtk_widget_show (messages);
    // Add messages text. Use columns 0 to 4 (exclusive) and rows 4 to 7 (exclusive) 

    myMessage = gtk_entry_new();
    gtk_entry_set_max_length (GTK_ENTRY (myMessage), 1000);
    gtk_entry_set_text (GTK_ENTRY (myMessage), "");
    gtk_table_attach_defaults (GTK_TABLE (table), myMessage, 0, 4, 5, 7);
    gtk_widget_show(myMessage);

/*
    myMessage = create_text ("");
    gtk_table_attach_defaults (GTK_TABLE (table), myMessage, 0, 4, 5, 7);
    gtk_widget_show (myMessage);
*/
    	

    // Add send button. Use columns 0 to 1 (exclusive) and rows 4 to 7 (exclusive)
    GtkWidget *send_button = gtk_button_new_with_label ("Send Message");
    gtk_table_attach_defaults(GTK_TABLE (table), send_button, 0, 1, 7, 8); 
    gtk_widget_show (send_button);
    g_signal_connect (send_button, "clicked", G_CALLBACK (send_message_event), myMessage);

    //add login/create button
    GtkWidget *login_create_button = gtk_button_new_with_label ("Login/Create");
    gtk_table_attach_defaults(GTK_TABLE (table), login_create_button, 3, 4, 7, 8);
    gtk_widget_show (login_create_button);

    //link login/create button to login window
    g_signal_connect (login_create_button, "clicked", G_CALLBACK (show_event), loginWindow);

    //add join room button
    GtkWidget *join_room_button = gtk_button_new_with_label ("Join Room");
    gtk_table_attach_defaults(GTK_TABLE (table), join_room_button, 2, 3, 7, 8);
    gtk_widget_show (join_room_button);
    g_signal_connect (join_room_button, "clicked", G_CALLBACK (join_room_event), NULL);

////////////////////right here
    //add create room button
    GtkWidget *create_room_button = gtk_button_new_with_label ("Create Room");
    gtk_table_attach_defaults(GTK_TABLE (table), create_room_button, 1, 2, 7, 8);
    gtk_widget_show (create_room_button);
    
    g_signal_connect (create_room_button, "clicked", G_CALLBACK (show_room), roomWindow);

    gtk_widget_show (table);
    gtk_widget_show (window);
    
    //timer stuff
    g_timeout_add(5000, (GSourceFunc) time_handler, (gpointer) window);
 
 
 
    //time_handler(window);

    gtk_main ();
    

    return 0;
}
