#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <semaphore.h>
#include <time.h>
#include <pthread.h>
#include <ctype.h>
#include <stdint.h>
#define MAX_JOURNS 15
#define MAX_USERS 1000
sem_t s_w_sem; //semaphore shared by users for writing summary
sem_t r_w_sem; //semaphoe shared by users & journalists
sem_t r_sem;   //semaphore shared by users for reading tweets database
int n;   //no of journalists
int r_count;  //readers count
char * timestamp;
int k = 0;  //count of number of tags per user

typedef struct structtags {
        char* val;
        struct structtags* next;
} tags;

tags* tag_list[MAX_USERS];

typedef struct string
{
	char val[100]; 
} string;

string hashtag;
string user[MAX_USERS];

char * to_lower(char *);
char * concat(const char *, const char *);
void create_journalists_db(void);
void * processJournalists(void *);
void * processUsers(void *);

tags* add_tag(tags* tagList, char* tag) {
        tags* newtags = malloc(sizeof(tags));
        if (NULL != newtags){
                newtags->val = to_lower(tag);
                newtags->next = NULL;
        }
        if (NULL != newtags) {
                newtags->next = tagList;
        }
        return newtags;
}

int main(void)
{
	
	int no_users = 1;
	//threads initialization
	pthread_t journalist_thread[MAX_JOURNS];  //crawler threads
	pthread_t user_thread[MAX_USERS];         //summarizer threads

	//semaphore initialization
	sem_init(&s_w_sem, 0, 1);
	sem_init(&r_w_sem, 0, 1);
	sem_init(&r_sem, 0, 1);

	remove("Output.txt");
	remove("data.txt");

	printf("Enter no of journalists(MAX 15): ");
	scanf("%d",&n);	
	create_journalists_db();

	for(int j=1; j<=n; j++){
		pthread_create(&journalist_thread[j], NULL, processJournalists, (void *)(intptr_t)j);
	}

	while(no_users <= MAX_USERS){
		//users
		k = 0;	
		printf("Enter Username: ");
		scanf("%s",user[no_users].val);
		printf("Enter Hashtags : \n");
		scanf("%s",hashtag.val);

		while(strcmp(hashtag.val,"STOP") != 0){
			k++;
			tag_list[no_users] = add_tag(tag_list[no_users], hashtag.val);
			scanf("%s",hashtag.val);
		}
		//summary timestamp
		timestamp = concat(user[no_users].val,"_______________________________________________Current timestamp: ");
		time_t t = time(NULL);
    	struct tm * tm = localtime(&t);
    	timestamp = concat(timestamp, asctime(tm));

    	pthread_create(&user_thread[no_users], NULL, processUsers, (void *)(intptr_t)no_users);
	
    	no_users++;
	}

	for(int j=1; j<=n; j++){
		pthread_join(journalist_thread[j], NULL);
	}

	for(int j=1; j<no_users; j++){
		pthread_join(user_thread[j], NULL);
	}

	//destroy semaphores 
	sem_destroy(&s_w_sem);
	sem_destroy(&r_w_sem);
	sem_destroy(&r_sem);

	return 0;	
}

void * processJournalists(void *arg){
	   //journalist threaf
			char ip[5], *ipfile, *line = NULL;
			size_t read;
			ssize_t len = 0;
			snprintf(ip,5,"%d", (int)(intptr_t)arg);
			ipfile = concat(ip,".txt");
			FILE * t_fp;
			FILE * j_fp = fopen(ipfile,"r");
			while(!feof(j_fp)){
				if(read =(getline(&line, &len, j_fp)) != -1)
				{
					sleep((int)(intptr_t)arg);
					sem_wait(&r_w_sem);
					t_fp = fopen("data.txt","a");   //shared database
					fprintf(t_fp,"%s", line);
					fclose(t_fp);
					sem_post(&r_w_sem);
				}
			}
			fclose(j_fp);
}

