#include "server.h"
#include "protocol.h"
#include <pthread.h>
#include <signal.h>
#include <errno.h>

const char exit_str[] = "exit";
const char *helpMsg = "/help\n/h\n/createroom roomname\n/create roomname\n/c roomname\n"
                        "deleteroom roomname\n/delete roomname\n/d roomname\n"
                        "/joinroom roomname\n/join roomname\n/j roomname\n"
                        "/leaveroom roomname\n/leave roomname\n/l roomname\n"
                        "/roomlist\n/rlist\n/r\n"
                        "/userlist\n/ulist\n/u\n"
                        "/chat\n username\n/ch username\n"
                        "/logout\n/quit\n/q\n";

char buffer[BUFFER_SIZE];
pthread_mutex_t buffer_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t job_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t user_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t room_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t audit_lock = PTHREAD_MUTEX_INITIALIZER;

List_t *job_list; 
List_t *room_list;
List_t *user_list;

int total_num_msg = 0;
int listen_fd;
int flag = 0;
FILE *audit_fp;

void sigint_handler(int sig) {
    int olderrno = errno;
    printf("shutting down server\n");
    flag = 1;
    errno = olderrno;
}

int server_init(int server_port) {
    int sockfd;
    struct sockaddr_in servaddr;

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(EXIT_FAILURE);
    } else
        printf("Socket successfully created\n");

    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(server_port);

    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (char *)&opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (SA *)&servaddr, sizeof(servaddr))) != 0) {
        printf("socket bind failed\n");
        exit(EXIT_FAILURE);
    } else
        printf("Socket successfully binded\n");

    // Now server is ready to listen and verification
    if ((listen(sockfd, 1)) != 0) {
        printf("Listen failed\n");
        exit(EXIT_FAILURE);
    } else
        printf("Server listening on port: %d.. Waiting for connection\n", server_port);

    return sockfd;
}

//Function running in thread
// Job thread?? (Consumer)
void *process_client(void *clientfd_ptr) {
    pthread_detach(pthread_self());
    int client_fd = *(int *)clientfd_ptr;
    free(clientfd_ptr);
    int received_size;
    int job_type;
    int insert_job_success;
    fd_set read_fds;
    char c_buffer[BUFFER_SIZE];
    int retval;
    while (1) {
        /*
        FD_ZERO(&read_fds);
        FD_SET(client_fd, &read_fds);
        retval = select(client_fd + 1, &read_fds, NULL, NULL, NULL);
        if (retval != 1 && !FD_ISSET(client_fd, &read_fds)) {
            // pthread_mutex_lock(&audit_lock);
            // fprintf(audit_fp, "Error with select() function\n");
            // pthread_mutex_unlock(&audit_lock);
            break;
        }*/

        /* Start Critical Section */
        //pthread_mutex_lock(&buffer_lock);

        bzero(c_buffer, BUFFER_SIZE);
        //read sizeof(buffer)-1 bytes -> reads 1 less than the capacity so null terminator will always exist
        received_size = read(client_fd, c_buffer, sizeof(c_buffer)-1); 
        if (received_size < 0) {
            // pthread_mutex_lock(&audit_lock);
            // fprintf(audit_fp, "Receiving failed\n");
            // pthread_mutex_unlock(&audit_lock);
            return NULL;
        } else if (received_size == 0) {
            continue;
        }
        
        //lock job buffer
        pthread_mutex_lock(&job_lock);
        //put job into job buffer
        job_type = jobType(c_buffer);
        insert_job_success = insertJob(job_list, c_buffer, client_fd);
        pthread_mutex_unlock(&job_lock);
        if(insert_job_success){

                if(job_type == LOGOUT){
                    continue;
                }
            //wait and read server reply 
            //petr_header* msg_header = (petr_header*)&buffer;
            //if(job_type != RMLIST && job_type != USRLIST &&
                //job_type != RMRECV && job_type != USRRECV){
                //read 8 bytes from buffer

                //received_size = recv(client_fd, c_buffer, 8, 0);
                
                //output error message or OK

                
                /*
                pthread_mutex_lock(&audit_lock);
                switch(((petr_header*)c_buffer)->msg_type){
                    case OK:
                        fprintf(audit_fp, "OK\n"); 
                        break;
                    case ERMEXISTS:
                        fprintf(audit_fp, "Room exists already\n");
                        break;
                    case ERMFULL:
                        fprintf(audit_fp, "Room capacity reached\n");
                        break;
                    case ERMNOTFOUND:
                        fprintf(audit_fp, "Room does not exist on server\n");
                        break;
                    case ERMDENIED:
                        fprintf(audit_fp, "Can't delete room, not creator or creator can't leave room\n");
                        break;
                    case EUSRNOTFOUND:
                        fprintf(audit_fp, "User does not exist on server\n");
                        break;
                    case ESERV:
                        fprintf(audit_fp, "Generic error\n");
                        break;
                    default:
                        fprintf(audit_fp, "Unknown error\n");
                        break;
                }
                pthread_mutex_unlock(&audit_lock);
                */
        }
        else{
            pthread_mutex_lock(&audit_lock);
            fprintf(audit_fp, "insertJob failed.\n");
            pthread_mutex_unlock(&audit_lock); 
            petr_header err_header;
            err_header.msg_type = 255;
            err_header.msg_len = 0;
            write(client_fd, &err_header, 8);
        }
        //unlock buffer
        bzero(c_buffer, BUFFER_SIZE);
        //pthread_mutex_unlock(&buffer_lock);
        
    }
    // Close the socket at the end
    printf("Close current client connection\n");
    //close(client_fd);
    return NULL;
}

