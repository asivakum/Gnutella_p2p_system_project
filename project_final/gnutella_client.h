/*
 *  gnutella_client.h
 *  
 *
 *  Created by Ashiwan on 1/24/11.
 *  Copyright 2011 Purdue University. All rights reserved.
 *
 */

#ifndef	_gnutella_client_h_
#define	_gnutella_client_h_


const char join[]="This is a JOIN message";

#define BUF_LEN 3000
#define DEF_TTL 3
#define DEF_TIMEOUT 30
#define MAX_SEQ 32767

struct neighbors {
	struct in_addr ipaddr;
	unsigned int portnum;
	int neighborid;
	struct neighbors *next;
};

struct my_query {
	char *msg_id;
	char *file;
	int ihave;
	int qhit;
	time_t start;
	struct my_query *next;
};

struct rcvd_query {
	char *messageid;
	char *reqfile;
	struct in_addr qryip;
	unsigned int qport;
	struct rcvd_query *next;
};










#endif // _gnutella_client_h_
