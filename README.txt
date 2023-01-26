Chat Server
Authored by Nitzan Migdal
206763351

==Description==
Chat server that communicate the messeges between all the connected clients.
the clients connect to the server and then they can communicate between one another. the server does'nt get messeges.
Functions:
chatServer.c:
    intHandler: handles the ocation when we want to quit the server and we have connected client.
    init_pool: Initializes the values of the pool as such:
        /* Largest file descriptor in this pool. */
        int maxfd;                      
        /* Number of ready descriptors returned by select. */
        int nready;                     
        /* Set of all active descriptors for reading. */
        fd_set read_set;                
        /* Subset of descriptors ready for reading. */
        fd_set ready_read_set;  
        /* Set of all active descriptors for writing. */
        fd_set write_set;               
        /* Subset of descriptors ready for writing.  */
        fd_set ready_write_set; 
        /* Doubly-linked list of active client connection objects. */
        struct conn *conn_head;
        /* Number of active client connections. */
        unsigned int nr_conns;
    add_conn: adds a connection to the linklist conn_head inside the pool specifed.
    remove_conn: removes a conn from the linklist.
    add_msg: sends a messege to the rest of the connected clients.
    main: makes the server and start a select that uses three fd's 
        read:   if read from a client :checking if a client sent information to the rest 
                from server: new connection is meet and need to be added to the conn linklist.
        write:  if there is a messege waiting to be sent to the specific client we are checking right now.
        end:    check if ^c was pressed and then exits the do while function.

==Structs==
    typedef struct conn_pool { 
            /* Largest file descriptor in this pool. */
            int maxfd;                      
            /* Number of ready descriptors returned by select. */
            int nready;                     
            /* Set of all active descriptors for reading. */
            fd_set read_set;                
            /* Subset of descriptors ready for reading. */
            fd_set ready_read_set;  
            /* Set of all active descriptors for writing. */
            fd_set write_set;               
            /* Subset of descriptors ready for writing.  */
            fd_set ready_write_set; 
            /* Doubly-linked list of active client connection objects. */
            struct conn *conn_head;
            /* Number of active client connections. */
            unsigned int nr_conns;
            
    }conn_pool_t;

    /*
    * Data structure to keep track of messages. Each message object holds one
    * complete line of message from a client.  
    *
    * The message objects are maintained per connection in a doubly-linked list. 
    * When a message is read from one connection, it is added to the list of all othe rconnections.
    *
    * A message is added to the list only when a complete line has been read from
    * the client. 
    */
    typedef struct msg {
            /* Points to the previous message object in the doubly-linked list. */
            struct msg *prev;
            /* Points to the next message object in the doubly-linked list. */
            struct msg *next;
            /* Points to a dynamically allocated buffer holding the message. */
            char *message;
            /* Size of the message. */
            int size;
    }msg_t;


    /*
    * Data structure to keep track of client connection state.
    *
    * The connection objects are also maintained in a global doubly-linked list.
    * There is a dummy connection head at the beginning of the list.
    */
    typedef struct conn {
            /* Points to the previous connection object in the doubly-linked list. */
            struct conn *prev;      
            /* Points to the next connection object in the doubly-linked list. */
            struct conn *next;      
            /* File descriptor associated with this connection. */
            int fd;                 
            /* 
            * Pointers for the doubly-linked list of messages that
            * have to be written out on this connection.
            */
            struct msg *write_msg_head;
            struct msg *write_msg_tail;
    }conn_t;
==Program Files==
    chatServer.c
    README.txt
==How to compile?==
compile: gcc -o tester main.c threadpool.c server.c threadpool.h -lpthread
run: ./tester (with the <port> <pool-size> <number_of_request_allowed>)

==Input:==
when calling the chatserver.c program we need to give the wanted port.
==Output:==
when a new connection is meet the program will print its value like this:
    ""New incoming connection on fd"
when closing a connection it prints:
    "connection closed for fd"
when a client sends info to another it will print the messege on all the other clients.
before everytime it waits in select it will print the value of pool->max_fd

==Exciptions==
- Usage: malloc pool failed : get when the malloc for the poll failed inside the main.
- Usage: listen failed : when the listen falied.
- Usage: bind failed: happens after a crush or ^z you need to change the port given to the program.
- Usage: 