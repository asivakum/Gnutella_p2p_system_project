/*
 *  Tracker.h
 *  
 *
 *  Created by Ashiwan on 1/23/11.
 *  Copyright 2011 Purdue University. All rights reserved.
 *
 */

#ifndef	_Tracker_h_
#define	_Tracker_h_

//extern int count;

// The config Database which holds the list of peer IDs, Initial file list of each client read from config file

typedef struct topo {
	int my_id;
	int *peer_ids;
	char **initial_file_list;
	struct topo *next;
} cfg;

// The default values for tracker port no and max length of file name
#define MYPORT 8500
#define INITIAL_FILE_COUNT 10
#define BUF_LEN 2000


typedef struct allocated {
	int client_id;
	char ip_addr[20];
	unsigned int port;
	struct  allocated *next;
} AllocTable;




#endif // _Tracker_h_