void *process_job()
{
    char newline = '\n';
    petr_header *job_header = malloc(sizeof(petr_header *));
    job_header->msg_len = 0;
    job_header->msg_type = 255;
    int retval;
    char proc_buffer[BUFFER_SIZE];
    // Continue attempting to grab a job buffer to execute
    while(1)
    {   
        // Check if sigint is raised.
        // If the flag is 1, Wipe everything and terminate the server.
        if(flag == 1)
        {
            pthread_mutex_lock(&user_lock);
            pthread_mutex_lock(&room_lock);
            node_t* currentRoom = room_list->head;
            node_t* prevRoom;
            while(currentRoom){
                
                prevRoom = currentRoom;
                currentRoom = currentRoom->next;
                // Free usersInRoom List
                room_in_s* prevRoomData = (room_in_s*)(prevRoom->data);
                node_t* userJoinedNode = prevRoomData->usersInRoom->head;
                node_t* prevJoinedNode = userJoinedNode;
                while(userJoinedNode){
                    prevJoinedNode = userJoinedNode;
                    userJoinedNode = userJoinedNode->next;
                    free(prevJoinedNode);
                }
                free(((room_in_s*)(prevRoom->data))->usersInRoom);
                free(prevRoom);
            }
             

            node_t* currentUser = user_list->head;
            node_t* prevUser;
            while(currentUser){
                prevUser = currentUser;
                currentUser = currentUser->next;
                //free rmJoined
                user_in_s* prevUserData = (user_in_s*)(prevUser->data);
                node_t* roomJoinedNode = prevUserData->rmJoined->head;
                node_t* prevJoinedNode = roomJoinedNode;
                //room_in_s* roomJoined = roomJoinedNode->data;
                //room_in_s* prevJoined = roomJoinedNode->data;
                while(roomJoinedNode){
                    prevJoinedNode = roomJoinedNode;
                    roomJoinedNode = roomJoinedNode->next;
                    free(prevJoinedNode);
                }
                free(((user_in_s*)(prevUser->data))->rmJoined);
                free(prevUser);
            } 
            pthread_mutex_unlock(&user_lock);
            pthread_mutex_unlock(&room_lock);
            exit(0);
        }


        pthread_mutex_lock(&job_lock);
        if(job_list->head != NULL)
        {
            node_t *temp = detachHead(job_list);
            room_in_s *targetRoom;
            pthread_mutex_unlock(&job_lock);
            job_in_s *job = (job_in_s *)(temp->data);
            int to_fd;
            char *carriageReturn;
            char *name;
            char targetUser[NAME_LEN];
            bzero(targetUser, NAME_LEN);
            char roomName[NAME_LEN];
            bzero(roomName, NAME_LEN);
            char msgFrom[BUFFER_SIZE];
            bzero(msgFrom, BUFFER_SIZE);
            char msgTo[BUFFER_SIZE];
            bzero(msgTo, BUFFER_SIZE);
            int index; 
            switch(job->type)
            {
                /* LOGOUT */
                case LOGOUT:
                    // Check if the user is creator of any rooms
                    // Delete Rooms if the user is the creator.
                    // Check if the user is joined in any rooms
                    // Leave room if the user is in the room.
                    pthread_mutex_lock(&user_lock);
                    pthread_mutex_lock(&room_lock);
                    user_in_s *user_info = ((node_t*)getUserFromFD(user_list, job->from_client_fd))->data;
                    node_t *currentRoom = user_info->rmJoined->head;
                    while(currentRoom){
                        if(isRoomCreator((room_in_s *)(currentRoom->data), user_info)){
                            retval = deleteRoom(room_list, ((room_in_s *)(currentRoom->data))->roomName, user_list, job->from_client_fd, job->type);
                            if(retval == 0){
                                // pthread_mutex_lock(&audit_lock);
                                // fprintf(audit_fp, "RMDELETE did not work correctly during client logout. %i\n", job->from_client_fd);
                                // pthread_mutex_unlock(&audit_lock);
                            }
                        }
                        else{
                            retval = removeUserFromRoom(room_list, ((room_in_s *)(currentRoom->data))->roomName, user_list, job->from_client_fd, job->type);
                            if(retval == 0){
                                // pthread_mutex_lock(&audit_lock);
                                // fprintf(audit_fp, "RMLEAVE did not work correctly during client logout. %i\n", job->from_client_fd);
                                // pthread_mutex_unlock(&audit_lock);
                            }
                        }
                        currentRoom = currentRoom->next;
                    }
                    // Audit is done in removeUser function
                    retval = removeUser(user_list, job->from_client_fd);
                    printAllUsers(user_list);
                    printAllRooms(room_list);
                    pthread_mutex_unlock(&room_lock);
                    pthread_mutex_unlock(&user_lock);
                    // ERROR occurred during removeUser
                    // if(retval == 1){
                        job_header->msg_type = OK;
                        job_header->msg_len = 0;
                        write(job->from_client_fd, job_header, 8);
                        close(job->from_client_fd);
                        // pthread_mutex_lock(&audit_lock);
                        // fprintf(audit_fp, "LOGOUT %i, %s\n", job->from_client_fd, job->msg);
                        // pthread_mutex_unlock(&audit_lock);
                        // write(job->from_client_fd, job_header, 8);
                        // close(job->from_client_fd);
                    // }
                    break;
                /* Create New Room */
                case RMCREATE:
                    pthread_mutex_lock(&user_lock);
                    pthread_mutex_lock(&room_lock);
                    char *from_user = getUserName(user_list, job->from_client_fd);
                    if(roomExists(room_list, job->msg))
                    {
                        pthread_mutex_unlock(&user_lock);
                        pthread_mutex_unlock(&room_lock);
                        job_header->msg_len = 0;
                        job_header->msg_type = ERMEXISTS;
                        write(job->from_client_fd, job_header, 8);
                        // pthread_mutex_lock(&audit_lock);
                        // fprintf(audit_fp, "ERMEXISTS %i %s\n", job->from_client_fd, job->msg);
                        // pthread_mutex_unlock(&audit_lock);
                        break;
                    }
                    if(!validNameLength(job->msg))
                    {
                        job_header->msg_len = 0;
                        job_header->msg_type = 255;
                        pthread_mutex_unlock(&user_lock);
                        pthread_mutex_unlock(&room_lock);
                        write(job->from_client_fd, job_header, 8);
                        // pthread_mutex_lock(&audit_lock);
                        // fprintf(audit_fp, "RMNameInvalid %i %s\n", job->from_client_fd, job->msg);
                        // pthread_mutex_unlock(&audit_lock);
                        break;
                    }
                    
                    createRoom(room_list, job->msg, user_list, from_user, job->from_client_fd);
                    pthread_mutex_unlock(&user_lock);
                    pthread_mutex_unlock(&room_lock);
                    job_header->msg_len = 0;
                    job_header->msg_type = OK;
                    write(job->from_client_fd, job_header, 8);
                    // pthread_mutex_lock(&audit_lock);
                    // fprintf(audit_fp, "RMCREATE %i %s\n", job->from_client_fd, job->msg);
                    // pthread_mutex_unlock(&audit_lock);
                    printAllUsers(user_list);
                    printAllRooms(room_list);
                    break;
                    
                /* Delete a Room */
                case RMDELETE:
                    pthread_mutex_lock(&user_lock);
                    pthread_mutex_lock(&room_lock);
                    if(!(room_list->head))
                    {
                        pthread_mutex_unlock(&room_lock);
                        job_header->msg_len = 0;
                        job_header->msg_type = ERMNOTFOUND;
                        write(job->from_client_fd, job_header, 8);
                        // pthread_mutex_lock(&audit_lock);
                        // fprintf(audit_fp, "ERMNOTFOUND %i %s\n", job->from_client_fd, job->msg);
                        // pthread_mutex_unlock(&audit_lock);
                        break;
                    }
                    // pthread_mutex_lock(&audit_lock);
                    // fprintf(audit_fp, "RMDELETE %i %s\n", job->from_client_fd, job->msg);
                    // pthread_mutex_unlock(&audit_lock);
                    retval = deleteRoom(room_list, job->msg, user_list, job->from_client_fd, RMDELETE);
                    printAllUsers(user_list);
                    printAllRooms(room_list);
                    pthread_mutex_unlock(&user_lock);
                    pthread_mutex_unlock(&room_lock);
                    break;
                /* Request List of Rooms */
                case RMLIST:
                    pthread_mutex_lock(&room_lock);
                    if(room_list->length > 0)
                    {
                        bzero(proc_buffer, BUFFER_SIZE);
                        printRoomListWithUsers(room_list, proc_buffer+8);
                        pthread_mutex_unlock(&room_lock);
                        job_header->msg_len = strlen(proc_buffer+8) + 1;
                        job_header->msg_type = RMLIST;
                        memcpy(proc_buffer, job_header, 8);
                        //(job_header*)(proc_buffer)->msg_len = strlen(proc_buffer+8)
                        //write(job->from_client_fd, job_header, 8);

                        write(job->from_client_fd, proc_buffer, (job_header->msg_len)+8);
                        // pthread_mutex_lock(&audit_lock);
                        // fprintf(audit_fp, "Attempted to print RMLIST but FAILED: %i, %s\n", job->from_client_fd, job->msg);
                        // pthread_mutex_unlock(&audit_lock);
                        break;
                    }
                    pthread_mutex_unlock(&room_lock);
                    job_header->msg_len = 0;
                    job_header->msg_type = RMLIST;
                    bzero(proc_buffer, BUFFER_SIZE);
                    memcpy(proc_buffer, job_header, 8);
                    write(job->from_client_fd, proc_buffer, 8);
                    // pthread_mutex_lock(&audit_lock);
                    // fprintf(audit_fp, "RMLIST %i, %s\n", job->from_client_fd, job->msg);
                    // pthread_mutex_unlock(&audit_lock);
                    break;
                /* Join a Room */
                case RMJOIN:
                    //node_t *current = (node_t *)(room_list->head);
                    retval = 0;
                    pthread_mutex_lock(&user_lock);
                    pthread_mutex_lock(&room_lock);
                    retval = addUserToRoom(room_list, job->msg, user_list, job->from_client_fd); 
                    printAllRooms(room_list);
                    printAllUsers(user_list);
                    pthread_mutex_unlock(&user_lock);
                    pthread_mutex_unlock(&room_lock);
                    //if retval == 1 -> user was successful added
                    if(retval == 1){
                        job_header->msg_type = OK;
                        job_header->msg_len = 0;
                        write(job->from_client_fd, job_header, 8);
                        // pthread_mutex_lock(&audit_lock);
                        // fprintf(audit_fp, "RMJOIN %i %s", job->from_client_fd, job->msg);
                        // pthread_mutex_unlock(&audit_lock);
                    }
                    /*

                    while(current)
                    {
                        if(strcmp(job->msg, ((room_in_s *)(current->data))->roomName) == 0)
                        {
                            pthread_mutex_lock(&user_lock);
                            retval = addUserToRoom(room_list, job->msg, user_list, job->from_client_fd);
                            pthread_mutex_unlock(&user_lock);
                            if(retval == 1)
                            {
                                job_header->msg_type = OK;
                                job_header->msg_len = 0;
                                write(job->from_client_fd, job_header, 8);
                            }
                            else
                            {
                                job_header->msg_len = 0;
                                job_header->msg_type = 255;
                                write(job->from_client_fd, job_header, 8);
                                pthread_mutex_lock(&audit_lock);
                                fprintf(audit_fp, "Failed to add the client to the room: %i, %s\n", job->from_client_fd, job->msg);
                                pthread_mutex_unlock(&audit_lock);
                            }
                            
                            pthread_mutex_lock(&audit_lock);
                            fprintf(audit_fp, "RMJOIN %i %s", job->from_client_fd, job->msg);
                            pthread_mutex_unlock(&audit_lock);
                            break;
                        }
                        current = current->next;
                    }
                    */
                    
                    if(retval == 0)
                    {
                        // ERMNOTFOUND Room not found with the input roomName
                        job_header->msg_len = 0;
                        job_header->msg_type = ERMNOTFOUND;
                        write(job->from_client_fd, job_header, 8);
                        // pthread_mutex_lock(&audit_lock);
                        // fprintf(audit_fp, "RMJOIN request resulted in ERMNOTFOUND: %i, %s\n", job->from_client_fd, job->msg);
                        // pthread_mutex_unlock(&audit_lock);
                    }
                    
                    //printJoinedRoomsFromFD(user_list, job->from_client_fd);
                    //printGetUsersInRoom(room_list, job->msg);
                    break;
                /* Leave a Room */
                case RMLEAVE:
                    pthread_mutex_lock(&user_lock);
                    pthread_mutex_lock(&room_lock);
                    if(!roomExists(room_list, job->msg)){
                        pthread_mutex_unlock(&user_lock);
                        pthread_mutex_unlock(&room_lock);
                        job_header->msg_len = 0;
                        job_header->msg_type = ERMNOTFOUND;
                        write(job->from_client_fd, job_header, 8);
                        // pthread_mutex_lock(&audit_lock);
                        // fprintf(audit_fp, "ERMNOTFOUND: %i, %s\n", job->from_client_fd, job->msg);
                        // pthread_mutex_unlock(&audit_lock);
                        break;
                    }
                    targetRoom = (room_in_s*)getRoom(room_list, job->msg);
                    name = getUserName(user_list, job->from_client_fd);
                    if(getUser(targetRoom->usersInRoom, name) == NULL){
                        pthread_mutex_unlock(&user_lock);
                        pthread_mutex_unlock(&room_lock);
                        job_header->msg_type = OK;
                        job_header->msg_len = 0;
                        write(job->from_client_fd, job_header, 8);
                        // pthread_mutex_lock(&audit_lock);
                        // fprintf(audit_fp, "OK, User is not in the room: %i, %s\n", job->from_client_fd, job->msg);
                        // pthread_mutex_unlock(&audit_lock);
                        break;
                    }
                    if(strcmp(name, targetRoom->creator) == 0){
                        pthread_mutex_unlock(&user_lock);
                        pthread_mutex_unlock(&room_lock);
                        job_header->msg_len = 0;
                        job_header->msg_type = ERMDENIED;
                        write(job->from_client_fd, job_header, 8);
                        // pthread_mutex_lock(&audit_lock);
                        // fprintf(audit_fp, "ERMDENIED, creator of the room can not leave the room: %i, %s\n", job->from_client_fd, job->msg);
                        // pthread_mutex_unlock(&audit_lock);
                        break;
                    }
                    
                    retval = removeUserFromRoom(room_list,job->msg, user_list, job->from_client_fd, RMLEAVE);
                    printAllUsers(user_list);
                    printAllRooms(room_list);
                    pthread_mutex_unlock(&user_lock);
                    pthread_mutex_unlock(&room_lock);
                    if(retval == 0){
                        job_header->msg_len = 0;
                        job_header->msg_type = 255;
                        write(job->from_client_fd, job_header, 8);
                        // pthread_mutex_lock(&audit_lock);
                        // fprintf(audit_fp, "removeUserFromRoom() FAILED during RMLEAVE request: %i, %s\n", job->from_client_fd, job->msg);
                        // pthread_mutex_unlock(&audit_lock);
                        break;
                    }
                    job_header->msg_len = 0;
                    job_header->msg_type = OK;
                    write(job->from_client_fd, job_header, 8);
                    // pthread_mutex_lock(&audit_lock);
                    // fprintf(audit_fp, "RMLEAVE %i, %s\n", job->from_client_fd, job->msg);
                    // pthread_mutex_unlock(&audit_lock);
                    break;
                /* Send a Message to a Room */
                case RMSEND:
                    carriageReturn = strchr(job->msg, '\r');
                    index = (int)(carriageReturn - job->msg);
                    strncpy(roomName, job->msg, index);
                    strncpy(msgFrom, (job->msg) + index + 2, strlen(job->msg)-(index+2));
                    strncat(msgTo, roomName, strlen(roomName));
                    strncat(msgTo, carriageReturn, 1);
                    strncat(msgTo, &newline, 1);
                    pthread_mutex_lock(&user_lock);
                    name = getUserName(user_list, job->from_client_fd);
                    strncat(msgTo, name, strlen(name));
                    pthread_mutex_unlock(&user_lock);
                    strncat(msgTo, carriageReturn, 1);
                    strncat(msgTo, &newline, 1);
                    strncat(msgTo, msgFrom, strlen(msgFrom));
                    bzero(proc_buffer, BUFFER_SIZE);
                    memcpy(proc_buffer+8, msgTo, strlen(msgTo));
                    job_header->msg_len = strlen(msgTo) + 1;
                    job_header->msg_type = RMRECV;
                    
                    // bzero(proc_buffer, BUFFER_SIZE);
                    // memcpy(proc_buffer, job_header, 8);
                    // strncat(proc_buffer, msgTo, strlen(msgTo));
                    // job_header->msg_len = sizeof()
                    pthread_mutex_lock(&user_lock);
                    pthread_mutex_lock(&room_lock);
                    targetRoom = (room_in_s *)getRoom(room_list, roomName);
                    if(targetRoom == NULL){
                        pthread_mutex_unlock(&user_lock);
                        pthread_mutex_unlock(&room_lock);
                        job_header->msg_len = 0;
                        job_header->msg_type = ERMNOTFOUND;
                        write(job->from_client_fd, job_header, 8);
                        // pthread_mutex_lock(&audit_lock);
                        // fprintf(audit_fp, "ERMNOTFOUND, FAILED during RMSEND: %i, %s\n", job->from_client_fd, job->msg);
                        // pthread_mutex_unlock(&audit_lock);
                        break;
                    }
                    List_t *usersInRoom = targetRoom->usersInRoom;
                    if(getUser(usersInRoom, getUserName(user_list, job->from_client_fd)) == NULL){
                        pthread_mutex_unlock(&user_lock);
                        pthread_mutex_unlock(&room_lock);
                        job_header->msg_len = 0;
                        job_header->msg_type = ERMDENIED;
                        write(job->from_client_fd, job_header, 8);
                        // pthread_mutex_lock(&audit_lock);
                        // fprintf(audit_fp, "ERMDENIED, FAILED during RMSEND: %i, %s\n", job->from_client_fd, job->msg);
                        // pthread_mutex_unlock(&audit_lock);
                        break;
                    }
                    node_t *currentNode = usersInRoom->head;
                    while(currentNode)
                    {
                        to_fd = ((user_in_s*)currentNode->data)->user_fd;
                        if(job->from_client_fd != to_fd)
                        {
                            memcpy(proc_buffer, job_header, 8);
                            write(to_fd, proc_buffer, (job_header->msg_len) + 8);
                            // pthread_mutex_lock(&audit_lock);
                            // fprintf(audit_fp, "RMRECV, Successfully executed RMSEND: %i, %s\n", job->from_client_fd, job->msg);
                            // pthread_mutex_unlock(&audit_lock);
                        }
                        currentNode = currentNode->next;
                    }
                    pthread_mutex_unlock(&user_lock);
                    pthread_mutex_unlock(&room_lock);
                    job_header->msg_type = OK;
                    job_header->msg_len = 0;
                    write(job->from_client_fd, job_header, 8);
                    break;
                /* Send a Message to a User */
                case USRSEND:
                    // Don't need mutex lock because it's not modifying any data.
                    pthread_mutex_lock(&user_lock);
                    carriageReturn = strchr(job->msg, '\r');
                    index = (int)(carriageReturn - job->msg);
                    strncpy(targetUser, job->msg, index);
                    strncpy(msgFrom, (job->msg) + index + 2, strlen(job->msg)-(index+2));
                    to_fd = userFdLookUp(user_list, targetUser);
                    if(to_fd < 0){
                        pthread_mutex_unlock(&user_lock);
                        job_header->msg_len = 0;
                        job_header->msg_type = EUSRNOTFOUND;
                        write(job->from_client_fd, job_header, 8);
                        // pthread_mutex_lock(&audit_lock);
                        // fprintf(audit_fp, "EUSRNOTFOUND, Failed during USRSEND: %i, %s\n", job->from_client_fd, job->msg);
                        // pthread_mutex_unlock(&audit_lock);
                        break;
                    }
                    bzero(msgTo, BUFFER_SIZE);
                    name = getUserName(user_list, job->from_client_fd);
                    strncat(msgTo, name, strlen(name));
                    strncat(msgTo, carriageReturn, 1);
                    strncat(msgTo, &newline, 1);
                    strncat(msgTo, msgFrom, strlen(msgFrom));
                    job_header->msg_len = strlen(msgTo) + 1;
                    job_header->msg_type = USRRECV;
                    bzero(proc_buffer, BUFFER_SIZE);
                    memcpy(proc_buffer+8, msgTo, strlen(msgTo));
                    memcpy(proc_buffer, job_header, 8);
                    write(to_fd, proc_buffer, (job_header->msg_len) + 8);
                    bzero(proc_buffer, BUFFER_SIZE);
                    job_header->msg_len = 0;
                    job_header->msg_type = OK;
                    write(job->from_client_fd, job_header, 8);
                    pthread_mutex_unlock(&user_lock);
                    
                    // pthread_mutex_lock(&audit_lock);
                    // fprintf(audit_fp, "USRSEND %i, %s\n", job->from_client_fd, job->msg);
                    // pthread_mutex_unlock(&audit_lock);
                    break;
                /* Request User List */
                case USRLIST:
                    pthread_mutex_lock(&user_lock);
                    if(user_list->length > 0)
                    {
                        bzero(proc_buffer, BUFFER_SIZE);
                        node_t *current = user_list->head;
                        user_in_s* user_info;
                        while(current)
                        {
                            user_info = (user_in_s *)(current->data);
                            if(job->from_client_fd != user_info->user_fd)
                            {
                                int name_len = strlen(user_info->name);
                                strncat(proc_buffer+8, user_info->name, name_len);
                                strncat(proc_buffer+8, &newline, 1);
                            }
                            current = current->next;
                        }
                        pthread_mutex_unlock(&user_lock);
                        if(user_list->length == 1){
                            job_header->msg_len = 0;
                        }
                        else{
                            job_header->msg_len = strlen(proc_buffer+8) + 1;
                        }
                        job_header->msg_type = USRLIST;
                        //write(job->from_client_fd, job_header, 8);
                        memcpy(proc_buffer, job_header, 8);
                        write(job->from_client_fd, proc_buffer, (job_header->msg_len) + 8);
                        // pthread_mutex_lock(&audit_lock);
                        // fprintf(audit_fp, "USRLIST %i, %s\n", job->from_client_fd, job->msg);
                        // pthread_mutex_unlock(&audit_lock);
                        break;
                    }
                    pthread_mutex_unlock(&user_lock);
                    job_header->msg_len = 0;
                    job_header->msg_type = USRLIST;
                    write(job->from_client_fd, job_header, 8);
                    // pthread_mutex_lock(&audit_lock);
                    // fprintf(audit_fp, "USRLIST, successfully executed but the list is empty: %i, %s\n", job->from_client_fd, job->msg);
                    // pthread_mutex_unlock(&audit_lock);
                    break;
                /* Invalid Message Type */
                default:
                    printf("Invalid Message Type");
            }
            //pthread_mutex_unlock(&job_lock);
        }
        else
        {
            pthread_mutex_unlock(&job_lock);  
            
        }
        
    }
    free(job_header);
}

