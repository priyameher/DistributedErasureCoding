/*
Implementing Object Storage-Put,Get,List
*/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<unistd.h>
#include<dirent.h>
#include<errno.h>
#include<sys/stat.h>
#include<fcntl.h>
#include "/home/srushti/isa-l/include/erasure_code.h"

#define NUM_DATA 8
#define NUM_PARITY 3
#define BUCKETSIZE 5
#define DIRECTORY_PATH "/home/srushti/Desktop/ObjectStorage/"

struct Node
{
	FILE *fp;
	int uid;
	char *filepath;
	long int size;
	struct Node *next;
};

int generateUid();
void menu(char[]);
int put(struct Node **,char[]);
int get(int,char *);
int isDir(const char *);
int putHash(struct Node **,int,char [],long int);
void display(struct Node*);
//int combineChunks(FILE *,FILE *); //for combining all the data chunks
void list(struct Node **);
void str_reverse(char *);
struct Node **arr;

//unsigned char gen[(NUM_DATA + NUM_PARITY) * (NUM_DATA + NUM_PARITY)];
unsigned char gen[(NUM_DATA + NUM_PARITY) * NUM_DATA]; //Generator matrix consist of identity matrix and parity matrix - gen[((8+3),3)]
char g_tbls[(NUM_DATA + NUM_PARITY) * (NUM_DATA + NUM_PARITY) * 32]; //g_tbls is internal structure used in ISA-L. Same as parity matrix.
char *databuffs; //databufs to be allocated using malloc as per file size
unsigned char *paritybuffs[NUM_PARITY];	//parities calculated will be stored here
unsigned char *datachunks[NUM_DATA]; //data from the chunk files will be stored here
int check_if_file_exists(char * newpath);

int main(void)
{	
	int ch=0;
	int uid=0;
	int succ=0;
	char filepath[100] = {'\0'};
	char targetFilePath[100] = {'\0'};
	int i=0; 
	int j=0;
	int k=0;
	int l=0;
	int m=0;
	int p=0;
	int q=0;
	char currentstring[100] = {'\0'};
	char choice[100] = {'\0'};

	setbuf(stdout, NULL);

	arr=(struct Node **)calloc(BUCKETSIZE, sizeof(struct Node *));
	
	do
	{
		strcpy(choice,"\0");
		menu(choice);

		if(choice[0]=='P'||choice[0]=='p'||choice[0]=='G'||choice[0]=='g')
		{
			for(i=0;choice[i]!=' ';i++)
			{
				currentstring[i] = choice[i];
			
			}
		}

		if(strcasecmp(currentstring, "Put") == 0)
		{	
			for(k=i+1, l=0; choice[k]!='\0'; k++, l++)
			{
				filepath[l]=choice[k];
				filepath[l+1]='\0';
			}

			uid=put(arr,filepath);

			if(uid!=-1)
			{
				printf("\n\tFile Split Successfully with UID : %d",uid);
			}
			else
			{
				printf("\n\tPut Operation Failed!!");
			}
		}
		else if(strcasecmp(currentstring, "Get") == 0)
		{
			char uid_c[5]="\0";

			for(j=i+1,m=0;choice[j]!=' ';j++,m++)
			{
				uid_c[m]=choice[j];	
			}
			sscanf(uid_c,"%d",&uid);
			
			for(p=j+1,q=0;choice[p]!='\0';p++,q++)
			{
				targetFilePath[q]=choice[p];
			}
			targetFilePath[q+1] = '\0';

			//printf("\n\tTarget File Path : %s", targetFilePath);

			succ=get(uid,targetFilePath);

			if(succ==0)
			{
				printf("\n\tThe File is retrieved successfully!!");
			}
			else
			{
				printf("\n\tGet Operation Failed!!");
			}

			memset(targetFilePath, '\0', strlen(targetFilePath));
		}
		else if(strcasecmp(choice, "List") == 0)
		{
			printf("\n\tListing the objects : ");
			list(arr);
			
		}
		strcpy(currentstring,"\0");
		
	}while(strcasecmp(choice,"EXIT")!=0);
	
	return 0;
}

int generateUid()
{
	static int uid=1;
	return uid++;
}

