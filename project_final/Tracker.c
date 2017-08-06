/*
 *  Tracker.c
 *  
 *
 *  Created by on 1/23/11.
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
#include "Tracker.h"

int error_flag =0;
int info_flag =0;
cfg *head=NULL;
cfg *free_head=NULL;
AllocTable *thead=NULL;
int count=0;


/**********************************************************************/
void
parse_config(char *configfile)
{
	FILE *fp;
	char line[100], line1[100];
	int i,j;
	cfg *temp, *p;
	char *token;
	char *next_string;
	char *subtoken, *str1, *str2, *saveptr1, *saveptr2;

	if ((fp = fopen(configfile,"r")) == NULL)
	{
		printf ("%%ERROR: Unable to open the config file.\n");
		exit(0);
	}
	if((fgets(line,sizeof(line),fp)) == NULL) {
		printf("%%ERROR: Config file corrupted.\n");
		fclose(fp);
		exit(0);
	}
	rewind(fp);
	
	while((fgets(line1, sizeof(line1),fp)) != NULL) {
		count++;
	}
		if (info_flag) {
			printf("%%INFO: Total no of nodes in system %d\n",count);
		}
        rewind(fp);
	for (i=0; i<count; i++) {
	  int temp1=0;
	  int temp2=0;
		fgets(line, sizeof(line),fp);
		for (j=0; j<100; j++) {
			if (line[j]=='\n') {
				line[j]='\0';
			}
		}
		if(info_flag) {
		   printf("the line: %s\n",line);
		}
		token=strtok_r(line,":",&next_string);
		p=(cfg *)malloc(sizeof(cfg));
		p->my_id=atoi(token);
		if(info_flag) {
		   printf("my id is: %d\n",p->my_id);
		}
		p->peer_ids=(int *)malloc(count*sizeof(int));
		p->initial_file_list=(char **)malloc(INITIAL_FILE_COUNT*sizeof(char *));
		for(j=0;j<INITIAL_FILE_COUNT;j++) {
		  p->initial_file_list[j]=(char *)malloc(20*sizeof(char));
		}
		
		if(info_flag) {
		   printf("next string: %s\n",next_string);
		}
		for(j=1,str1=next_string; ;j++,str1=NULL) {
		  token=strtok_r(str1,":;",&saveptr1);
		  if(token==NULL) {
		    break; 
		  }
		  if(info_flag) {
		    printf("%d: %s\n",j,token);
		  }
		  if(j!=1) {
		    strcpy(p->initial_file_list[temp1],token);
			if(info_flag) {
		       printf("TOKEN: %s\n",p->initial_file_list[temp1]);
		    }
		    temp1++;
		  }
                  if(j==1) {
		     for(str2=token; ;str2=NULL) {
		         subtoken=strtok_r(str2, " ",&saveptr2);
		          if(info_flag)
		            printf("subtoken: %s\n",subtoken);
		         if(subtoken==NULL)
		            break;
		         (p->peer_ids[temp2])=atoi(subtoken);
		           if(info_flag)
		            printf("SUBTOKEN: \t----> %d\n",(p->peer_ids[temp2]));
		         temp2++;
		     }
		  }
	       }
			p->initial_file_list[temp1]='\0';
	       (p->peer_ids[temp2])=32767;
	       
	       if(head==NULL) {
		  head=p;
		  p->next=NULL;
	       } else {
		  for(temp=head;temp->next!=NULL;temp=temp->next);
		      temp->next=p;
		      p->next=NULL;
		}
	  }
}

