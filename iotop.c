/**
 * @file   iotop.c
 * @author balaji <balaji@beethoven>
 * @date   Thu Mar  7 00:29:03 2013
 * 
 * @brief  my implementation of iotop over iopp  which 
 * writes the data in io percentage to a shared memeory 
 * for GAS This code is was initially taken from 
 * git://github.com/markwkm/iopp.git
 */

/*
 * Copyright (C) 2008 Mark Wong
 */

#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <math.h>
#include <unistd.h>

#include "list.h"
#include "red_black_tree.h"


static double total_io;
static pid_t my_pid;

#define MAX_NR_PROCESSES_HIST 50000


/** 
 * Checks if a process is alive or dead
 * 
 * @param pid
 * 
 * @return   1 means the process is alive
 *           0 means the process is dead
 */	     
int check_if_alive(int pid)
{
  char command[128];
  sprintf(command, "sudo ls /proc/%d 1>/dev/zero 2> /dev/null", pid);
  if(0==system(command))
    return 1;
  else
    return 0;
}


#define PROC "/proc"

#define GET_VALUE(v)							\
  p = strchr(p, ':');							\
  ++p;									\
  ++p;									\
  q = strchr(p, '\n');							\
  length = q - p;							\
  if (length >= BUFFERLEN)						\
    {									\
      printf("ERROR - value is larger than the buffer: %d\n", __LINE__); \
      exit(1);								\
    }									\
  strncpy(value, p, length);						\
  value[length] = '\0';							\
  v = atoll(value);

#define BTOKB(b) b >> 10
#define BTOMB(b) b >> 20

#define BUFFERLEN 255
#define COMMANDLEN 1024
#define VALUELEN 63

#define NUM_STRINGS 8

struct io_node
{
  int pid;
  long long rchar;
  long long wchar;
  long long syscr;
  long long syscw;
  long long read_bytes;
  long long write_bytes;
  long long cancelled_write_bytes;
  long long read_write_bytes;
  char command[COMMANDLEN + 1];
  struct list_head list;
  //  struct io_node *next;
};

struct io_node *head = NULL;
int command_flag = 0;
int idle_flag = 0;
int mb_flag = 0;
int kb_flag = 0;
int hr_flag = 0;

/* Prototypes */
char *format_b(long long);
struct io_node *get_ion(int);
struct io_node *new_ion(char *);
static void upsert_data(struct io_node *);

char *
format_b(long long amt)
{
  static char retarray[NUM_STRINGS][16];
  static int	index = 0;
  register char *ret;
  register char tag = 'B';

  ret = retarray[index];
  index = (index + 1) % NUM_STRINGS;

  if (amt >= 10000) {
    amt = (amt + 512) / 1024;
    tag = 'K';
    if (amt >= 10000) {
      amt = (amt + 512) / 1024;
      tag = 'B';
      if (amt >= 10000) {
	amt = (amt + 512) / 1024;
	tag = 'G';
      }
    }
  }

  snprintf(ret, sizeof(retarray[index]) - 1, "%lld%c", amt, tag);

  return (ret);
}

int
get_cmdline(struct io_node *ion)
{
  int fd;
  int length;
  char filename[BUFFERLEN + 1];
  char buffer[COMMANDLEN + 1];
  char *p;
  char *q;


  length = snprintf(filename, BUFFERLEN, "%s/%d/cmdline", PROC, ion->pid);
  if (length == BUFFERLEN)
    printf("WARNING - filename length may be too big for buffer: %d\n",
	   __LINE__);
  fd = open(filename, O_RDONLY);
  if (fd == -1)
    return 1;
  length = read(fd, buffer, sizeof(buffer) - 1);
  close(fd);
  buffer[length] = '\0';
  if (length == 0)
    return 2;
  if (command_flag == 0)
    {
      /*
       * The command is near the beginning; we don't need to be able to
       * the entire stat file.
       */
      p = strchr(buffer, '(');
      ++p;
      q = strchr(p, ')');
      length = q - p;
    }
  else
    p = buffer;
  length = length < COMMANDLEN ? length : COMMANDLEN;
  strncpy(ion->command, p, length);
  ion->command[length] = '\0';
  return 0;
}


struct io_node* get_ion(int pid)
{
  static struct io_node *node;
  static struct list_head *iter;
  node = NULL;