void menu(char choice[100])
{	
	printf("\n\n\t-------------------------------------------------------------------");
	printf("\n\tObjectStorage> ");
        printf("\n\t1. Type 'put' and path of file for creating an Object");
        printf("\n\t2. Type 'get', UID and path of file for getting an Object");
        printf("\n\t3. Type 'list' for getting the list of all objects in object storage");
        printf("\n\t4. 'exit'\n");
	printf("\n\tSelect choice : ");
	scanf("%[^\n]%*c", choice);
	printf("\t--------------------------------------------------------------------\n");
	
}

int get(int uid,char *targetFilePath)
{	
	FILE *fchunk=NULL;
	FILE *ft = NULL;	
	int succ=0;
	char *newFilePath = NULL;
	char *chunk = NULL;
	int flag=0;
	int flag2=0;
	int missingChunks=0;
	int i=0;
	int j=0;
	int k=0;
	char file_name[50]={'\0'};	
	char file_path[50]={'\0'};
	char int_str[20]={'\0'};
	char targetPath_old[50] = {'\0'};		// target file path before adding filename
	char *buff = NULL;
	int fch = 0;
	int fd = 0;
	int chunk_size = 0;
	long int file_size = 0;
	char directory[100] = {'\0'};
	int dirSucc = 0;
	int check=0;					
	int availableChunks[NUM_DATA + NUM_PARITY] = {0};
	unsigned char gen1[(NUM_DATA+NUM_PARITY)*(NUM_DATA+NUM_PARITY)];
	unsigned char recovery[NUM_DATA * NUM_DATA];
	unsigned char decode[NUM_DATA * NUM_DATA];
	unsigned char frdm[NUM_DATA * NUM_PARITY];
	char r_tables[(NUM_DATA + NUM_PARITY) * (NUM_DATA + NUM_PARITY) * 32];
	unsigned char *temp_buffs[NUM_PARITY] = {NULL};
	unsigned char *available_buffs[NUM_DATA + NUM_PARITY] = {NULL};
	int res = -100;
	int t=0;
	int mm=0;
	int marr[NUM_PARITY] = {0};
	int z=0;
	int h=0;
	int newline=0;
	int v=0;
	//int r=0;
	int prints=0;
	struct Node *temp = arr[uid % BUCKETSIZE];	

	for(i = 0 ; i < NUM_PARITY ; ++i)
	{
		marr[i] = -1 ;
	}	
    	
	//available buffs memoryallocation for data chunks
	for(i=0; i<NUM_DATA; i++)
	{
		if(i == NUM_DATA - 1)
		{
    			chunk_size = file_size-(chunk_size * (NUM_DATA - 1));	
    		}
		available_buffs[i]=(char *)malloc(sizeof(char)*chunk_size);
		memset(available_buffs[i], 0, chunk_size);
	}
	
	//available buffs memoryallocation for parity chunks
	for(i=0; i<NUM_PARITY; i++)
	{
		available_buffs[i + NUM_DATA]=(char *)malloc(sizeof(char)*file_size);
		memset(available_buffs[i + NUM_DATA], 0, file_size);
	}

	while(temp != NULL)
    	{
    		if(temp->uid == uid)
    		{
    			strcpy(file_path, temp->filepath);
			file_size = temp->size ;
    		}
    		temp = temp->next;
    	}

	for(i=0; file_path[i] != '\0'; i++)
	{
		if(file_path[i] == '/')
		{
			flag2=1;
		}	
	}
	
	printf("\n\t Filepath : %s", file_path);
	
	if(flag2 == 0)
	{
		strcpy(file_name, file_path);
		printf("%s", file_name);
	}
	else
	{
		str_reverse(file_path);
		for(j=0,k=0;file_path[j]!='/' ;j++,k++)
		{
			file_name[k]=file_path[j];
		}
		file_name[k+1] = '\0';
		str_reverse(file_name);
		str_reverse(file_path);
	}
	
	chunk_size = file_size / NUM_DATA ;
	buff=(char *)malloc(sizeof(char)*(chunk_size + NUM_DATA));

	strcpy(targetPath_old, targetFilePath);	

	printf("\n\t Target File path : %s", targetFilePath);
	do
	{
		//is a file
		
		if(isDir(targetFilePath)==0)
		{
			//1=file
			//0=dir
			
			flag=0;
			
			strcat(targetFilePath, file_name);
			sprintf(int_str,"%d",uid);

			/*if(check_if_file_exists(targetFilePath) == -1)
			{
				printf("\n\tFile already exists. Please enter a new name : ");
				scanf("%s", file_name);
				
				strcpy(targetFilePath, targetPath_old);
				strcat(targetFilePath, file_name);
				
				flag = 0;
			}*/
			
			//open the target file
			ft = fopen(targetFilePath,"a");

			if(ft == NULL)
			{
				printf("\n\tTarget file could not be opened!!");
				succ = -1;
			}
			else
			{
				fd = open(targetFilePath, O_RDWR | O_CREAT);
				for(i = 0 ; i < NUM_DATA ; ++i)
				{
					chunk=(char *)calloc(100,sizeof(char));
					memset(chunk, 0, 100);

					if(chunk == NULL)
					{
						printf("\n\tMemory not allocated for chunks");
						succ = -1;
					}
					else
					{
						sprintf(directory, DIRECTORY_PATH"folder%d", i);
						dirSucc = isDir(directory);
						
						if(dirSucc != 0)
						{
							check = mkdir(directory, 0777); 
						}
						if(check != 0)
						{
							printf("\n\t Folder creation failed!!!");
							succ = -1;
						}
						
						sprintf(chunk, DIRECTORY_PATH"folder%d/%d_chunk%d", i, uid, i);
						fchunk = fopen(chunk, "r") ;

						if(fchunk == NULL)
						{
							availableChunks[i] = 0 ;
							//printf("\n\tChunk file could not be opened");
							//succ = -1;	
						}
						else
						{
							availableChunks[i] = 1 ;
	
							fch = open(chunk, O_RDONLY | O_CREAT);

							if(i==(NUM_DATA - 1))
							{
					    			chunk_size=file_size-(chunk_size*(NUM_DATA - 1));
					    		}

							read(fch, buff, chunk_size);
							write(fd, buff, chunk_size);

							memcpy(available_buffs[i], buff, chunk_size);
						}
					}
				}
				
				for(i=0 ; i<NUM_PARITY ; ++i)
				{
					sprintf(chunk, DIRECTORY_PATH"parity%d/%d_parity%d", i, uid, i);
					fchunk = fopen(chunk, "r") ;

					if(fchunk == NULL)
					{
						availableChunks[i+NUM_DATA] = 0;
						//printf("\n\tParity file could not be opened");
						//succ = -1;	
					}
					else
					{
						availableChunks[i+NUM_DATA] = 1;	
				
						fch = open(chunk, O_RDONLY);
						read(fch, buff, file_size);

					memcpy(available_buffs[i+NUM_DATA], buff, file_size);						
					}
				}
			}
			
			// Regeneration Code
			for(i = 0, j = 0 ; i < (NUM_DATA + NUM_PARITY) && j < NUM_PARITY ; ++i)
			{
				if(availableChunks[i] == 0)
				{
					printf("\n\tChunk %d is missing.", i);
					missingChunks++;
					marr[j] = i ;
					j++;
				}
			}

			printf("\n\n\tAvailable Chunks : ");
			for(i = 0 ; i < (NUM_DATA + NUM_PARITY) ; ++i)
			{
				printf("%d  ", availableChunks[i]);
			}

			if(missingChunks > 0)
			{
				gf_gen_rs_matrix(gen, (NUM_DATA + NUM_PARITY), NUM_DATA);
				
				printf("\n\n\tGenerator Matrix : ");
				for(i = 0, j = 0 ; i < (NUM_DATA + NUM_PARITY) * NUM_DATA ; ++i)
				{
					if(i % NUM_DATA == 0)
					{
						printf("\n\t");
					}
					printf("%d \t", gen[i]);
				}

				for(i = 0, k = 0 ; i < (NUM_DATA + NUM_PARITY) ; ++i)
				{
					if(availableChunks[i] == 1)
					{
						for(j = 0 ; j < NUM_DATA ; ++j)
						{
							recovery[(k * NUM_DATA) + j] = gen[(i * NUM_DATA) + j];
						}
						k++;
					}
				}
				
				i = NUM_DATA;
				while(k < NUM_DATA)
				{
					for(j = 0 ; j < NUM_DATA ; ++j)
					{
						recovery[(k * NUM_DATA) + j] = gen[(i * NUM_DATA) + j];
					}
					k++;
					i++;
				}

				printf("\n\n\tRecovery Matrix : ");
				for(i = 0 ; i < (NUM_DATA) * NUM_DATA ; ++i)
				{
					if(i % NUM_DATA == 0)
					{
						printf("\n\t");
					}
					printf("%d \t", recovery[i]);
				}
				
				res = gf_invert_matrix(recovery, decode, NUM_DATA);

				printf("\n\n\tInvert matrix return value : %d", res);

				printf("\n\n\tDecode Matrix (Inverse of recovery matrix) : ");
				for(i = 0, j = 0 ; i < (NUM_DATA * NUM_DATA) && (j < NUM_DATA * NUM_DATA) ; ++i)
				{
					if(i % NUM_DATA == 0)
					{
						printf("\n\t");
					}
					printf("%d \t", decode[i]);
				}

				printf("\n\n\tFailed row decode matrix : \n\t");
				// frdm contains missing chunk's rows of inverted matrix 
				t = 0;
				mm = 0;
				for(mm = 0; mm < missingChunks; mm++)
				{
					//for(v = NUM_DATA *(marr[mm]) ; v < NUM_DATA *(marr[mm]+1) ; v++)
					for(v=0 ; v<NUM_DATA ; v++)  
					{
						if(marr[mm] != -1)
						{
							frdm[t] = decode[(marr[mm] * NUM_DATA) + v];
							printf("%d ", frdm[t]);
							t++;
						}
					}
					printf("\n\t");
				} 

				ec_init_tables(NUM_DATA, NUM_PARITY, frdm, r_tables);

				for(i = 0 ; i < (NUM_PARITY) ; ++i)
				{
					temp_buffs[i] = (char *)malloc(sizeof(char)*file_size);
					memset(temp_buffs[i], 0, file_size);
				}

				ec_encode_data_base(file_size, NUM_DATA, missingChunks + 1, r_tables, available_buffs, temp_buffs);

				printf("\n\tRegenerated missing chunks : ");
				for(i = 0, j = 0; i < NUM_DATA ; i++)
				{
					if(availableChunks[i] != 1)
					{
					        printf("  %d", i);
					        marr[j] = i;
					        j++;
					}
				}
				j--;

				for(i=0, j=0 ; i<NUM_DATA ; ++i)
				{
					if(availableChunks[i] == 0)
					{
						sprintf(chunk, DIRECTORY_PATH"folder%d/%d_chunk%d", i, uid, i);			
						fchunk=fopen(chunk,"w");
				    		
						if(fchunk == NULL)
						{
							printf("\n\tMissing data file %d could not be created!", i);
						}
						else
						{
							fch = open(chunk, O_RDWR | O_CREAT);
							printf("\n\t Temp_buff[%d] : %s", j, temp_buffs[j]);
							write(fch, temp_buffs[j], sizeof(temp_buffs[j]));
							j++;
						}
						
					}
				}

				for(i=0 ; i<NUM_PARITY ; ++i)
				{
					if(availableChunks[i + NUM_DATA] == 0)
					{
						sprintf(chunk, DIRECTORY_PATH"parity%d/%d_parity%d", i, uid, i);
						fchunk=fopen(chunk,"w");
				    		
						if(fchunk == NULL)
						{
							printf("\n\tMissing parity file %d could not be created!", i);
						}
						else
						{
							fch = open(chunk, O_RDWR | O_CREAT);
							printf("\n\t Temp_buff[%d] : %s", j, temp_buffs[j]);
							write(fch, temp_buffs[j], sizeof(temp_buffs[j]));
							j++;
						}
					}
				}
				
				/*printf("\n\tTemp buff : ");
				for(i=0 ; i<(NUM_PARITY) ; ++i)
				{
					printf("\n\t**** %s", temp_buffs[i]);
				}*/

				//flag=-1;			//to run loop again to get chunks after regeneration
				strcpy(targetFilePath, targetPath_old);	
				succ = get(uid,targetPath_old);
			}
				
		}	
		//directory filepath
		else
		{
			printf("\n\tThe given targetFilePath is incorrect");
			printf("\n\tPlease enter correct directory path : ");
			scanf("%s",targetFilePath);
			flag=1;
		}
	}while(flag != 0);

     	return succ;//success code
}