/**********************************************************************/
int 
main(int argc, char *argv[])
{
	char *config_file=NULL;
	int cfgflag=0, pflag=0;
	unsigned int myport=0;
	int opt;
	int sockfd;
	int yes=1;
	struct sockaddr_in myaddr;
	struct sockaddr_in from;
	char buffer[1000]; 
	int addr_len;
	cfg *temp, *temp2;
	int i,j,numbytes;
	AllocTable *itr,*ptr;
        static char send_buf[BUF_LEN];
	static char temp_buf[BUF_LEN];
	if (argc>7) {
		printf("%%Error: Greater than supported input options for tracker\n");
		return 0;
	}
	while ((opt = getopt(argc,argv,"t:p:ei")) != -1 ){
		switch (opt) {				
			case 't':
				config_file=optarg;
				cfgflag=1;
				break;
			case 'e':
				error_flag=1;
				break;
			case 'i':
				info_flag=1;
				break;
		        case 'p':
			        pflag=1;
					myport=atoi(optarg);
				break;			
			case '?':
				if ((optopt=='t') || (optopt=='p')) {
					fprintf(stderr,"The option -%c requires an arg to be passed.t - config file, p - port no\n",optopt);
				}
				else {
					fprintf(stderr,"Unknown character '\\x%x' in option.\n",optopt);
				}
				return 0;
		 }
	}
	if(cfgflag==0) {
           printf("%%DEBUG: No config file specified.\n");
           printf("%%DEBUG: Using default file - config.\n");
	   config_file = "config";
	}
        if(pflag==0) {
	   printf("%%INFO: No port specified. Using default 8500\n");
           myport=MYPORT;
        }	
	if (info_flag) {
	    printf("%% INFO: Config file to be opened: %s\n",config_file);
	}
	
	/* Open the config file here and parse to get the list of peer IDs, files*/
	
	parse_config(config_file);

	/* Open the UDP socket and listen on that*/
	if ((sockfd=socket(AF_INET, SOCK_DGRAM, 0))==-1) {
		perror("Socket Creation");
		return 1;
	}
	setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int));
	memset((char *) &myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_port = htons(myport);
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	if (bind(sockfd,(struct sockaddr *) &myaddr, sizeof(myaddr))==-1) {
		perror("bind");
	    close(sockfd);
	    return 1;
	}
	
	if (info_flag) {
	    printf("%%INFO: Created a UDP socket - %d \n",sockfd);
	}
	
	/* Get a UDP message, IP and port no, check if it is JOIN */
	while(1) {
		addr_len = sizeof(from);
		if((numbytes = recvfrom(sockfd,&buffer, sizeof(buffer),0,(struct sockaddr *)&from, &addr_len)) == -1) {
			perror("recvfrom call");
			close(sockfd);
			return 1;
		}
		buffer[numbytes]='\0';
		printf("recv()'d %d bytes of data in buf\n", numbytes);
		printf("from IP address %s and port no %d\n",
		        inet_ntoa(from.sin_addr),(int) ntohs(from.sin_port));
		
		if ((strncmp(buffer,"This is a JOIN message",22)) != 0) {
			printf("%%ERROR: This is not a JOIN message! I am discarding it!!\n");
		} else if (head==NULL) {
			printf("%%ERROR: I am running out of configuration info!!\n");
		} else {
			ptr=(AllocTable *)malloc(sizeof(AllocTable));
			strcpy(ptr->ip_addr,inet_ntoa(from.sin_addr));
			ptr->port=(int) (ntohs(from.sin_port));
			ptr->client_id=head->my_id;
			memset(send_buf,0,sizeof(send_buf));
			memset(temp_buf,0,sizeof(temp_buf));
			(void)sprintf(send_buf, "RESPONSE Count %d IP %s Port %d ID %d Files",count,inet_ntoa(from.sin_addr),(int) ntohs(from.sin_port),head->my_id);
			int iter=0;
			while (head->initial_file_list[iter]!='\0') {
				(void)sprintf(temp_buf, " %s",head->initial_file_list[iter]);
				(void)strcat(send_buf, temp_buf);
				iter++;
			}
			(void)strcat(send_buf, " PeerInfo");
			for (j=0; (head->peer_ids[j])!=32767; j++) {
				(void)sprintf(temp_buf, " %d",head->peer_ids[j]);
				(void)strcat(send_buf, temp_buf);
			    for (itr=thead; itr!=NULL; itr=itr->next) {
					if (itr->client_id==((head->peer_ids[j]))) {
						(void)sprintf(temp_buf, " %s %d",itr->ip_addr,itr->port);
						(void)strcat(send_buf, temp_buf);
				    }
			    }
			}
			if(info_flag) {
                           printf("The response is \n");
			   printf("%s\n",send_buf);
			}
			if ((numbytes = sendto(sockfd, &send_buf, strlen(send_buf), 0, (struct sockaddr *)&from,sizeof(struct sockaddr_in))) == -1) {
				perror("sendto fail");
				close(sockfd);
				return 1;
			} else {
				if (info_flag)
				printf("Sending response to %s %d no bytes : %d\n",inet_ntoa(from.sin_addr),ntohs(from.sin_port),numbytes);
			}
			
				 //3. To move head to free list
				 if (free_head==NULL) {
					 free_head=head;
					 head=head->next;
					 free_head->next=NULL;					 
				 } else {
					 for (temp2=free_head; temp2->next!=NULL; temp2=temp2->next);
					 temp2->next=head;
					 head=head->next;
					 temp2->next->next=NULL;
				 }

				 
				 //4. To form a linked list of allocated nodes.
				 if(thead==NULL) {
					 thead=ptr;
					 ptr->next=NULL;
				 } else {
					 for(itr=thead;itr->next!=NULL;itr=itr->next);
					 itr->next=ptr;
					 ptr->next=NULL;
				 }

	}
	
  }
	
	close(sockfd);
	
	return 0;
	
}