  if(head==NULL)
    return head;
  list_for_each(iter, &head->list)
    {
      node = container_of(iter, struct io_node, list);
      if(node->pid == pid)
	return node;
    }
  return head;
}

int
get_tcomm(struct io_node *ion)
{
  int fd;
  int length;
  char filename[BUFFERLEN + 1];
  char buffer[BUFFERLEN + 1];
  char *p;
  char *q;

  length = snprintf(filename, BUFFERLEN, "%s/%d/stat", PROC, ion->pid);
  if (length == BUFFERLEN)
    printf("WARNING - filename length may be too big for buffer: %d\n",
	   __LINE__);
  fd = open(filename, O_RDONLY);
  if (fd == -1)
    return 1;
  length = read(fd, buffer, sizeof(buffer) - 1);
  close(fd);
  /*
   * The command is near the beginning; we don't need to be able to
   * the entire stat file.
   */
  p = strchr(buffer, '(');
  ++p;
  q = strchr(p, ')');
  length = q - p;
  length = length < BUFFERLEN ? length : BUFFERLEN;
  strncpy(ion->command, p, length);
  ion->command[length] = '\0';
  return 0;
}

static struct io_node * insert_ion(struct io_node *ion)
{
  list_add_tail(&ion->list,&head->list);
  return head;
}

static void get_stats_iotop()
{
  DIR *dir = opendir(PROC);
  struct dirent *ent;
  char filename[BUFFERLEN + 1];
  char buffer[BUFFERLEN + 1];

  char value[BUFFERLEN + 1];

  total_io =0;
  /* Loop through the process table and display a line per pid. */
  while ((ent = readdir(dir)) != NULL)
    {
      int rc;
      int fd;
      int length;

      char *p;
      char *q;

      struct io_node *ion;
      struct io_node *old_ion;

      static long long rchar;
      static long long wchar;
      static long long syscr;
      static long long syscw;
      static long long read_bytes;
      static long long write_bytes;
      static long long cancelled_write_bytes;

      if (!isdigit(ent->d_name[0]))
	continue;

      /// Skip iotop process itself :P
       if(my_pid == atoi(ent->d_name))
       	continue;

      ion = new_ion(ent->d_name);

      if (command_flag == 1)
	rc = get_cmdline(ion);
      if (command_flag == 0 || rc != 0)
	/* If the full command line is not asked for or is empty... */
	rc = get_tcomm(ion);

      if (rc != 0)
	{
	  free(ion);
	  continue;
	}

      /* Read 'io' file. */
      length = snprintf(filename, BUFFERLEN, "%s/%s/io", PROC, ent->d_name);
      if (length == BUFFERLEN)
	printf("WARNING - filename length may be too big for buffer: %d\n",
	       __LINE__);
      fd = open(filename, O_RDONLY);
      if (fd == -1)
	{
	  free(ion);
	  continue;
	}
      length = read(fd, buffer, sizeof(buffer) - 1);
      close(fd);
      buffer[length] = '\0';

      /* Parsing the io file data. */
      p = buffer;
      GET_VALUE(ion->rchar);
      GET_VALUE(ion->wchar);
      GET_VALUE(ion->syscr);
      GET_VALUE(ion->syscw);
      GET_VALUE(ion->read_bytes);
      GET_VALUE(ion->write_bytes);
      GET_VALUE(ion->cancelled_write_bytes);

      old_ion = get_ion(ion->pid);

      /* Display the pid's io data. */
      if (old_ion != NULL)
	{
	  rchar = llabs(ion->rchar - old_ion->rchar);
	  wchar = llabs(ion->wchar - old_ion->wchar);
	  syscr = llabs(ion->syscr - old_ion->syscr);
	  syscw = llabs(ion->syscw - old_ion->syscw);
	  read_bytes = llabs(ion->read_bytes - old_ion->read_bytes);
	  write_bytes = llabs(ion->write_bytes - old_ion->write_bytes);
	  cancelled_write_bytes = llabs(ion->cancelled_write_bytes -
					old_ion->cancelled_write_bytes);

	  rchar = BTOKB(rchar);
	  wchar = BTOKB(wchar);
	  syscr = BTOKB(syscr);
	  syscw = BTOKB(syscw);
	  read_bytes = BTOKB(read_bytes);
	  write_bytes = BTOKB(write_bytes);
	  cancelled_write_bytes = BTOKB(cancelled_write_bytes);
	  ion->read_write_bytes = read_bytes + write_bytes + rchar + wchar ;
	  if(ion->read_write_bytes!=0)
	    ion->read_write_bytes = (ion->read_write_bytes);
	  //	  old_ion->read_write_bytes = read_bytes + write_bytes + rchar + wchar ;
	  total_io +=  ion->read_write_bytes;
	  assert(total_io>=0);
	  if (!(idle_flag == 1 && rchar == 0 && wchar == 0 && syscr == 0 &&
		syscw == 0 && read_bytes == 0 && write_bytes == 0 &&
		cancelled_write_bytes == 0)) {
#ifdef DEBUG
	    if (hr_flag == 0)
	      printf("%5d %8lld %8lld %8lld %8lld %8lld %8lld %8lld %s %lld %u\n",
		     ion->pid,
		     rchar,
		     wchar,
		     syscr,
		     syscw,
		     read_bytes,
		     write_bytes,
		     cancelled_write_bytes,
		     ion->command,
		     ion->read_write_bytes,ion);
	    else
	      printf("%5d %5s %5s %8lld %8lld %5s %6s %7s %s %lld %u\n",
		     ion->pid,
		     format_b(rchar),
		     format_b(wchar),
		     syscr,
		     syscw,
		     format_b(read_bytes),
		     format_b(write_bytes),
		     format_b(cancelled_write_bytes),
		     ion->command,
		     ion->read_write_bytes,ion);
#endif
	  }
    	}
      upsert_data(ion);
    }
  closedir(dir);
  return;
}