int isDir(const char *filePath){
	DIR *directory=opendir(filePath);
	if(directory!=NULL)
	{
		closedir(directory);
		return 0;//is a directory
	}
	if(errno=ENOTDIR){
		return 1;
	}
	return -1;
}

//Put Function
int put(struct Node **arr,char file_path[100])
{
	FILE *fp = NULL;
	FILE *fchunk = NULL;
    	int succ=0;
    	long int file_size = 0;
    	long int chunk_size = 0;
    	char file_name[50]={'\0'};	    
	char chunk_path[100] = {'\0'};
    	char file_chunk[100]={'\0'};
    	char int_str[20]={'\0'};
    	int i=0;
    	char ch='\0';
    	int k=0;
    	int j=0;
    	int uid=0;
    	int rs=0;
	int fd=0;
	int fch=0;
	char directory[100] = {'\0'};
	int dirSucc = 0;
	int check=0;

    	uid=generateUid();
    	
    	//Generating Generator Matrix
	gf_gen_rs_matrix(gen, (NUM_DATA + NUM_PARITY), NUM_DATA);
	//now gen structure contains generator matrix(Identity matrix + Parity matrix) required to compute parities for 8:3 EC
	
	/*printf("\n\t g_tbls, before init : \n\t");
	for(i = 0 ; i < (NUM_DATA + NUM_PARITY) * (NUM_DATA + NUM_PARITY) ; ++i)
	{
		if(i % (NUM_DATA + NUM_PARITY) == 0)
		{
			printf("\n\t ");
		}
		printf("  %d", g_tbls[i]);
	}*/

	//Initializing g_tables structure
	ec_init_tables(NUM_DATA, NUM_PARITY, &gen[NUM_DATA * NUM_DATA], g_tbls);
	
	/*printf("\n\t g_tbls, after init : \n\t");
	for(i = 0 ; i < (NUM_DATA + NUM_PARITY) * (NUM_DATA + NUM_PARITY) ; ++i)
	{
		if(i % (NUM_DATA + NUM_PARITY) == 0)
		{
			printf("\n\t ");
		}
		printf("  %d", g_tbls[i]);
	}*/

    		
	fp = fopen(file_path, "r");

	if(fp == NULL)
	{
		printf("\n\tFile could not be opened!");
	}
	else
	{
		fd = open(file_path, O_RDONLY | O_CREAT);
		
		fseek(fp, 0L, SEEK_END);
	    	file_size = ftell(fp);
	    	chunk_size = file_size / NUM_DATA;
	    
	    	//for parity
		for(i=0; i<NUM_PARITY; i++)
		{
			paritybuffs[i] = (char *)malloc(sizeof(char)*file_size);
			memset(paritybuffs[i], 0, file_size);
		}
		
		//for data
		for(i=0; i<NUM_DATA; i++)
		{
			if(i == NUM_DATA - 1){
	    			chunk_size = file_size-(chunk_size * (NUM_DATA - 1));	
	    		}
			datachunks[i]=(char *)malloc(sizeof(char)*chunk_size);
			memset(datachunks[i], 0, chunk_size);
		}
	
	    	databuffs=(char *)malloc(sizeof(char)*(file_size/NUM_DATA));//to store data chunks
	    
		succ=putHash(arr,uid,file_path,file_size);

		if(succ == 1)
		{
			printf("\n\tEntry not added to HashTable");
			return -1;
		}
		else
		{
			printf("\n\tEntry added to HashTable successfully!!");	
		}	    	

		chunk_size = file_size / NUM_DATA;
	    
	    	printf("\n\tChunk size is = %ld bytes\n", chunk_size);
	    	
	    	for(i=0; i<NUM_DATA; i++)
		{
	    		if(i==(NUM_DATA - 1))
			{
	    			chunk_size=file_size-(chunk_size*(NUM_DATA - 1));
	    			databuffs=(char *)realloc(databuffs,chunk_size);
	    		}
			
			sprintf(directory, DIRECTORY_PATH"folder%d", i);
			dirSucc = isDir(directory);
			if(dirSucc != 0)
			{
				check = mkdir(directory, 0777); 
			}
			
			if(check != 0)
			{
				printf("\n\t Folder creation failed!!!");
				succ = -1;
			}			
	    		sprintf(file_chunk, DIRECTORY_PATH"folder%d/%d_chunk%d", i, uid, i); // /home/Obj/folder0/1_chunk0
	    		fchunk=fopen(file_chunk,"w");

			if(fchunk == NULL)
			{
				printf("\n\tChunk file could not be created!");
			}
			else
			{
				//fread(databuffs,chunk_size,1,fp);
		    		//fwrite(databuffs,chunk_size,1,fchunk);

				fch = open(file_chunk, O_WRONLY | O_CREAT);

				read(fd, databuffs, chunk_size);
				write(fch, databuffs, chunk_size);

		    		memcpy(datachunks[i], databuffs, chunk_size);
			}
	    	}

		printf("\n\tParity Chunk before %d : %s", i, paritybuffs[i]);

		ec_encode_data_base(file_size, NUM_DATA, NUM_PARITY, g_tbls, datachunks, paritybuffs);

		for(i=0; i<NUM_PARITY; i++)
		{
	    		sprintf(file_chunk, DIRECTORY_PATH"parity%d/%d_parity%d", i, uid, i);
			fchunk=fopen(file_chunk,"w");
	    		
			if(fchunk == NULL)
			{
				printf("\n\tParity file could not be created!");
			}
			else
			{
				fch = open(file_chunk, O_RDWR | O_CREAT);

				//fwrite(paritybuffs[i],sizeof(paritybuffs[i]),1,fchunk);
				write(fch, paritybuffs[i], sizeof(paritybuffs[i]));

		    		printf("\n\tParity Chunk %d : %s", i, paritybuffs[i]);
			}
	    	}

	    	printf("\n\tParity Chunks Generated");
	}
	
    	return uid;
    	
}

