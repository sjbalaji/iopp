/* red_black_tree.h --- 
 * Filename: red_black_tree.h
 * Description: 
 * Author: balaji
 * Email:  sjbalajimdu@gmail.com
 * Organization:  IIT Madras
 * Created: Mon Feb 18 17:56:43 2013 (+0530)
 * Last-Updated: Thu Mar  7 05:14:27 2013 (+0530)
 *           By: balaji
 *     Update #: 12
 */

// The original red black tree implementation is taken from 
// http://web.mit.edu/~emin/www.old/source_code/red_black_tree/index.html
// The rb tree is modified as per my requirement 

/* Redistribution and use in source and binary forms, with or without */
/* modification, are permitted provided that neither the name of Emin */
/* Martinian nor the names of any contributors are be used to endorse or */
/* promote products derived from this software without specific prior */
/* written permission. */

/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS */
/* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT */
/* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR */
/* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT */
/* OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, */
/* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT */
/* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, */
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY */
/* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT */
/* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE */
/* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */


#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include "misc.h"
/* #include "stack.h" */
#include "list.h"
#include <assert.h>
#include <string.h>

/*  CONVENTIONS:  All data structures for red-black trees have the prefix */
/*                "rb_" to prevent name conflicts. */
/*                                                                      */
/*                Function names: Each word in a function name begins with */
/*                a capital letter.  An example funcntion name is  */
/*                CreateRedTree(a,b,c). Furthermore, each function name */
/*                should begin with a capital letter to easily distinguish */
/*                them from variables. */
/*                                                                     */
/*                Variable names: Each word in a variable name begins with */
/*                a capital letter EXCEPT the first letter of the variable */
/*                name.  For example, int newLongInt.  Global variables have */
/*                names beginning with "g".  An example of a global */
/*                variable name is gNewtonsConstant. */

/* comment out the line below to remove all the debugging assertion */
/* checks from the compiled code.  */
#define DEBUG_ASSERT 1

/* Compile read-write barrier */
#define barrier() asm volatile("": : :"memory")

/* Pause instruction to prevent excess processor bus usage */ 
#define cpu_relax() asm volatile("pause\n": : :"memory")
#define EBUSY 1
typedef unsigned spinlock;
int IntComp(const int a,const int b);
void IntDest(int a);
void IntPrint(const double a);
void InfoPrint(double a);
void InfoDest(int a);

void spin_lock(spinlock *lock);
void spin_unlock(spinlock *lock);

typedef struct rb_red_blk_node {

  union{
    int key;
    int (*Compare)(const int a, const int b); 
  }__attribute__((__packed__));

  union{
    double utilization[10];
    void (*DestroyKey)(int a);
  }__attribute__((__packed__));

  union{
    void *f3;
    void (*DestroyInfo)(int a);
  }__attribute__((__packed__));

  union{
    int red; /* if red=0 then the node is black */
    void (*PrintKey)(const double a);
  }__attribute__((__packed__));

  union{
    struct rb_red_blk_node* left;
    void (*PrintInfo)(double a);
  }__attribute__((__packed__));

  union{
  struct rb_red_blk_node* right;
  struct rb_red_blk_node* root; // Containt the tree          
  }__attribute__((__packed__));

  union{
  struct rb_red_blk_node* parent;
  struct rb_red_blk_node* nil;              
  }__attribute__((__packed__));

  // In case of the root it contains the
  // free list pointer
  struct list_head list; 
  spinlock lock;
  int number;
} rb_red_blk_node;

// have the shared memory pointer and initialize them in a list
rb_red_blk_node* init_shared_memory(rb_red_blk_node* sm,int size); 

// This function replaces the Safemalloc for node memory alloc
// This function allocates the memory from the shared memory 
rb_red_blk_node * get_node(rb_red_blk_node *root);

rb_red_blk_node * RBTreeCreate(int  (*CompFunc)(const int, const int),
			       void (*DestFunc)(int), 
			       void (*InfoDestFunc)(int), 
			       void (*PrintFunc)(const double),
			       void (*PrintInfo)(double),
			       rb_red_blk_node* sm);

rb_red_blk_node * RBTreeCreateFake(int  (*CompFunc)(const int, const int),
			       void (*DestFunc)(int), 
			       void (*InfoDestFunc)(int), 
			       void (*PrintFunc)(const double),
			       void (*PrintInfo)(double),
			       rb_red_blk_node* sm);

rb_red_blk_node * RBTreeInsert(rb_red_blk_node* root, int key, double utilization, int position);

void RBTreePrint(rb_red_blk_node* root);

// Return the memory to the free list 
void RBDelete(rb_red_blk_node* root , rb_red_blk_node* node);
void RBTreeDestroy(rb_red_blk_node* root);
rb_red_blk_node* TreePredecessor(rb_red_blk_node* root,rb_red_blk_node* node);
rb_red_blk_node* TreeSuccessor(rb_red_blk_node *root,rb_red_blk_node* node);
rb_red_blk_node* RBExactQuery(rb_red_blk_node* root, int value);
int delete_key(struct rb_red_blk_node* root, int key);
int update_key(struct rb_red_blk_node* root, int key, double value, int position);
void check_key(struct rb_red_blk_node* root, int key, double percent,int position, int nr_entries);
/* stk_stack * RBEnumerate(rb_red_blk_node* root,int low, int high); */
void print_utilization(struct rb_red_blk_node* node);
void NullFunction(void*);
