#ifndef SERVER_H
#define SERVER_H

#include <arpa/inet.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define NAME_LEN 512
#define SA struct sockaddr

//node for generic linked list
struct node{
	void* data;
	struct node* next;
};
typedef struct node node_t;

//generic linked list
struct List{
	node_t* head;
	int length;
};
typedef struct List List_t;

struct user_in{
	char name[NAME_LEN];
	int user_fd;
	List_t *rmJoined;
	//struct user_in *next;
};
typedef struct user_in user_in_s;

struct room_in{
	char roomName[NAME_LEN];
	char creator[NAME_LEN];
	List_t *usersInRoom;
};
typedef struct room_in room_in_s;

struct job_in{
	uint8_t type;
	char *msg;
	int from_client_fd;
};
typedef struct job_in job_in_s;

void run_server(int server_port, int Nthread);

void insertHead(List_t* list, void* to_add);
void insertTail(List_t* list, void* to_add);
int removeNode(List_t* list, void* to_remove);
int removeHead(List_t* list);
void* detachHead(List_t* list);
void* detachNode(List_t* list, void* to_detach);
void deleteList(List_t* list);
void deleteListNodes(List_t* list);

void insertUser(List_t* list, char* user_name, int userfd);	//inserts at head
void* getUser(List_t* user_li, char* user_name);
void* getUserFromFD(List_t* user_li, int userfd);
char* getUserName(List_t* user_li, int userfd);
int userExists(List_t* user_li, char* user_name);
int userFdLookUp(List_t* user_li, char *username);
int removeUser(List_t* user_li, int client_fd);
void deleteUserList(List_t* list);
void* getJoinedRooms(List_t* userList, char* username);
void *getJoinedRoomsFromFD(List_t* userList, int userfd);

void addJob(List_t* jobList, uint8_t type, char* msg, uint32_t msg_len, int from_client_fd); //add to tail of list
void* removeJob(List_t* jobList);	//remove from head of joblist
void deleteJobContents(job_in_s* to_delete);
void deleteJob(void* to_delete);	

void createRoom(List_t* roomList, char* roomName, List_t* userList, char* creator, int userfd);
int addUserToRoom(List_t* roomList, char* roomName, List_t* userList, int userfd);
int insertJob(List_t* jobList, char* buffer, int from_client_fd);
int jobType(char* buffer);
int jobMsgLen(char* buffer);
char* jobMsg(char* buffer);
int roomExists(List_t* roomList, char* roomName);
void* clearRoom(List_t* roomList, char* room_name, List_t* userList);
void* getRoom(List_t* roomList, char* roomName);
int deleteRoom(List_t* roomList, char* roomName, List_t* userList, int client_fd, int msg_type);
int msgUserRmClosed(room_in_s* room, int msg_type);
void deleteRoomContents(room_in_s* to_delete);
int removeUserFromRoom(List_t* roomList, char* roomName, List_t* userList, int userfd, int msg_type);
int isRoomCreator(room_in_s* room, user_in_s* user);
//gets List_t* of usersInRoom of room given
void* getUsersInRoom(List_t* roomList, char* roomName);
//printing functions
//prints users inside room (room->usersInRoom)
void printGetUsersInRoom(List_t* roomList, char* roomName);
//prints rooms that user is in (user->rmJoined)
void printJoinedRoomsFromFD(List_t* userList, int userfd);
//prints rooms that user is in (user->rmJoined)
void printJoinedRooms(List_t* userList, char* username);
void printAllRooms(List_t* roomList);
void printAllUsers(List_t* userList);
//copies users in room into proc_buffer
void printUsersInRoom(List_t* room_in_s, char *proc_buffer);
//copies rooms with users into proc_buffer
void printRoomListWithUsers(List_t* room_list, char *proc_buffer);

int validNameLength(char *names);


#endif
