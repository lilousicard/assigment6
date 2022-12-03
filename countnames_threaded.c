#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <ctype.h>

/*****************************************
//CS149 Fall 2022
//Template for assignment 6
//San Jose State University
//originally prepared by Bill Andreopoulos
*****************************************/


//thread mutex lock for access to the log index
//TODO you need to use this mutexlock for mutual exclusion
//when you print log messages from each thread
pthread_mutex_t tlock1 = PTHREAD_MUTEX_INITIALIZER;


//thread mutex lock for critical sections of allocating THREADDATA
//TODO you need to use this mutexlock for mutual exclusion
pthread_mutex_t tlock2 = PTHREAD_MUTEX_INITIALIZER; 


//thread mutex lock for access to the name counts data structure
//TODO you need to use this mutexlock for mutual exclusion
pthread_mutex_t tlock3 = PTHREAD_MUTEX_INITIALIZER; 


void* thread_runner(void*);
pthread_t tid1, tid2;

//struct points to the thread that created the object. 
//This is useful for you to know which is thread1. Later thread1 will also deallocate.
struct THREADDATA_STRUCT
{
  pthread_t creator;
};
typedef struct THREADDATA_STRUCT THREADDATA;

THREADDATA* p=NULL;


//variable for indexing of messages by the logging function.
int logindex=0;
int *logip = &logindex;


//The name counts.
//From 11/12/2022 1628, A6 Channel "Assume you are dealing with at most 100 distinct names of length at most 30 characters"
struct NAME_STRUCT
{
  char name[30];
  int count;
};
typedef struct NAME_STRUCT THREAD_NAME;


//node with name_info for a linked list
struct NAME_NODE
{
  THREAD_NAME name_count;
  struct NAME_NODE *next;
};

#define HASHSIZE 26
static struct NAME_NODE *hashtab[HASHSIZE]; /* pointer table */

int hash(char first) {
  //printf("in hash debug %c maps to %d\n",first,first%26);
  return first%26;
}

struct NAME_NODE *lookup(char* name) {
  struct NAME_NODE *np;
  for (np = hashtab[hash(name[0])]; np != NULL; np = np->next)
      if (strcmp(name, np->name_count.name) == 0)
          return np; /* found */
  return NULL; /* not found */
}

struct NAME_NODE *insert(char* name){
  struct NAME_NODE *np;
  //Name is not already in the list
  if((np=lookup(name)) == NULL){
    np = (struct NAME_NODE *) malloc(sizeof(*np));
    if (np == NULL)
      return NULL;
    strcpy(np->name_count.name,name);
    np->name_count.count = 1;
    int hashval = hash(name[0]);
   
    np->next = hashtab[hashval];
    hashtab[hashval] = np;
  } else {
    //Name is in the list
    np->name_count.count = np->name_count.count + 1;
  }
  return np;
}

void freeHash(){
  struct NAME_NODE *np;
  for(int i = 0; i < HASHSIZE; i++){
    while(hashtab[i]!=NULL){
      np = hashtab[i];
      hashtab[i]=np->next;
      free(np);
    }
  }
}

void getTime(char* times){
  time_t now;
  time(&now);
  int hours, minutes, seconds, day, month, year;
  struct tm *local = localtime(&now);

  hours = local->tm_hour;       // get hours since midnight (0-23)
  minutes = local->tm_min;      // get minutes passed after the hour (0-59)
  seconds = local->tm_sec;      // get seconds passed after minute (0-59)

  day = local->tm_mday;         // get day of month (1 to 31)
  month = local->tm_mon + 1;    // get month of year (0 to 11)
  year = local->tm_year + 1900; // get year since 1900

  // print local time
  if (hours < 12) // before midday
    sprintf(times,"%02d/%02d/%d %02d:%02d:%02d am", day, month, year, hours, minutes, seconds);
  else  // after midday
    sprintf(times,"%02d/%02d/%d %02d:%02d:%02d pm", day, month, year, hours - 12, minutes, seconds);
}

