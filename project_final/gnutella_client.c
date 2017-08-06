/*
 *  gnutella_client.c
 *  
 *
 *  Created by Ashiwan on 1/24/11.
 *  Copyright 2011 Purdue University. All rights reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h> 
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <stdint.h>
#include <ctype.h>
#include "gnutella_client.h"

int error_flag=0;
int info_flag=0;
int my_id=0;
int nodes=0;
struct neighbors *phead=NULL;
struct neighbors *eQhead=NULL;
char *my_ip;
int my_port=0;
struct my_query *myhead=NULL;
struct rcvd_query *rcvdhead=NULL;
int seq_no=1;


/**********************************************************************/
void send_query(char *fname, int ttl, char** my_list,int sd)
{
	int k;
	static char qbuf[BUF_LEN];
	static char tmpbuf[20];
	struct my_query *myp,*mytmp;
	struct neighbors *ntr;
	struct sockaddr_in pr;
	int numb;
	int just=0;
	
	for (k=0; my_list[k]!='\0'; k++) {
		if (strcmp(fname,my_list[k])==0) {
		    printf("FILE: %s - search within myself successful\n",fname);
			just=1;
			break;
		}
	}
	memset(qbuf,0,sizeof(qbuf));
	memset(tmpbuf,0,sizeof(tmpbuf));
	
	myp=(struct my_query *)malloc(sizeof(struct my_query));
	myp->msg_id=(char *)malloc(15*sizeof(char));
	myp->file=(char *)malloc(50*sizeof(char));
	
	(void)sprintf(tmpbuf, "%d,%d",my_id,seq_no);
	strncpy(myp->msg_id,tmpbuf,strlen(tmpbuf));
	strncpy(myp->file,fname,strlen(fname));
	if (just) {
		myp->ihave=1;
	} else {
		myp->ihave=0;
	}
	myp->qhit=0;
	(void)sprintf(qbuf, "QUERY ( %d,%d , %d , %s )",my_id,seq_no,ttl,fname);
	
	time(&myp->start);

	printf("Originating the Query: %s \n",qbuf);
	
	for (ntr=phead; ntr!=NULL; ntr=ntr->next) {
		memset(&pr, 0, sizeof(pr));	
		pr.sin_family = AF_INET;
		inet_aton(inet_ntoa(ntr->ipaddr),&pr.sin_addr);
		//nbr.sin_addr.s_addr = pntr->ipaddr; 
		pr.sin_port = htons(ntr->portnum);	
		if ((numb = sendto(sd, &qbuf, strlen(qbuf), 0, (struct sockaddr *)&pr,sizeof(struct sockaddr_in))) == -1) {
			perror("sendto fail");
			close(sd);
			exit(0);
		} else {
			if (info_flag)
				printf("Sending the query message to %s %d no bytes : %d\n",inet_ntoa(pr.sin_addr),ntohs(pr.sin_port),numb);
		}				
	}
	
	if(myhead==NULL) {
		myhead=myp;
		myp->next=NULL;
	} else {
		for(mytmp=myhead;mytmp->next!=NULL;mytmp=mytmp->next);
		mytmp->next=myp;
		myp->next=NULL;
	}
	seq_no=(seq_no+1)%MAX_SEQ;
}