int putHash(struct Node **arr,int uid,char filepath[100],long int file_size){

	int bucket_no;
	int i,hash;
	struct Node *newnode;

	bucket_no = uid % BUCKETSIZE;

	if(arr[bucket_no]==NULL)
	{
		newnode = (struct Node*)malloc(sizeof(struct Node));

		if(newnode==NULL)
		{
			printf("\n\tMemory allocation for newnode failed");
			return 1;
		}	
		else
		{
			newnode->filepath=(char*)calloc(strlen(filepath),sizeof(char));
			
			strcpy(newnode->filepath,filepath);
			printf("\n\tFile Path is %s ",newnode->filepath);

			//newnode->fp=fopen(filepath,"r");
			newnode->uid=uid;
			newnode->size=file_size;
			newnode->next=NULL;
		}

		arr[bucket_no]=newnode;
	}
	else
	{
		struct Node *temp=NULL;
		temp=(arr[bucket_no]);
		
		while(temp->next!=NULL)
		{
			temp=temp->next;
		}

		newnode= (struct Node*)malloc(sizeof(struct Node));

		if(newnode==NULL)
		{
			printf("\n\tMemory allocation for newnode failed");
			return 1;
		}	
		else
		{
			newnode->filepath=(char*)calloc(strlen(filepath),sizeof(char));

			for(i=0;filepath[i]!='\0';i++)
			{
				newnode->filepath[i]=filepath[i];
			}

			newnode->uid=uid;
			newnode->size=file_size;
			newnode->next=NULL;
			temp->next=newnode;
		}
	}
	
	display(arr[bucket_no]);

	return 0;
}