/*********************************************************
// function main 
*********************************************************/
int main(int argc, char *argv[])
{
  if (argc != 3){
    printf("FATAL ERROR: The number of file you entered is incorrect\n");
    return 0;
  } 

  printf("create first thread\n");
  pthread_create(&tid1,NULL,thread_runner,argv[1]);
  
  printf("create second thread\n");
  pthread_create(&tid2,NULL,thread_runner,argv[2]);
  
  printf("wait for first thread to exit\n");
  pthread_join(tid1,NULL);
  printf("first thread exited\n");

  printf("wait for second thread to exit\n");
  pthread_join(tid2,NULL);
  printf("second thread exited\n");

  printf("/*********************************************************/\n");

  struct NAME_NODE *np;
  for(int i = 0; i < HASHSIZE; i++){
    np = hashtab[i];
    while(np!=NULL){
      printf("%s: %d\n",np->name_count.name,np->name_count.count);
      np = np->next;
    }
  }

  freeHash();

  exit(0);

}//end main


/**********************************************************************
// function thread_runner runs inside each thread 
**********************************************************************/
void* thread_runner(void* x)
{
  pthread_t me;
  FILE *fp;
  char* fileName = (char*) x;
  me = pthread_self();
  char* time = (char*)malloc(30);

  getTime(time);
  pthread_mutex_lock(&tlock1);
  logindex++;
  printf("Logindex %d, thread %ld, PID %d, %s: This is thread %ld (p=%p)\n",logindex,me,getpid(), time , me,p);
  pthread_mutex_unlock(&tlock1);


  pthread_mutex_lock(&tlock2); // critical section starts
  if (p==NULL) {
    p = (THREADDATA*) malloc(sizeof(THREADDATA));
    p->creator=me;
  }
  pthread_mutex_unlock(&tlock2);  // critical section ends

  pthread_mutex_lock(&tlock1);
  getTime(time);
  if (p!=NULL && p->creator==me) {
    logindex++;
    printf("Logindex %d, thread %ld, PID %d, %s: This is thread %ld and I created THREADDATA %p\n",logindex,me,getpid(), time,me,p);
  } else {
    logindex++;
    printf("Logindex %d, thread %ld, PID %d, %s: This is thread %ld and I can access the THREADDATA %p\n",logindex,me,getpid(), time,me,p);
  }
  pthread_mutex_unlock(&tlock1);


  // opening file for reading
  fp = fopen(fileName, "r");
  if(fp == NULL){
    fprintf(stderr, "range: cannot open file %s\n",fileName);
  } else {
    getTime(time);
    pthread_mutex_lock(&tlock1);
    logindex++;
    printf("Logindex %d, thread %ld, PID %d, %s: opened file %s\n",logindex,me,getpid(), time , fileName);
    pthread_mutex_unlock(&tlock1);

    //While loop to read file
    char str[30];
    int lineIndex = 0;
    while(fgets (str, 30, fp) != NULL ) {
      lineIndex++;
      if(str[0]=='\n'||str[0]==0){
        fprintf(stderr,"Warning - file %s line %d is empty.\n",fileName,lineIndex);
        continue;
      }
      if(str[0]==' '){
        continue;
      }

      //remove new line
      str[strlen(str)-1] = 0;
      struct NAME_NODE *np = lookup(str);
      pthread_mutex_lock(&tlock3);
      if(np==NULL){
        insert(str);
      } else {
        np->name_count.count = np->name_count.count + 1;
      }
      
      pthread_mutex_unlock(&tlock3);  
    }

  }

  pthread_mutex_lock(&tlock1);
  logindex++;
  pthread_mutex_unlock(&tlock1);  


  pthread_mutex_lock(&tlock2);
  // TODO use mutex to make this a start of a critical section 
  getTime(time);
  if (p!=NULL && p->creator==me) {
    printf("Logindex %d, thread %ld, PID %d, %s: This is thread %ld and I delete THREADDATA\n",logindex,me,getpid(), time ,me);
    free(p);
    p=NULL;

  } else {
    printf("Logindex %d, thread %ld, PID %d, %s: This is thread %ld and I can access (but not delete) the THREADDATA\n",logindex,me,getpid(), time ,me);
  }
  // TODO critical section ends
  pthread_mutex_unlock(&tlock2);  

  pthread_exit(NULL);

  free(time);

  return NULL;

}//end thread_runner