/**********************************************************************/
int file_lookup(char *sfile, char **flist)
{
	int j;
	for (j=0; flist[j]!='\0'; j++) {
		if (strcmp(sfile,flist[j]) == 0) {
			return(1);
		}
	}
	return(0);
}
/**********************************************************************/
void send_establish_msg(int sok)
{
	struct neighbors *pntr;
	struct sockaddr_in nbr;
	int num;
	static char msg[BUF_LEN];
	
	for (pntr=phead; pntr!=NULL; pntr=pntr->next) {
		memset(&nbr, 0, sizeof(nbr));	
		nbr.sin_family = AF_INET;
		inet_aton(inet_ntoa(pntr->ipaddr),&nbr.sin_addr);
		//nbr.sin_addr.s_addr = pntr->ipaddr; 
		nbr.sin_port = htons(pntr->portnum);	
		(void)sprintf(msg, "%s %d","ESTABLISH-NEIGHBOR",my_id);
		if(info_flag)
		printf("Sending the message: %s\n",msg);
		if ((num = sendto(sok, &msg, strlen(msg), 0, (struct sockaddr *)&nbr,sizeof(struct sockaddr_in))) == -1) {
			perror("sendto fail");
			close(sok);
			exit(0);
		} else {
			if (info_flag)
				printf("Sending Establish-neighbor message to %s %d no bytes : %d\n",inet_ntoa(nbr.sin_addr),ntohs(nbr.sin_port),num);
		}				
	}
}
/**********************************************************************/
void parse_response(char *buf, char **list)
{
	char *temp;
	char *p1,*p2,*p3;
	struct neighbors *p,*itr;
	temp=(char *)malloc(20*sizeof(char));
	p1=(char *)malloc(20*sizeof(char));
	p2=(char *)malloc(20*sizeof(char));
	p3=(char *)malloc(20*sizeof(char));
	my_ip=(char *)malloc(20*sizeof(char));
	int i=0,j;
	
	sscanf(buf,"%*s %*s %d %*s %s %*s %d %*s %d %*s",&nodes,my_ip,&my_port,&my_id);
        if(info_flag)
	printf("Count is %d , My id is %d, My IP is %s, My port is %d\n",nodes,my_id,my_ip,my_port);
	
	while (sscanf(buf,"%s",temp)!=EOF) {
		if (strncmp(temp,"Files",5)!=0) {
			if(info_flag)
			printf("I got %s\n",temp);
			buf=buf+strlen(temp)+1;
		} else {
			if(info_flag)
			printf("I got %s\n",temp);
			break;
		}
	}
	buf=buf+strlen(temp)+1;
	while (1) {
		sscanf(buf,"%s",temp);
		if(strncmp(temp,"PeerInfo",8) !=0) {
			strcpy(list[i],temp);
			buf=buf+strlen(temp)+1;
			i++;
		} else {
			list[i]='\0';
			buf=buf+strlen(temp)+1;
			break;
		}
	}
	while ((sscanf(buf,"%s %s %s",p1,p2,p3) !=EOF)) {
		p=(struct neighbors *)malloc(sizeof(struct neighbors));
		p->neighborid=atoi(p1);
		inet_aton(p2,&(p->ipaddr));
		p->portnum=atoi(p3);
		if(phead==NULL) {
			phead=p;
			p->next=NULL;
		} else {
			for(itr=phead;itr->next!=NULL;itr=itr->next);
			itr->next=p;
			p->next=NULL;
		}
		buf=buf+strlen(p1)+strlen(p2)+strlen(p3)+3;
	}
	
}
/**********************************************************************/
int 
main(int argc, char *argv[])
{
	int addrflag=0, portflag=0,myflag=0,timeflag=0;
	char *tracker_ip;
	char *tracker_port,*srchtimeout;
	int opt;
	int i,j,pp,numbytes;
	int sockfd,maxfd;
	struct sockaddr_in trkraddr, from;
	fd_set readfd, writefd, rdmaster, wrmaster;
	struct timeval timeout;
	//timeout.tv_sec = 3; //Ashiwan set to max value to wait after sending a query
	//need to feed in remaining value of time to select
	timeout.tv_usec = 0;
	char *buffer;
	int addr_len;
	int flaga=0, flagb=0, flagc=0, flagd=0, flage=0;
	struct neighbors *iter,*node;
	char *my_id_str;
	char *count_str;
	my_id_str=(char *)malloc(5*sizeof(char));
	count_str=(char *)malloc(5*sizeof(char));
	buffer=(char *)malloc(BUF_LEN);	
	char **file_list;
	file_list=(char **)malloc(10*sizeof(char *));
	for (i=0; i<10; i++) {
		file_list[i]=(char *)malloc(50*sizeof(char));
	}
	int est_id;
	struct neighbors *qnode,*qitr;
	int ret;
	struct neighbors *p,*itr;
	struct rcvd_query *q,*qtmp;
	struct my_query *myitr,*pvs;
	char user_buffer[1024];
	char *tmp, *nextstr=NULL;
	int ttl=0;
	int searchtimeout=0;
	char *srchfile;
	char *srchmsgid;
	char *hitmsgid;
	char *hitip;
	int srchttl;
	srchfile=(char *)malloc(50*sizeof(char));
	srchmsgid=(char *)malloc(15*sizeof(char));
	hitmsgid=(char *)malloc(15*sizeof(char));
	hitip=(char *)malloc(20*sizeof(char));
	struct sockaddr_in fwd;
	int hitid,hitport;
	time_t curr, lastupd;
	lastupd=time(0);
	
	
	if (argc>9) {
		printf("%%Error: Greater than supported input options for client\n");
		return 0;
	}
	
	tracker_ip=(char *)malloc(10*sizeof(char));
	tracker_port=(char *)malloc(5*sizeof(char));
	srchtimeout=(char *)malloc(5*sizeof(char));
	
	while ((opt = getopt(argc,argv,"a:p:t:ei")) != -1 ){
		switch (opt) {				
			case 'a':
				tracker_ip=optarg;
				addrflag=1;
				break;
			case 'p':
				tracker_port=optarg;
				portflag=1;
				break;
			case 't':
				srchtimeout=optarg;
				timeflag=1;
				break;
			case 'e':
				error_flag=1;
				break;
			case 'i':
				info_flag=1;
				break;
			case '?':
				if ((optopt=='a') || (optopt=='p')) {
					fprintf(stderr,"The option -%c requires a config file name as an arg. a-tracker ip,p-tracker port,t-searchtimeout.\n",optopt);
				}
				else {
					fprintf(stderr,"Unknown character '\\x%x' in option.\n",optopt);
				}
				return 0;
		}
	}
	
	if((addrflag==0) || (portflag==0)) {
		printf("%%ERROR: Please specify the IP, port of tracker using -a, -p options\n");
		return 1;
	}
	if (timeflag==0) {
		searchtimeout=DEF_TIMEOUT;
	} else {
		searchtimeout=atoi(srchtimeout);
	}

	
	/* Open the UDP socket and listen on that*/
	if ((sockfd=socket(AF_INET, SOCK_DGRAM, 0))==-1) {
		perror("Socket Creation");
		return 1;
	}
	
	memset(&trkraddr, 0, sizeof(trkraddr));	
	trkraddr.sin_family = AF_INET;	
    trkraddr.sin_addr.s_addr = inet_addr(tracker_ip); 
	trkraddr.sin_port = htons(atoi(tracker_port));	

	if (info_flag) {
	    printf("%%INFO: Created a client UDP socket - %d \n",sockfd);
	}
	
	if ((numbytes = sendto(sockfd, &join, 22, 0, (struct sockaddr *)&trkraddr,sizeof(struct sockaddr_in))) == -1) {
		perror("sendto fail");
		return 1;
	} else {
		if (info_flag)
		printf("Sending JOIN message to tracker %s %d no bytes : %d\n",inet_ntoa(trkraddr.sin_addr),ntohs(trkraddr.sin_port),numbytes);
	}
	
	addr_len = sizeof(from);
	FD_SET(sockfd, &rdmaster);
	maxfd = sockfd;
	
	FD_SET(fileno(stdin),&rdmaster);
	timeout.tv_sec=0;
	
	/* select API call */
	while(1) {
		time(&curr);
		pvs=NULL;
		for (myitr=myhead; myitr!=NULL; pvs=myitr,myitr=myitr->next) {
			if ((int)(curr-(myitr->start))>=searchtimeout) {
				if (!myitr->qhit) {
					printf("Expiring a query which has not rcvd() any hit\n");
					printf("Query ID: %s expired\n",myitr->msg_id);
				} else {
					printf("Expiring a query which has rcvd() a hit\n");
					printf("Query ID: %s expired\n",myitr->msg_id);
}
				if (pvs==NULL) {
					//myitr->next=NULL;
					myhead=myhead->next;
				} else if (myitr->next==NULL) {
					pvs->next=NULL;
				} else {
					pvs->next=myitr->next;
				}
			}
		}						

		readfd = rdmaster;
		//writefd = wrmaster;
		
		if((ret=select(maxfd+1,&readfd,NULL,NULL,&timeout)) == -1) {
			perror("Select call failed");
			exit(1);
		}
		if(ret==0) {
			continue;
		}
		for(pp=0;pp<=maxfd;pp++) {
			if(FD_ISSET(pp,&readfd)) {
				if(pp==fileno(stdin)) {
					
					memset(&user_buffer,'\0',1024);
					fflush(stdout);
		            fflush(stdin);
		            if (!fgets(user_buffer, sizeof(user_buffer), stdin)) {
						break;
			    }					
		            if(user_buffer[strlen(user_buffer)-1]=='\n')
					   user_buffer[strlen(user_buffer)-1]='\0';
					tmp=strtok(user_buffer, " ");
					nextstr=strtok(NULL," ");
					if (tmp==NULL) {
						printf("%%ERROR: I did not receive any file name to query\n");						
					} else {
						if (nextstr==NULL) {
							printf("TTL not specified. Using default\n");
							ttl=DEF_TTL;
						} else {
							if(atoi(nextstr)==0) {
							  printf("%%ERROR: Cannot start a Query with TTL 0. Aborting the search\n");
							  break;
							}
							ttl=atoi(nextstr);
						}
						send_query(tmp,ttl,file_list,sockfd);
					}					
					
				} else {
					if((numbytes = recvfrom(pp,buffer, BUF_LEN,0,(struct sockaddr *)&from, &addr_len)) == -1) {
						perror("recvfrom call");
						return 1;
					}
					if (strncmp(buffer,"RESPONSE",8) ==0) {					
						if ((strcmp(inet_ntoa(from.sin_addr),inet_ntoa(trkraddr.sin_addr)) != 0) && ((int) ntohs(from.sin_port) != (int) ntohs(trkraddr.sin_port))) {
							printf("%%ERROR: Rcvd() response from a random server not the tracker!\n");
						} else {
							flaga=1;
						}
						if (flaga) {
							printf(" I rvcd() response from %s %d num bytes is %d\n",inet_ntoa(from.sin_addr),(int) ntohs(from.sin_port),numbytes);
							printf("%s",buffer);
							printf("\n");
							parse_response(buffer,file_list);
							if(info_flag) {
							   for (iter=phead; iter!=NULL; iter=iter->next) {
								printf("Peer id is %d\n",iter->neighborid);
								printf("Peer IP is %s\n",inet_ntoa(iter->ipaddr));
								printf("Peer port no is %d\n",iter->portnum);
							   }
							}
							send_establish_msg(sockfd);
							flagb=1;
						}
						memset(buffer,'\0',BUF_LEN);
						//break;
					} else if (strncmp(buffer,"ESTABLISH-NEIGHBOR",18) ==0) {
							if (flagb) {
								sscanf(buffer,"%*s %d",&est_id);

								printf(" I rvcd() establish-neighbor from ID: %d %s %d num bytes is %d\n",est_id,inet_ntoa(from.sin_addr),(int) ntohs(from.sin_port),numbytes);
								for (iter=phead; iter!=NULL; iter=iter->next) {
									if (iter->neighborid==est_id) {
										if(info_flag)
										printf("I already have the neighbor in my list\n");
										flagc=1;
									} 
								}
								if (!flagc) {
									p=(struct neighbors *)malloc(sizeof(struct neighbors));
									p->neighborid=est_id;
									p->ipaddr=from.sin_addr;
									p->portnum=(int) ntohs(from.sin_port);									
									if(phead==NULL) {
										phead=p;
										p->next=NULL;
									} else {
										for(itr=phead;itr->next!=NULL;itr=itr->next);
										itr->next=p;
										p->next=NULL;
									}
								}
							} else {
								//Did not get response yet. So add establish message to a Q
								sscanf(buffer,"%*s %d",&est_id);

								printf(" I rvcd() establish-neighbor from ID: %d %s %d num bytes is %d\n",est_id,inet_ntoa(from.sin_addr),(int) ntohs(from.sin_port),numbytes);
								qnode=(struct neighbors *)malloc(sizeof(struct neighbors));
								qnode->neighborid=est_id;
								qnode->ipaddr=from.sin_addr;
								qnode->portnum=(int) ntohs(from.sin_port);									
								if(eQhead==NULL) {
									eQhead=qnode;
									qnode->next=NULL;
								} else {
									for(qitr=eQhead;qitr->next!=NULL;qitr=qitr->next);
									qitr->next=qnode;
									qnode->next=NULL;
								}
							}
						memset(buffer,'\0',BUF_LEN);
					} else if (strncmp(buffer,"QUERY",5) ==0) {
						printf(" I rvcd() Query from %s %d num bytes is %d\n",inet_ntoa(from.sin_addr),(int) ntohs(from.sin_port),numbytes);
						printf("%s\n",buffer);
						sscanf(buffer,"%*s %*s %s %*s %d %*s %s",srchmsgid,&srchttl,srchfile);
						flage=0;
						int oldflag=0;
						for (itr=phead; itr!=NULL; itr=itr->next) {
							if ((strcmp(inet_ntoa(from.sin_addr),inet_ntoa(itr->ipaddr)) == 0) && ((int) ntohs(from.sin_port) == itr->portnum)) {
								flage=1;
								break;
							}
						}
						for (qtmp=rcvdhead; qtmp!=NULL; qtmp=qtmp->next) {
							if (strcmp(srchmsgid,qtmp->messageid)==0) {
								printf("I have seen this query already. Discarding\n");
								oldflag=1;
								break;
							}
						}
						if (!flage) {
							printf("Recvd query from someone who is not my neighbor. Discarding\n");
						} else {
							if (!oldflag) {
								if(file_lookup(srchfile,file_list)) {
								//Send a Query hit message to the sender of the query with my IP, ID,port.
									char hbuf[100];
									(void)sprintf(hbuf, "QRYHIT ( %s , %s , %d , %s , %d )",srchmsgid,srchfile,my_id,my_ip,my_port);
									printf("Sending query hit %s\n",hbuf);
									memset(&fwd, 0, sizeof(fwd));	
									fwd.sin_family = AF_INET;
									inet_aton(inet_ntoa(from.sin_addr),&fwd.sin_addr);
									//nbr.sin_addr.s_addr = pntr->ipaddr; 
									fwd.sin_port = from.sin_port;	
									if ((numbytes = sendto(sockfd, &hbuf, strlen(hbuf), 0, (struct sockaddr *)&fwd,sizeof(struct sockaddr_in))) == -1) {
										perror("sendto fail");
										close(sockfd);
										exit(0);
									} else {
										if (info_flag)
											printf("Sending the query hit message to %s %d no bytes : %d\n",inet_ntoa(fwd.sin_addr),ntohs(fwd.sin_port),numbytes);
									}								
								}
								srchttl--;
								if (srchttl==0) {
									printf("TTL for Query %s expired\n",buffer);
								} else {
									char sbuf[100];
									(void)sprintf(sbuf, "QUERY ( %s , %d , %s )",srchmsgid,srchttl,srchfile);
									printf("Forwarding query %s\n",sbuf);
									for (itr=phead; itr!=NULL; itr=itr->next) {
										if ((strcmp(inet_ntoa(from.sin_addr),inet_ntoa(itr->ipaddr)) == 0) && ((int) ntohs(from.sin_port) == itr->portnum)) {
} else {
										    memset(&fwd, 0, sizeof(fwd));	
											fwd.sin_family = AF_INET;
											inet_aton(inet_ntoa(itr->ipaddr),&fwd.sin_addr);
											//nbr.sin_addr.s_addr = pntr->ipaddr; 
											fwd.sin_port = htons(itr->portnum);	
											if ((numbytes = sendto(sockfd, &sbuf, strlen(sbuf), 0, (struct sockaddr *)&fwd,sizeof(struct sockaddr_in))) == -1) {
												perror("sendto fail");
												close(sockfd);
												exit(0);
											} else {
												if (info_flag)
													printf("Forwarding the query message to %s %d no bytes : %d\n",inet_ntoa(fwd.sin_addr),ntohs(fwd.sin_port),numbytes);
											}																														
									    }
									}
								}
								//Add the query to the list of seen queries
								q=(struct rcvd_query *)malloc(sizeof(struct rcvd_query));
								q->messageid=(char *)malloc(15*sizeof(char));
								q->reqfile=(char *)malloc(50*sizeof(char));
								strcpy(q->messageid,srchmsgid);
								strcpy(q->reqfile,srchfile);
								inet_aton(inet_ntoa(from.sin_addr),&(q->qryip));
								q->qport=(int) ntohs(from.sin_port);
								if(rcvdhead==NULL) {
									rcvdhead=q;
									q->next=NULL;
								} else {
									for(qtmp=rcvdhead;qtmp->next!=NULL;qtmp=qtmp->next);
									qtmp->next=q;
									q->next=NULL;
								}								
							}
						}
						memset(buffer,'\0',BUF_LEN);
					} else {
						if (strncmp(buffer,"QRYHIT",6) == 0) {
							int flagf=0;
							printf(" I rvcd() Query Hit from %s %d num bytes is %d\n",inet_ntoa(from.sin_addr),(int) ntohs(from.sin_port),numbytes);
							printf("%s\n",buffer);
							sscanf(buffer,"%*s %*s %s %*s %*s %*s %d %*s %s %*s %d",hitmsgid,&hitid,hitip,&hitport);
							
							for (myitr=myhead; myitr!=NULL; myitr=myitr->next) {
								if (strcmp(hitmsgid,myitr->msg_id)==0) {
									printf("File %s Search successful\n",myitr->file);
									printf("File is with %d %s %d\n",hitid,hitip,hitport);
									myitr->qhit=1;
									if (myitr->ihave == 0) {
										for (i=0; file_list[i]!='\0'; i++);
										file_list[i]=malloc(50*sizeof(char));
										strcpy(file_list[i++],myitr->file);
										file_list[i]='\0';
									}
									flagf=1;
									break;
								}
							}
							if (!flagf) {
								for (qtmp=rcvdhead; qtmp!=NULL; qtmp=qtmp->next) {
									if (strcmp(hitmsgid,qtmp->messageid)==0) {
										printf("Forwarding query hit %s\n",buffer);
										memset(&fwd, 0, sizeof(fwd));	
										fwd.sin_family = AF_INET;
										inet_aton(inet_ntoa(qtmp->qryip),&fwd.sin_addr);
										//nbr.sin_addr.s_addr = pntr->ipaddr; 
										fwd.sin_port = htons(qtmp->qport);	
										if ((numbytes = sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&fwd,sizeof(struct sockaddr_in))) == -1) {
											perror("sendto fail");
											close(sockfd);
											exit(0);
										} else {
											if (info_flag)
												printf("Forwarding the query hit message to %s %d no bytes : %d\n",inet_ntoa(fwd.sin_addr),ntohs(fwd.sin_port),numbytes);
										}								
										break;
									}
								}
							}
							memset(buffer,'\0',BUF_LEN);
						}
					}

					if (flagb) {
						//Take the messages from establish Q and add them to neighbor list.
						while (eQhead!=NULL) {
							flagd=0;
							for (itr=phead; itr!=NULL; itr=itr->next) {
								if (itr->neighborid==eQhead->neighborid) {
									flagd=1;
								}
							}
							if (!flagd) {
								itr->next=eQhead;
								eQhead=eQhead->next;
								itr->next->next=NULL;								
							} else {
								eQhead=eQhead->next;
							}
						}
					}																				
				}
			}
		}
	}
    
	return EXIT_SUCCESS;
	
}