struct io_node *
new_ion(char *pid)
{
  struct io_node *ion;

  ion = (struct io_node *) malloc(sizeof(struct io_node));
  bzero(ion, sizeof(struct io_node));
  ion->pid = atoi(pid);

  return ion;
}

static void upsert_data(struct io_node *ion)
{
  static struct io_node *node;
  static struct list_head *iter;
  node = NULL;
  iter=NULL;

  /* List is empty. */
  if (head == NULL)
    {
      head = ion;
      INIT_LIST_HEAD(&head->list);
      return;
    }

  /* Check if we have seen this pid before. */
  list_for_each(iter,&head->list)
    {
      node = container_of(iter, struct io_node, list);
      assert(node!=NULL);
      // If the process is already in our list then 
      // find the entry update the values and free 
      // the memory allocated for the structure
      if (node->pid == ion->pid)
	{
	  node->rchar = ion->rchar;
	  node->wchar = ion->wchar;
	  node->syscr = ion->syscr;
	  node->syscw = ion->syscw;
	  node->read_bytes = ion->read_bytes;
	  node->write_bytes = ion->write_bytes;
	  node->cancelled_write_bytes = ion->cancelled_write_bytes;
	  node->read_write_bytes = ion->read_write_bytes;
	  /*
	   * If the pids wrap, then the command may be different then before.
	   */
	  strcpy(node->command, ion->command);
	  free(ion);
	  return;
	}
    }

  /* Add this pid to the list. */
  head = insert_ion(ion);
  return;
}

static void delete(struct io_node* temp)
{
  list_del(&temp->list);
  printf("De allocating memory \n");
  free(temp);
  printf("Deallocated memory \n");
  return;
}

int main(int argc, char *argv[])
{
  kb_flag = 1;
  idle_flag = 1;
  int delay = 1;

  my_pid = getpid();
  printf("My pid is %d \n",my_pid);
  printf("Initializing the shared memory and rb-tree");

  struct io_node *temp;
  while(1) 
    {
      total_io = 0;
      get_stats_iotop();
      // Calculate the percentage and iterate populate the data structure 
      temp = head;
      if(total_io==0)
	continue;
      struct list_head *iter = &head->list;
      list_for_each(iter, &head->list)
	{
	  temp = container_of(iter, struct io_node, list);
	  if(temp!=NULL)
	    {
	      if((temp->read_write_bytes)/total_io < 1)
		{
		  if(temp->read_write_bytes!=0)
		    {
		      printf("Pid %d IO percentage  %f\n",temp->pid, (temp->read_write_bytes)*100/total_io);
		    }
		}
	      if((temp->read_write_bytes)/total_io > 1 || !check_if_alive(temp->pid))
		{
		  delete(temp);
		  break;
		}
	    }
	}
      sleep(delay);
      printf("Iter over \n");
    }
  return 0;
}
