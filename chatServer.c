#include "chatServer.h"
#include "signal.h"
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/ioctl.h>
static int end_server = 0;

void intHandler(int SIG_INT) {
	/* use a flag to end_server to break the main loop */
}

int main (int argc, char *argv[])
{
	/* {charServer.c , port, url }*/
	signal(SIGINT, intHandler);
	int server_fd = 0;
	conn_pool_t* pool;
   	struct sockaddr_in server_socket;
	char buf[512];
	int count_read = 0;
	int on = 1;
	int check = 0;
	/*************************************************************/
	/* Create an AF_INET stream socket to receive incoming      */
	/* connections on                                            */
	/*************************************************************/
	if((server_fd = socket(AF_INET,SOCK_STREAM,0))<0){
		printf("Usage: socket failed");
		exit(1);
	}
	server_socket.sin_family = AF_INET;
	server_socket.sin_addr.s_addr = htonl(argv[2]);
    server_socket.sin_port = htons(argv[1]);
	/*************************************************************/
	/* Set socket to be nonblocking. All of the sockets for      */
	/* the incoming connections will also be nonblocking since   */
	/* they will inherit that state from the listening socket.   */
	/*************************************************************/
	check = ioctl(server_fd, (int)FIONBIO, (char *)&on);
	if(check <0){
		printf("Usage: ioctl failed ");
		return 1;
	}
	/*************************************************************/
	/* Bind the socket                                           */
	/*************************************************************/
	check = bind(server_fd,(struct sockaddr*)& server_socket,sizeof(server_socket));
	if(check<0) {
		printf("Usage: bind failed");
		return 1;
	}
	/*************************************************************/
	/* Set the listen back log                                   */
	/*************************************************************/
	check = listen(server_fd,5);
	if(check < 0){
		printf("Usage: listen failed");
		return 1;
	}

	/*************************************************************/
	/* Initialize fd_sets  			                             */
	/*************************************************************/
	pool = malloc(sizeof(conn_pool_t));
	if(pool == NULL){
		printf("Usage: malloc pool failed");
		return 1;
	}
	check = init_pool(pool);
	if(check == 1){
		printf("Usage: init pool failed");
		return 1;
	}
	/*************************************************************/
	/* Loop waiting for incoming connects, for incoming data or  */
	/* to write data, on any of the connected sockets.           */
	/*************************************************************/
	do
	{
		/**********************************************************/
		/* Copy the master fd_set over to the working fd_set.     */
		/**********************************************************/

		/**********************************************************/
		/* Call select() 										  */
		/**********************************************************/
		select(pool->maxfd,&pool->read_set,&pool->write_set,NULL,10);

		
		/**********************************************************/
		/* One or more descriptors are readable or writable.      */
		/* Need to determine which ones they are.                 */
		/**********************************************************/
		
		for (conn_t* cur_conn = pool->conn_head;cur_conn->next != NULL; cur_conn = cur_conn->next/* check all descriptors, stop when checked all valid fds */)
		{
			/* Each time a ready descriptor is found, one less has  */
			/* to be looked for.  This is being done so that we     */
			/* can stop looking at the working set once we have     */
			/* found all of the descriptors that were ready         */
				
			/*******************************************************/
			/* Check to see if this descriptor is ready for read   */
			/*******************************************************/
			if (FD_ISSET(cur_conn->fd,&pool->ready_read_set))
			{
				/***************************************************/
				/* A descriptor was found that was readable		   */
				/* if this is the listening socket, accept one      */
				/* incoming connection that is queued up on the     */
				/*  listening socket before we loop back and call   */
				/* select again. 						            */
				/****************************************************/
                if(cur_conn->fd == server_fd){
					int fd = accept(cur_conn->fd,&server_socket,sizeof(server_socket));
					add_conn(fd,pool);
				}
				/****************************************************/
				/* If this is not the listening socket, an 			*/
				/* existing connection must be readable				*/
				/* Receive incoming data his socket             */
				/****************************************************/
				else{
					if((count_read = read(cur_conn->fd,buf,511)) == 0){
						/*the client closed the socket and we cannot read from it..*/
						remove_conn(cur_conn,server_fd);
					}
					else{
						/*adding the messege in a 512 size packages to the messeges of all the connections.*/
						add_msg(cur_conn,buf,count_read,pool);
						while((count_read = read(cur_conn->fd,buf,511)) == 511){
							add_msg(cur_conn->fd,buf,512,pool);
						}
						if(count_read != 0){
							add_msg(cur_conn->fd,buf,count_read,pool);
						}
					}
					

				}
				/* If the connection has been closed by client 		*/
                /* remove the connection (remove_conn(...))    		*/
							
				/**********************************************/
				/* Data was received, add msg to all other    */
				/* connections					  			  */
				/**********************************************/
                
		                  
				
			} /* End of if (FD_ISSET()) */
			/*******************************************************/
			/* Check to see if this descriptor is ready for write  */
			/*******************************************************/
			if (FD_ISSET(cur_conn->fd,&pool->ready_write_set)) {
				/* try to write all msgs in queue to sd */
				write_to_client(cur_conn->fd,pool);
		 	}
		 /*******************************************************/
		 
		 
      } /* End of loop through selectable descriptors */

   } while (end_server == 0);

	/*************************************************************/
	/* If we are here, Control-C was typed,						 */
	/* clean up all open connections					         */
	/*************************************************************/
	
	return 0;
}