// Main Thread
void run_server(int server_port, int Nthread) {
    listen_fd = server_init(server_port); // Initiate server and start listening on specified port
    int client_fd;
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);

    pthread_t tid;    

    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
		perror("Failed to set signal handler");
		exit(EXIT_FAILURE);
	}

    while (1) {
        // Wait and Accept the connection from client
        printf("Wait for new client connection\n");
        int *client_fd = malloc(sizeof(int));
	    char *user_name = malloc(sizeof(char) * NAME_LEN);
        *client_fd = accept(listen_fd, (SA *)&client_addr, (socklen_t*)&client_addr_len);
        
        if (*client_fd < 0) {
            printf("server acccept failed\n");
            exit(EXIT_FAILURE);
        } 
        else {
            printf("Client connetion accepted\n");

             // Critical Section (buffer)
            pthread_mutex_lock(&buffer_lock);

            bzero(&buffer, BUFFER_SIZE);
            read(*client_fd, &buffer, 8);
            petr_header *msg_header = (petr_header*) &buffer;
            uint32_t msg_len = msg_header->msg_len;
            uint8_t msg_type = msg_header->msg_type;
            char msg_in[msg_len + 1];
            read(*client_fd, &buffer, msg_len);
            memcpy(msg_in, &buffer, msg_len);
            bzero(&buffer, BUFFER_SIZE);
            
            pthread_mutex_unlock(&buffer_lock);
            // End of Critical Section (buffer);

            // Login
            //<username>
            if(msg_type == 0x10)
            {
                if(userExists(user_list, msg_in)){
                    // Reject Login & Close Connection
                    msg_header->msg_len = 0;
                    msg_header->msg_type = EUSREXISTS;
                    // Critical Section (buffer)
                    pthread_mutex_lock(&buffer_lock);
                    write(*client_fd, msg_header, 8);
                    pthread_mutex_unlock(&buffer_lock);
                    // End of Critical Section (buffer);
                    // pthread_mutex_lock(&audit_lock);
                    // fprintf(audit_fp, "LOGIN(%i) %s\n", msg_header->msg_type, msg_in);
                    // pthread_mutex_unlock(&audit_lock);
                    // Closing connection
                    close(*client_fd);
                    continue;
                };
                pthread_mutex_lock(&user_lock);
                insertUser(user_list, msg_in, *client_fd);
                pthread_mutex_unlock(&user_lock);
                msg_header->msg_len = 0;
                msg_header->msg_type = OK;
                write(*client_fd, msg_header, 8);
                // Create a client Thread
                pthread_create(&tid, NULL, process_client, (void *)client_fd);
            }
            // initial message type isn't LOGIN
            else
            {
                pthread_mutex_lock(&buffer_lock);
                bzero(&buffer, BUFFER_SIZE);
                msg_header->msg_type = ESERV;
                msg_header->msg_len = 0;
                memcpy(&buffer, msg_header, 8);
                write(*client_fd, &buffer, 8);
                pthread_mutex_unlock(&buffer_lock);
            }
        }
    bzero(buffer, BUFFER_SIZE);
    // close(*client_fd);
    }
    close(listen_fd);
}