void * processUsers(void * arg){
		     
	//user process
	int a = (int)(intptr_t)arg;
	char * inputfile = "data.txt";
	FILE * input_fp, * output_fp;
	char * line = NULL, * temp = NULL, * current_tag = NULL, * current_sentense = NULL, * token = NULL;
	size_t len = 0;
	ssize_t read;
	int i = 0, j = 0, valid = 0, tagslen = k;
		
	//start iterating for every hashtag to generated summary for it
    tags* iter = NULL;		
    for (iter = tag_list[a]; NULL != iter; iter = iter->next){
		current_tag = iter->val;
		
		//readers-writers synchronization
		sem_wait(&r_sem);
		r_count++;   //readers' count	
		if(r_count == 1)
    		sem_wait(&r_w_sem);

		sem_post(&r_sem);
		input_fp = fopen(inputfile, "r");
		char *temp_out_file = concat(user[a].val, current_tag);
		output_fp = fopen(temp_out_file, "w+");

		//start reading the input file for given tag
		if (input_fp != NULL){			
			while ((read = getline(&line, &len, input_fp)) != -1){
				valid = 0;
				current_sentense = NULL;

				// some checks to remove junk from the read line
				if (line[strlen(line) - 1] == '\n'){
					line[strlen(line) - 1] = '\0';
				}
						
				//split the line into words
				token = strtok(line, " ");
				while (token != NULL){		
					// check if the line contains the current hashtag, if yes, mark the line for output
					temp = to_lower(token);
					if (strcmp(temp, current_tag) == 0){
						valid = 1;
					}
					if (temp)
						free(temp);
					if (token[0] != '#'){// no hashtag should go to the summary
						//append word to the sentense
						current_sentense = concat(current_sentense, token);
						current_sentense = concat(current_sentense," ");
					}
					token = strtok (NULL, " ");
				}
				
				// write the line to intermediate file
				if (valid == 1){
					//write this sentense to the output file
					current_sentense[strlen(current_sentense) - 1] = '\0';
					fprintf(output_fp,"%s\n", current_sentense);
				}
					
				if (current_sentense)
					free(current_sentense);
			}
			fclose(input_fp);
			fclose(output_fp);
				
			//readers-writers synchronization		
			sem_wait(&r_sem);
			r_count--;

			if(r_count == 0)
				sem_post(&r_w_sem);

			sem_post(&r_sem);								
			if (temp_out_file) 
				free(temp_out_file);
		}
	}
		
	// Code for executing the summarization script
	//Takes all the intermediate files and generates a single summary file
			
	char * summary_file_name = concat("summary_", user[a].val);
	summary_file_name = concat(summary_file_name, ".txt");		
	output_fp = fopen(summary_file_name, "w");
		
	iter = NULL;
		
    for (iter = tag_list[a]; NULL != iter; iter = iter->next){
		size_t read;
		char * line = NULL;
		ssize_t len = 0;

		// get file name for files created from previous step 
		char * inputfile = concat(user[a].val, iter->val);
		
		//prepare the command to trigger the summarization code
		char * command = "python getsummary.py ";
		command = concat(command, inputfile);
			
		//execute the command
		system(command);
			
		// output the current tag
		fprintf(output_fp,"\n%s\n", iter->val);

		// write everything from the intermediate files to summary file
		char * temp_outputfile = concat(inputfile, "_out");
		FILE * temp_fp = fopen(temp_outputfile, "r");

		while ((read = getline(&line, &len, temp_fp)) != -1){
			fprintf(output_fp,"%s", line);
		}			
		fclose(temp_fp);
		if (temp_outputfile)
			free(temp_outputfile);
		if (command)
			free(command);
		if (inputfile)
			free(inputfile);
	}
	fclose(output_fp);
			
	// to write final output file containg all individual user-outputs
	FILE * final_ip_fp = fopen(summary_file_name, "r");

	//writers synchronization
	sem_wait(&s_w_sem);
	FILE * final_op_fp = fopen("Output.txt", "a");			
	fprintf(final_op_fp,"%s", "\n");
	fprintf(final_op_fp,"%s", timestamp);

	while ((read = getline(&line, &len, final_ip_fp)) != -1){	
		fprintf(final_op_fp,"%s", line);
	}

	fclose(final_op_fp);
	sem_post(&s_w_sem);
	fclose(final_ip_fp);

	if (line)
		free(line);
	if (summary_file_name)
		free(summary_file_name);
	
}

//function to convert the input string to lower case
char * to_lower(char * input) {
	int i = 0;
	const size_t len = input ? strlen(input): 0;
	char * output = (char *) malloc(len + 1);
	for(i = 0; i< len; i++){
			output[i] = tolower(input[i]);
	}
	output[len] = '\0';
	return output;
}

//function to concatenate 2 input strings and return result
char* concat(const char *s1, const char *s2)
{
    const size_t len1 = s1 ? strlen(s1): 0;
    const size_t len2 = s2 ? strlen(s2): 0;
    char *result = (char *) malloc(len1+len2+1);
    memcpy(result, s1, len1);
    memcpy(result+len1, s2, len2+1);
    return result;
}

//function to create no of journalists
void create_journalists_db(void){
	int l, i = 1, count = 0, start, end;
	size_t read;
	ssize_t len = 0;
	FILE * op_fp;
	char c, opfile[5], *line = NULL, *op;

	FILE * in_fp = fopen("tweetdata.txt","r");
	for(c = getc(in_fp); c != EOF; c =getc(in_fp))
		if(c == '\n')
			count++;	
	fclose(in_fp);

	l =  count / n;
	start = 0;
	end = l;
	in_fp = fopen("tweetdata.txt","r");

	while(i <= n){
		snprintf(opfile,5,"%d", i);
		op = concat(opfile,".txt");
		op_fp = fopen(op,"w");
		for( ; start<end ; start++){
			if(read=(getline(&line, &len, in_fp)) != -1);
			{
				fprintf(op_fp,"%s", line);
			}
		
		}
		i++;
		if(i == n)
			end = count;
		else 
			end = start+l;

		fclose(op_fp);
	}
	fclose(in_fp);
}