void display(struct Node *temp)
{
	printf("\n\tHash Table records : ");

	while(temp!=NULL)
	{
		printf("\n\tuid = %d ",temp->uid);
		printf("\n\tFilepath = %s ",temp->filepath);
		printf("\n\tFile Size = %ld bytes\n",temp->size);

		temp=temp->next;
	}
}


void list(struct Node **arr)
{
	int i=0;
	struct Node *temp=NULL;

	for(i=0; i<BUCKETSIZE; i++)
	{
		printf("\n\tBucket Number : %d",i);

		if(arr[i]==NULL)
		{
			printf("\n\tBucket Empty\n");
		}
		else
		{
			temp=arr[i];
			display(temp);
		}
	}
}

void str_reverse(char fstr[50])
{
    int i=0 ;
    int j=0 ;
    int count=0 ;
    char fstr2[50];
    
    count = strlen(fstr) ;
    for(i=count-1, j=0 ; i>=0 ; i--, j++)
    {
        fstr2[j] = fstr[i] ;
        fstr2[j+1] = '\0' ;
    } 
    
    strcpy(fstr, fstr2);
} 

int check_if_file_exists(char * newpath)
{
	newpath[strcspn(newpath, "\n")] = 0;

	if(access(newpath, F_OK) != -1)
	{
		//File already exists
		return -1;
	}
	else
	{
		return 1;
	}
}