int main(int argc, char *argv[]) {
    int opt; 
    unsigned int port = 0;
    unsigned int Nthread = 2;
    char argBuffer[NAME_LEN];
    char auditFile[NAME_LEN];
    while ((opt = getopt(argc, argv, "hj:")) != -1) {
        switch (opt) {
        // case 'p':
        //     port = atoi(optarg);
        //     break;
        case 'h':
        	fprintf(stdout, "%s [-h][-j N] PORT_NUMBER AUDIT_FILENAME", argv[0]);
        	return EXIT_SUCCESS;
        case 'j':
           	Nthread = atoi(optarg);
        	break;
        default: /* '?' */
            fprintf(stderr, "Server Application Usage: %s -p <port_number>\n",
                    argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    strcpy(argBuffer, argv[optind]);
    port = atoi(argBuffer);
    strcpy(auditFile, argv[optind +1]);
    audit_fp = fopen(auditFile, "w");
    
    if(audit_fp == 0){
        fprintf(stderr, "ERROR: File did not open correctly.");
    }
    
    if (port == 0) {
        fprintf(stderr, "ERROR: Port number for server to listen is not given\n");
        fprintf(stderr, "Server Application Usage: %s -p <port_number>\n",
                argv[0]);
        exit(EXIT_FAILURE);
    }

    user_list = malloc(sizeof(List_t));
    user_list->head = NULL;
    user_list->length = 0;
    job_list = malloc(sizeof(List_t));
    job_list->head = NULL;
    job_list->length = 0;
    room_list = malloc(sizeof(List_t));
    room_list->head = NULL;
    room_list->length = 0;

    // Create N number of Job Threads
    int i;
    pthread_t tid;
    for(i = 0; i < Nthread; i++){
        pthread_create(&tid, NULL, process_job, NULL);
    }

    run_server(port, Nthread);

    return 0;
}

//inserts a new user using provided user_name and userfd
//mallocs a new user using the given arguments
void insertUser(List_t* list, char* user_name, int userfd){
    //list == head
    //user_name == user to insert
    if(list->length == 0){
        list->head = NULL;
    }
    
    node_t* head = list->head;
    //create new node
    node_t* new_node;
    new_node = malloc(sizeof(new_node));
    //create the data in node (user data)
    new_node->data = malloc(sizeof(user_in_s));
    //set new_node username and next ptr
    strcpy(((user_in_s*)(new_node->data))->name, user_name);
    ((user_in_s*)(new_node->data))->user_fd = userfd;
    List_t *rmJoined = malloc(sizeof(List_t));
    rmJoined->length = 0;
    rmJoined->head = NULL;
    ((user_in_s*)(new_node->data))->rmJoined = rmJoined;
    new_node->next = head;
    
    //increment list count and set head
    list->head = new_node;
    list->length++;
}

// returns node_t* to the user with name user_name
// returns NULL if user was not found
void* getUser(List_t* user_li, char* user_name){
    int retval;
    //check empty list
    if(user_li->length <= 0){ 
        // pthread_mutex_lock(&audit_lock);
        // fprintf(audit_fp, "in getUser: user list is <= 0 (%d)\n", user_li->length);
        // pthread_mutex_unlock(&audit_lock);
        return NULL;
    }
    //start current ptr at the head
    node_t* current = user_li->head;
    //char* current_name;
    //continue until the end of the list is reached
    while(current){
        retval = strcmp(((user_in_s*)(current->data))->name, user_name); 
        if(retval == 0){
            return current;
        }
        current = current->next;
    }
    //if the end of the list was reached return NULL
    return NULL;
}
//returns NULL if not found or list->length <= 0
//returns node_t (not node->data)
void* getUserFromFD(List_t* user_li, int userfd){
    if(user_li->length <= 0){
        // pthread_mutex_lock(&audit_lock);
        // fprintf(audit_fp, "in getUserFromFD: empty list given\n");
        // pthread_mutex_unlock(&audit_lock);
        return NULL;
    }
    //search the list until user with userfd is found
    node_t* current = user_li->head;
    while(current){
        if(((user_in_s*)(current->data))->user_fd == userfd) return current;
        current = current->next;
    }
    //if not found, return NULL
    return NULL;
}

char* getUserName(List_t* user_li, int userfd){
    if(user_li->length <= 0){
        // pthread_mutex_lock(&audit_lock);
        // fprintf(audit_fp, "in getUserName: empty list given\n");
        // pthread_mutex_unlock(&audit_lock);
        return NULL;
    }
    //search for userfd and return user
    node_t* user = (node_t*)getUserFromFD(user_li, userfd);
    
    return ((user_in_s*)(user->data))->name;
}

// returns 1 if user exists
// returns 0 if user doesn't exist
int userExists(List_t* user_li, char* user_name){
    if(getUser(user_li, user_name) == NULL) return 0;
    return 1;
}

// returns 1 if user successfully removed (freed user and removed from list)
// returns 0 if user remove failed (didn't exist)
int removeUser(List_t* user_li, int client_fd){
    //start current ptr at head
    node_t* current = user_li->head;
    node_t* prev = NULL;
    
    //check empty list
    if(user_li->length <= 0){ 
        // pthread_mutex_lock(&audit_lock);
        // fprintf(audit_fp, "ERROR LOGOUT %i \n", client_fd);
        // pthread_mutex_unlock(&audit_lock);
        return 0;
    }
    //continue iteration until end of list
    while(current){
        if(((user_in_s*)(current->data))->user_fd == client_fd){
            //make prev point at current->next (if prev exists)
            // pthread_mutex_lock(&audit_lock);
            // fprintf(audit_fp, "LOGOUT %i %s\n", client_fd, ((user_in_s*)(current->data))->name);
            // pthread_mutex_unlock(&audit_lock);
            if(prev){
                prev->next = current->next;
            }
            //head of list was removed (no prev)
            else{
                //change head of list
                user_list->head = current->next;
            }
            //decrement user list length
            user_list->length--;
            //deletes nodes in rmJoined
            deleteList(((user_in_s*)(current->data))->rmJoined);
            //delete rmJoined list
            free(((user_in_s*)(current->data))->rmJoined);
            //delete user
            free(current->data);
            //delete user node
            free(current);
            return 1;
        }
        prev = current;
        current = current->next;
    }
    // if end of list was reached return NULL
    // pthread_mutex_lock(&audit_lock);
    // fprintf(audit_fp, "ERROR LOGOUT %i %s\n", client_fd, ((user_in_s*)(current->data))->name);
    // pthread_mutex_unlock(&audit_lock);
    return 0;
}

void deleteUserList(List_t* list){
    if(list == NULL){
        // pthread_mutex_lock(&audit_lock);
        // fprintf(audit_fp, "in deleteList: list ptr is null\n");
        // pthread_mutex_unlock(&audit_lock);
        return;
    }

    node_t* current = list->head;
    node_t* prev = NULL;
    while(current){
        prev = current;
        current = current->next;
        //free(((user_in_s*)(prev->data))->rmJoined);
        deleteList(((user_in_s*)(prev->data))->rmJoined);
        free(prev->data);
        free(prev);
    }
    list->head = NULL;
    list->length = 0;
}

//prints list if it exists
//if list length <= 0, the names will not print
void printUserList(List_t* user_li){
    //check null ptr
    if(user_li == NULL){
        // pthread_mutex_lock(&audit_lock);
        // fprintf(audit_fp, "in printUserList: null list ptr given\n");
        // pthread_mutex_unlock(&audit_lock);
        return;
    }
    //print out length
    printf("list length: %d\n", user_li->length);
    //if the length is 0 or less, return
    if(user_li->length <= 0){
        return;
    }
    node_t* current = user_li->head;
    //print until the end of the list
    while(current){
        printf("name:%s\t", ((user_in_s*)(current->data))->name);
        current = current->next;
    }
}

//inserts (mallocs) new node at head of list
//to_add is expected to be data that is already malloced
void insertHead(List_t* list, void* to_add){
    //check empty list
    if(list->length == 0) 
        list->head = NULL;
    if(to_add == NULL){
        // pthread_mutex_lock(&audit_lock);
        // fprintf(audit_fp, "in insertHead: to_add is a null ptr\n");
        // pthread_mutex_unlock(&audit_lock);
        return;
    }

    node_t* head = list->head;
    node_t* new_node;
    new_node = malloc(sizeof(node_t));
    //new_node holds a pointer to the data and to the next node
    //set new_node next ptr
    new_node->next = head;
    new_node->data = to_add;
    //set new head and increment length
    list->head = new_node;
    list->length++;
}

//removes node specified
//to_remove is the ptr to the actual node (not the info in it)
int removeNode(List_t* list, void* to_remove){
    if(list->length <= 0){
        return 0;
    }
    //start current at head and iterate through list
    node_t* current = list->head;
    node_t* prev = NULL;
    while(current){
        if(current == to_remove){
            //if prev exists, make update prev->next
            if(prev){
                prev->next = current->next;
            }
            //removing the head of the list
            else{
                //update head of list
                list->head = current->next;
            }
            list->length--;
            free(current->data);
            free(current);
            return 1;
        }
        prev = current;
        current = current->next;
    }
    return 0;
}

void insertTail(List_t* list, void* to_add){
    //check empty list
    if(to_add == NULL){
        // pthread_mutex_lock(&audit_lock);
        // fprintf(audit_fp, "in insertTail: to_add is a null ptr\n");
        // pthread_mutex_unlock(&audit_lock);
        return;
    }
    if(list->length == 0){
        insertHead(list, to_add);
        return;
    }

    node_t *head = list->head;
    node_t *current = head;

    while(current->next != NULL){
        current = current->next;
    }
    
    current->next = malloc(sizeof(node_t));
    current->next->data = to_add;
    current->next->next = NULL;
    list->length++;    
}

//removes head (deletes it)
int removeHead(List_t* list){
    return removeNode(list, list->head);
}

//detaches head (does not delete it)
//returns head (node_t)
void* detachHead(List_t* list){
    if(list->length == 0){
        // pthread_mutex_lock(&audit_lock);
        // fprintf(audit_fp, "in detachHead: list is empty\n");
        // pthread_mutex_unlock(&audit_lock);
        return NULL;
    }
    node_t* to_detach = list->head;
    list->head = to_detach->next;
    list->length--;
    return to_detach;
}



//deletes the entire list
//does not free list pointer given
void deleteList(List_t* list){
    if(list == NULL){
        // pthread_mutex_lock(&audit_lock);
        // fprintf(audit_fp, "in deleteList: list ptr is null\n");
        // pthread_mutex_unlock(&audit_lock);
        return;
    }

    node_t* current = list->head;
    node_t* prev = NULL;
    while(current){
        prev = current;
        current = current->next;
        free(prev->data);
        free(prev);
    }
    list->head = NULL;
    list->length = 0;
}

void deleteListNodes(List_t* list){
    if(list == NULL){
        // pthread_mutex_lock(&audit_lock);
        // fprintf(audit_fp, "in deleteList: list ptr is null\n");
        // pthread_mutex_unlock(&audit_lock);
        return;
    }

    node_t* current = list->head;
    node_t* prev = NULL;
    while(current){
        prev = current;
        current = current->next;
        free(prev);
    }
    list->head = NULL;
    list->length = 0;
}

//adds job with given attributes to jobList
//adds at tail
//if msg contains contents, it will copy the string into the job struct
//  otherwise, it will leave the msg as NULL in the job struct
void addJob(List_t* jobList, uint8_t type, char* msg, uint32_t msg_len, int from_client_fd){
    //create the new job
    job_in_s* new_job;
    new_job = malloc(sizeof(job_in_s));
    new_job->type = type;
    new_job->from_client_fd = from_client_fd;
    //allocate for msg and then copy msg
    if(msg != NULL){
        int str_len = msg_len;
        new_job->msg = malloc(sizeof(char) * str_len);
        strncpy(new_job->msg, msg, msg_len);
    }
    else{
        new_job->msg = NULL;
    }
    //add to list
    insertTail(jobList, (void *)new_job);
}

//returns node of job(not node->data)
//does not free returned node
void* removeJob(List_t* jobList){
    if(jobList->length == 0){
        // pthread_mutex_lock(&audit_lock);
        // fprintf(audit_fp, "in removeJob: jobList is empty\n");
        // pthread_mutex_unlock(&audit_lock);
        return NULL;
    }
    return detachHead(jobList);
}

//deletes msg in job
void deleteJobContents(job_in_s* to_delete){
    free(to_delete->msg);
}

//deletes job contents and node->data
void deleteJob(void* to_delete){
    //delete the contents of job
    deleteJobContents((job_in_s*)(((node_t*)to_delete)->data));
    //delete the data struct
    free(((node_t*)to_delete)->data);
}

// Returns user's fd 
// returns -1 upon failure 
int userFdLookUp(List_t* user_li, char *username){
    if(user_li->head == NULL){
        // pthread_mutex_lock(&audit_lock);
        // fprintf(audit_fp, "Empty user list");
        // pthread_mutex_unlock(&audit_lock);
        return -1;
    }
    node_t *current;
    current = user_li->head;
    while(current)
    {
        if(strcmp(((user_in_s*)(current->data))->name, username) == 0)
        {
            return ((user_in_s*)current->data)->user_fd;    
        }
        current = current->next;
    }
    return -1;
}


void createRoom(List_t* roomList, char* roomName, List_t* userList, char* creator, int userfd){
    //make a new room
    room_in_s* new_room;
    new_room = malloc(sizeof(room_in_s));
    strcpy(new_room->roomName, roomName);
    strcpy(new_room->creator, creator);
    //allocate data for user list in room
    new_room->usersInRoom = malloc(sizeof(List_t *));
    new_room->usersInRoom->length = 0;
    new_room->usersInRoom->head = NULL;
    insertHead(roomList, new_room);
    //insert creator into room's userlist
    //insertUser(new_room->usersInRoom, creator, userfd);
    //update creator's roomJoined list
    
    addUserToRoom(roomList, roomName, userList, userfd);
    /*
    node_t* user_node = (node_t*)getUser(userList, creator);
    user_in_s* user = user_node->data;
    insertHead(user->rmJoined, (void*)new_room);
    //insert room into room list
    insertHead(roomList, (void*)new_room);
    */
}
//deletes room and all of the contents inside room_in_s
int deleteRoom(List_t* roomList, char* roomName, List_t* userList, int client_fd, int msg_type){
    int retval;
    petr_header header;
    header.msg_len = 0;
    header.msg_type = 255;
    if(roomList->length == 0){
        // pthread_mutex_lock(&audit_lock);
        // fprintf(audit_fp, "in deleteRoom: empty roomList\n");
        // pthread_mutex_unlock(&audit_lock);
        return 0;
    }
    //iterate through the list until data->roomName is fine
    node_t* room = roomList->head;
    node_t* prev_room = NULL;
    while(room){
        //if there is a match, delete 
        if(strcmp(((room_in_s*)(room->data))->roomName, roomName) == 0){
            // retval is -1 upon error, userFd upon success
            retval = userFdLookUp(user_list,((room_in_s*)(room->data))->creator);
            
            // The client is the creator
            if(retval == client_fd){
                msgUserRmClosed((room_in_s*)(room->data), msg_type);
                
                // pthread_mutex_lock(&audit_lock);
                // fprintf(audit_fp, "RMDELETE %s by %s\n", ((room_in_s*)(room->data))->roomName, ((room_in_s*)(room->data))->creator);
                // pthread_mutex_unlock(&audit_lock);
                
                /*
                if(prev_room){
                    prev_room->next = room->next;
                }
                //removing the head of the list
                else{
                    //update head of list
                    roomList->head = room->next;
                }
                */
                room_in_s* cleared = (room_in_s*)clearRoom(roomList, roomName, userList);
                void* room_node = detachNode(roomList, cleared);
                free(cleared->usersInRoom);
                // deleteListNodes(((room_in_s*)(room))->usersInRoom);
                //deleteRoomContents((room_in_s*)(room->data));
                //roomList->length--;
                free(room);
                //free(room_node);
                return 1;
            }
            // Client is not the creator, ERMDENIED
            header.msg_type = ERMDENIED;
            write(client_fd, &header, 8);
            return 1;
        }
        prev_room = room;
        room = room->next;
    }
    header.msg_type = ERMNOTFOUND;
    write(client_fd, &header, 8);
    return 1;
}

//deletes usersInRoom in room to_delete
void deleteRoomContents(room_in_s* to_delete){
    deleteListNodes(to_delete->usersInRoom);
    free(to_delete->usersInRoom);
}
//returns room (not node_t)
//returns NULL if roomlist is empty or room wasn't found
void* getRoom(List_t* roomList, char* roomName){
    if(roomList->length == 0){
        // pthread_mutex_lock(&audit_lock);
        // printf("in getRoom: empty roomList\n");
        // pthread_mutex_unlock(&audit_lock);
        return NULL;
    }
    //iterate through the list until data->roomName is fine
    node_t* room = roomList->head;
    node_t* prev_room = NULL;
    while(room){
        //if there is a match, delete 
        if(strcmp(((room_in_s*)(room->data))->roomName, roomName) == 0){
            return room->data;
        }
        prev_room = room;
        room = room->next;
    }
    //if the room wasn't found, returns NULL
    return NULL;
}

void* getRoomUserList(List_t* roomList, char* roomName){
    if(roomList->length <= 0){
        // pthread_mutex_lock(&audit_lock);
        // fprintf(audit_fp, "in getRoom: empty roomList\n");
        // pthread_mutex_unlock(&audit_lock);
        return NULL;
    }
    //get the room
    room_in_s* room = (room_in_s*)getRoom(roomList, roomName);
    //return the user list
    if(room) return room->usersInRoom;
    //if getRoom returned NULL, return NULL
    return NULL;
}

void printUsersInRoom(List_t* usersInRoom, char *proc_buffer){
    char comma = ',';
    node_t *current = usersInRoom->head;
    while(current)
    {
        user_in_s *user_info = (user_in_s *)current->data;
        strncat(proc_buffer, user_info->name, strlen(user_info->name));
        if(current->next != NULL) strncat(proc_buffer, &comma, 1);
        current = current->next;
    }
}

void printRoomListWithUsers(List_t* room_list, char *proc_buffer){
    char newline = '\n', colon = ':', space = ' ';
    node_t *current = room_list->head;
    bzero(proc_buffer, BUFFER_SIZE);
    while(current)
    {
        room_in_s *room_info = (room_in_s *)current->data;
        strncat(proc_buffer, room_info->roomName, strlen(room_info->roomName));
        strncat(proc_buffer, &colon, 1);
        strncat(proc_buffer, &space, 1);
        printUsersInRoom(room_info->usersInRoom, proc_buffer);
        strncat(proc_buffer, &newline, 1);
        current = current->next;
    }
}

//returns 1 if room exists, otherwise returns 0
int roomExists(List_t* roomList, char* roomName){
    if(getRoom(roomList, roomName)) return 1;
    return 0;
}
//adds user to room (puts pointer to user in roomlist)
//adds room to user's rmJoined (puts room pointer in rmJoined)
//does not create a new user (uses an existing user in the user_list)
int addUserToRoom(List_t* roomList, char* roomName, List_t* userList, int userfd){
    void* tempRoom = getRoom(roomList, roomName);
    room_in_s *room = (room_in_s *)tempRoom;
    //if room doesn't exist, return 0
    if(room == NULL){
        // pthread_mutex_lock(&audit_lock);
        // fprintf(audit_fp, "in addUserToRoom: room specified does not exist\n");
        // pthread_mutex_unlock(&audit_lock);
        return 0;
    }
    //get username
    char* username = getUserName(user_list, userfd);
    //add user from userList to to room->usersInRoom
    //get user from user_list
    node_t* user = (node_t*)getUserFromFD(userList, userfd);
    //add room to user's rmJoined
    insertHead(((user_in_s*)(user->data))->rmJoined, room);    
    //add user to room's userInRoom
    insertHead(room->usersInRoom, user->data);
    return 1;
}

int removeUserFromRoom(List_t* roomList, char* roomName, List_t* userList, int userfd, int msg_type){
    room_in_s* room = getRoom(roomList, roomName);
    //if room doesn't exist, return 0
    if(room == NULL){
        // pthread_mutex_lock(&audit_lock);
        // fprintf(audit_fp, "in removeUserFromRoom: room specified does not exist\n");
        // pthread_mutex_unlock(&audit_lock);
        return 0;
    }
    //return removeUser(((room_in_s*)room)->usersInRoom, userfd);

    //--------below is new code ------------
    //get the user from the user list
    node_t* user = (node_t*)getUserFromFD(userList, userfd);
    node_t* room_node;
    node_t* user_node;
    List_t* joined_rooms = (List_t*)getJoinedRooms(userList, ((user_in_s*)(user->data))->name);
    if(user == NULL) return 0;
    //detach the room from user's rmJoined list
    room_node = detachNode(joined_rooms, room);
    if(room_node == NULL){
        //if detachNode was not successful, return 0
        return 0;
    }
    //detach user from room's usersInRoom list
    user_node = detachNode(room->usersInRoom, user->data);
    if(user_node == NULL){
        return 0;
    }
    //free those nodes (but not the inside data)
    free(user_node);
    free(room_node);
    //return success
    return 1;
}

int validNameLength(char *name){
    int name_len = strlen(name);
    if(name_len > NAME_LEN)
        return 0;
    return 1;
}

//buffer = unparsed command
//will parse buffer and add job to the list
//returns 1 if success, 0 if failure
int insertJob(List_t* jobList, char* buffer, int from_client_fd){
    if(buffer == NULL){
        // pthread_mutex_lock(&audit_lock);
        // fprintf(audit_fp, "in insertJob: buffer is NULL\n");
        // pthread_mutex_unlock(&audit_lock);
        return 0;
    }
    if(from_client_fd < 0){
        // pthread_mutex_lock(&audit_lock);
        // fprintf(audit_fp, "in insertJob: from_client_fd < 0\n");
        // pthread_mutex_unlock(&audit_lock);
        return 0;
    }
    if(jobList == NULL){
        // pthread_mutex_lock(&audit_lock);
        // fprintf(audit_fp, "in insertJob: jobList is NULL\n");
        // pthread_mutex_unlock(&audit_lock);
        return 0;
    }
    //take the buffer and insert job according job type
    //if job msg exists, send in the address of the job msg to addJob
    //addJob needs:
    //  1. jobList 
    //  2. address of msg (if exists)
    //  3. job_type
    //  4. from_client_fd
    char* msg = NULL;
    uint32_t msg_len = jobMsgLen(buffer);
    uint8_t job_type = jobType(buffer);
    //if the job request is invalid, write error and return 0 
    if(job_type != LOGOUT && job_type != RMCREATE && job_type != RMDELETE &&
        job_type != RMCLOSED && job_type != RMLIST && job_type != RMJOIN &&
        job_type != RMLEAVE && job_type != USRSEND && job_type != USRLIST &&
        job_type != RMSEND)
    {
        // pthread_mutex_lock(&audit_lock);
        // fprintf(audit_fp, "in insertJob: job_type(%d) was invalid\n", job_type);
        // pthread_mutex_unlock(&audit_lock);
        return 0;
    }
    //set the job msg if it necessary
    if(job_type != LOGOUT && 
        job_type != RMLIST &&
        job_type != USRLIST)
    {
        msg = jobMsg(buffer);
    }
    addJob(jobList, job_type, msg, msg_len, from_client_fd);
    
    return 1;
}


//buffer == start of petr_header
//returns job_type as specified in petr_header
//NOTE: these do not do error checking
int jobType(char* buffer){
    return ((petr_header*)buffer)->msg_type;
}
//returns the length of msg
int jobMsgLen(char* buffer){
    return ((petr_header*)buffer)->msg_len;
}
//returns job_msg
char* jobMsg(char* buffer){
    return (buffer+8);
}

int msgUserRmClosed(room_in_s* room, int msg_type){
    List_t *userList = room->usersInRoom;
    node_t *current = userList->head;
    petr_header job_header;
    job_header.msg_type = 0;
    job_header.msg_len = 0;
    char buffer[NAME_LEN+8];
    int retval;
    // No users in the room
    if(!current){
        // pthread_mutex_lock(&audit_lock);
        // fprintf(audit_fp, "No one in the room: %s", room->roomName);
        // pthread_mutex_unlock(&audit_lock);
        return 0;
    }
    
    user_in_s* current_user;
    while(current){
        bzero(buffer, NAME_LEN+8);
        current_user = current->data;
        if(isRoomCreator(room, current_user)){
            // if(msg_type != LOGOUT)
            // {
                job_header.msg_len = 0;
                job_header.msg_type = OK;
                write(current_user->user_fd, &job_header, 8);
            // }
        }
        else
        {
            job_header.msg_len = strlen(room->roomName) + 1;
            job_header.msg_type = RMCLOSED;
            memcpy(buffer, &job_header, 8);
            memcpy(buffer+8, room->roomName, strlen(room->roomName));    
            retval = write(current_user->user_fd, buffer, job_header.msg_len + 8);
        }
        // pthread_mutex_lock(&audit_lock);
        // fprintf(audit_fp, "RMCLOSED (Creator: %s, closed the room): %i %s", room->creator, ((user_in_s *)current->data)->user_fd, ((user_in_s *)current->data)->name);
        // pthread_mutex_unlock(&audit_lock);
        current = current->next;
    }
    return 1;
}



//to_detach == data inside node to search for
//returns a node_t (not node_t->data)
//returns NULL if list, list->head == NULL, list->length == 0
//returns NULL if not to_detach is not found in list
void* detachNode(List_t* list, void* to_detach){
    if(list == NULL){
        return NULL;
    }
    if(list->head == NULL){
        return NULL;
    }
    if(list->length <= 0){
        return NULL;
    }
    //get to_detach from the list and return it
    node_t* current = list->head;
    node_t* prev = NULL;
    while(current){
        if(current->data == to_detach){
            //update head if necessary
            if(current == list->head){
                list->head = current->next;
            }
            else{
                prev->next = current->next;
            }
            list->length--;
            return current;
        }
        prev = current;
        current = current->next;
    }
    return NULL;
}

//returns List_t* of joined rooms that user is in
//uses userList to fetch user under the name username
void* getJoinedRooms(List_t* userList, char* username){
    if(userList == NULL){
        return NULL;
    }
    if(userList->head == NULL){
        return NULL;
    }
    if(userList->length == 0){
        return NULL;
    }
    if(username == NULL){
        return NULL;
    }
    node_t* user = (node_t*)getUser(userList, username);
    //check if user exists in the list
    if(user == NULL){
        return NULL;
    }
    return ((user_in_s*)(user->data))->rmJoined;
}
//returns list of joined rooms that user is in
//uses userList to fetch user under the FD userfd
void *getJoinedRoomsFromFD(List_t* userList, int userfd){
    char* username = getUserName(userList, userfd);
    return getJoinedRooms(userList, username);
}

void printJoinedRooms(List_t* userList, char* username){
    List_t* joined_rooms = (List_t*)getJoinedRooms(userList, username);
    if(joined_rooms == NULL){
        printf("in printJoinedRooms: joined_rooms == NULL\n");
        return;
    }
    node_t* current = joined_rooms->head;
    room_in_s* room;
    printf("(");
    while(current){
        room = current->data;
        printf("%s, ", room->roomName);
        current = current->next;
    }
    printf(")\n");
}
void printJoinedRoomsFromFD(List_t* userList, int userfd){
    List_t* joined_rooms = (List_t*)getJoinedRoomsFromFD(userList, userfd);
    node_t* current = joined_rooms->head;
    room_in_s* room;
    while(current){
        room = current->data;
        printf("%s,", room->roomName);
        current = current->next;
    }
}

void* getUsersInRoom(List_t* roomList, char* roomName){
    room_in_s* room = getRoom(roomList, roomName);
    if(room == NULL){
        return NULL;
    }
    return room->usersInRoom;
}
void printGetUsersInRoom(List_t* roomList, char* roomName){
    List_t* users_in_room = (List_t*)getUsersInRoom(roomList, roomName);
    if(users_in_room == NULL){
        printf("room is NULL\n");
        return;
    }
    node_t* current = users_in_room->head;
    user_in_s* user;
    printf("(");
    while(current){
        user = current->data;
        printf("%s,", user->name);
        current = current->next;
    }
    printf(")\n");
}
//returns 1 if user is creator of room; returns 0 otherwise
int isRoomCreator(room_in_s* room, user_in_s* user){
    if(room == NULL){
        return 0;
    }
    if(user == NULL){
        return 0;
    }
    if(strcmp(room->creator, user->name) == 0){
        return 1;
    }
    return 0;
}
//returns ptr to node of room that was cleared (node_t)
//clears out room and updates the user's rmJoined
//returns NULL on error (list empty, list == NULL, etc)
void* clearRoom(List_t* roomList, char* room_name, List_t* userList){
    if(roomList == NULL || room_name == NULL || userList == NULL)
    {
        return NULL;
    }
    if(room_list->head == NULL || userList->head == NULL ||
        room_list->length <= 0 || userList->length <= 0)
    {
        return NULL;
    }
    //get the room
    room_in_s* room = getRoom(roomList, room_name);
    //node_t* room_node = (node_t*)getRoom(roomList, room_name);
    if(room == NULL){
        return NULL;
    }
    
    //get users inside room
    List_t* usersInRoom = room->usersInRoom;
    //for each user, getUser and modify their rmJoined
    node_t* current = usersInRoom->head;
    user_in_s* user = NULL;
    void* detached = NULL;
    //continue until usersInRoom is cleared out
    while(current)
    {
        user = current->data;
        //detach room in rmJoined 
        detached = detachNode(user->rmJoined, room);
        free(detached);
        //detach user in usersInRoom
        detached = detachNode(room->usersInRoom, user);
        free(detached);
        current = current->next;
    }
    if(room->usersInRoom->length != 0){
        // pthread_mutex_lock(&audit_lock);
        // fprintf(audit_fp, "After clearRoom: usersInRoom->length != 0. Actual length:%d\n", room->usersInRoom->length);
        // pthread_mutex_unlock(&audit_lock);
    }
    return room;
}
void printAllRooms(List_t* roomList){
    printf("\n--------in printAllRooms--------\n");
    if(roomList == NULL){
        printf("in printAllRooms: roomList is NULL\n");
        return;
    }
    if(roomList->head == NULL){
        printf("in printAllRooms: roomList->head is NULL\n");
        printf("\troomList->length:%d\n", roomList->length);
        return;
    }
    printf("number of rooms:%d\n", roomList->length);
    node_t* current = roomList->head;
    room_in_s* room = current->data;
    while(current){
        printf("roomName:%s, usersInRoom:%d\n", room->roomName, room->usersInRoom->length);
        printf("usersInRoom:(");
        printGetUsersInRoom(roomList, room->roomName);
        printf(")\n");
        current = current->next;
        printf("\n");
    }
    printf("\n--------end printAllRooms--------\n");
}
void printAllUsers(List_t* userList){
    printf("--------in printAllUsers--------\n");
    if(userList == NULL){
        printf("in printAllUsers: userList is NULL\n");
        return;
    }
    if(userList->head == NULL){
        printf("in printAllUsers: userList->head is NULL\n");
        printf("\tuserList->length:%d\n", userList->length);
        return;
    }
    printf("number of users:%d\n", userList->length);
    node_t* current = userList->head;
    user_in_s* user;
    while(current){
        user = current->data;
        printf("userName:%s, rmJoined:%d\n", user->name, user->rmJoined->length);
        printf("rooms Joined:(");
        printJoinedRooms(userList, user->name);
        printf(")\n");
        current = current->next;
        printf("\n");
    }
    printf("\n--------end printAllUsers--------\n");
}
/* Debugging Lines
- UsersInRoom Check
(node_t*)(((List_t*)(((room_in_s*)(((node_t*)(room_list->head))->data))->usersInRoom))->head))


*/