int init_pool(conn_pool_t* pool) {
	//initialized all fields
	pool->maxfd = 0;
	pool->nready = 0;
	FD_ZERO(&pool->read_set);
	FD_ZERO(&pool->ready_read_set);
	FD_ZERO(&pool->write_set);
	FD_ZERO(&pool->ready_write_set);
	pool->nr_conns = 0;
	return 0;
}

int add_conn(int sd, conn_pool_t* pool) {
	/*
	 * 1. allocate connection and init fields
	 * 2. add connection to pool
	 * */
	if(sd != 0){
		conn_t* new_conn = (conn_t*)malloc(sizeof(conn_t));
		new_conn->fd = sd;
		new_conn->prev = NULL;
		new_conn->write_msg_head = NULL;
		new_conn->write_msg_tail = NULL;
		if(pool->conn_head == NULL){
			new_conn->next = NULL;
			pool->conn_head = new_conn;
		}
		else{
			new_conn->next = pool->conn_head;
			pool->conn_head->prev = new_conn;
			pool->conn_head = new_conn;
		}

		if(sd > pool->maxfd){
			pool->maxfd = sd;
		}
		pool->nr_conns++;
		pool->nready++;

	}
	FD_SET(sd,&pool->read_set);
	FD_SET(sd,&pool->ready_read_set);
	FD_SET(sd,&pool->ready_write_set);
	FD_SET(sd,&pool->write_set);
	
	return 0;
}


int remove_conn(int sd, conn_pool_t* pool) {
	/*
	* 1. remove connection from pool 
	* 2. deallocate connection 
	* 3. remove from sets 
	* 4. update max_fd if needed 
	*/
	conn_t* current_conn = pool->conn_head;
	/*go to the placement of the sd inside the pool*/
	while(current_conn->fd != sd){

		if((current_conn = current_conn->next)!=NULL);
		else return;
	}
	if(current_conn == pool->conn_head){
		pool->conn_head = current_conn->next;
	}
	current_conn->prev->next = current_conn->next;
	/*what is remove from sets.*/
	/*freeing the data of the connection */
	if(current_conn->fd == pool->maxfd)pool->maxfd = current_conn->prev->fd;
	free(current_conn);
	FD_CLR(sd,&pool->read_set);
	FD_CLR(sd,&pool->ready_read_set);
	FD_CLR(sd,&pool->ready_write_set);
	FD_CLR(sd,&pool->write_set);
	return 0;
}


int add_msg(int sd,char* buffer,int len,conn_pool_t* pool) {
	
	/*
	 * 1. add msg_t to write queue of all other connections 
	 * 2. set each fd to check if ready to write 
	 */
	msg_t* new_msg = (msg_t*)malloc(sizeof(msg_t));
	new_msg->message = buffer;
	new_msg->next = NULL;
	new_msg->size = strlen(buffer);
	for(conn_t* cur_con = pool->conn_head;cur_con->next!=NULL;cur_con = cur_con->next){
		if(cur_con->fd != sd){
			if(cur_con->write_msg_tail==cur_con->write_msg_head&& cur_con->write_msg_head==NULL){
				new_msg->prev = NULL;
				cur_con->write_msg_head = new_msg;
				cur_con->write_msg_tail = new_msg;
			}
			else{
				if(cur_con->write_msg_tail==cur_con->write_msg_head){
					cur_con->write_msg_head->next = new_msg;
				}
				cur_con->write_msg_tail->next = new_msg;
				cur_con->write_msg_tail= new_msg;
			}
			/*not sure what flag should go up here */
			FD_SET(cur_con->fd,&pool->ready_write_set);
		}
	}
	return 0;
}

int write_to_client(int sd,conn_pool_t* pool) {
	
	/*
	 * 1. write all msgs in queue 
	 * 2. deallocate each writen msg 
	 * 3. if all msgs were writen successfully, there is nothing else to write to this fd... */
	conn_t* cur_conn = pool->conn_head;
	while(cur_conn->fd != sd){
		if(cur_conn->next != NULL){
			cur_conn = cur_conn->next;	
		}
		else return 1;/*check if the sd given exits in the pool*/
	}
	while(cur_conn->write_msg_head != NULL){
		write(sd,cur_conn->write_msg_head,cur_conn->write_msg_head->size);
		cur_conn->write_msg_head = cur_conn->write_msg_head->next;
		free(cur_conn->write_msg_head->prev);
		cur_conn->write_msg_head->prev = NULL;
	}
	return 0;
}

