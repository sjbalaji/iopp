/* red_black_tree.c --- 
 * Filename: red_black_tree.c
 * Description: 
 * Author: balaji
 * Email:  sjbalajimdu@gmail.com
 * Organization:  IIT Madras
 * Created: Mon Feb 18 17:56:12 2013 (+0530)
 * Last-Updated: Thu Mar  7 22:39:18 2013 (+0530)
 *           By: balaji
 *     Update #: 27
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


#include "red_black_tree.h"
#include "list.h"


//////////////////////////////////////////////////////////
// Locks from: http://locklessinc.com/articles/locks/

/* Atomic exchange (of various sizes) */
static inline void *xchg_64(void *ptr, void *x)
{
  __asm__ __volatile__("xchgq %0,%1"
		       :"=r" ((unsigned long long) x)
		       :"m" (*(volatile long long *)ptr), "0" ((unsigned long long) x)
		       :"memory");

  return x;
}

static inline unsigned xchg_32(void *ptr, unsigned x)
{
  __asm__ __volatile__("xchgl %0,%1"
		       :"=r" ((unsigned) x)
		       :"m" (*(volatile unsigned *)ptr), "0" (x)
		       :"memory");

  return x;
}

static inline unsigned short xchg_16(void *ptr, unsigned short x)
{
  __asm__ __volatile__("xchgw %0,%1"
		       :"=r" ((unsigned short) x)
		       :"m" (*(volatile unsigned short *)ptr), "0" (x)
		       :"memory");

  return x;
}

void spin_lock(spinlock *lock)
{
  while (1)
    {
      if (!xchg_32(lock, EBUSY)) return;
	
      while (*lock) cpu_relax();
    }
}

void spin_unlock(spinlock *lock)
{
  barrier();
  *lock = 0;
}

static int spin_trylock(spinlock *lock)
{
  return xchg_32(lock, EBUSY);
}

// Locks from: http://locklessinc.com/articles/locks/
//////////////////////////////////////////////////////////

int IntComp(const int a,const int b) {
  if( a > b) return(1);
  if( a < b) return(-1);
  return(0);
}

void IntDest(int a) {
  ;
}


void IntPrint(const double a) {
  printf("%f",a);
}

void InfoPrint(double a) {
  printf(" info %f -- ",a);
}

void InfoDest(int a){
  ;
}
// Size defines the number of elements in the sm
// This functions return the root of the rb_tree
rb_red_blk_node* init_shared_memory(rb_red_blk_node* sm, int size)
{
  // Initialize the free list 
  struct rb_red_blk_node* ne = &sm[1];
  memset(ne, 0, sizeof(struct rb_red_blk_node)*(size-1));
  struct list_head *head = &sm[0].list;
  sm[0].number=0;
  INIT_LIST_HEAD(head);
  int i;
  for(i=1;i<size;i++)
    {
      list_add_tail(&sm[i].list, head);
    }
  return RBTreeCreate(IntComp,IntDest,InfoDest,IntPrint,InfoPrint,sm);
}

rb_red_blk_node* get_node(rb_red_blk_node* root)
{
  struct list_head* head = &root->list;
  struct list_head* free_ptr = head->next;
  struct rb_red_blk_node* node;
  assert(free_ptr!=NULL);
  // If exits then not enough space
  list_del(free_ptr);
  node =  container_of(free_ptr, struct rb_red_blk_node, list);
  assert(node!=NULL);
  // Before returning the new node
  // Make all the values zero 
  node->key=0;
  int i=0;
  for(i=0;i<10;i++)
    node->utilization[i]=0;
  return node;
}

int free_node(rb_red_blk_node* root, rb_red_blk_node* node)
{
  // This function frees the shared memory and adds the freed memory in
  // the free_memory list 
  struct list_head* head = &root->list;
  struct list_head* lptr = &node->list;
  node->key = -1;
  node->utilization[0]=-1;
  list_add_tail(lptr, head);
  return 0;
}

/***********************************************************************/
/*  FUNCTION:  RBTreeCreate */
/**/
/*  INPUTS:  All the inputs are names of functions.  CompFunc takes to */
/*  void pointers to keys and returns 1 if the first arguement is */
/*  "greater than" the second.   DestFunc takes a pointer to a key and */
/*  destroys it in the appropriate manner when the node containing that */
/*  key is deleted.  InfoDestFunc is similiar to DestFunc except it */
/*  recieves a pointer to the info of a node and destroys it. */
/*  PrintFunc recieves a pointer to the key of a node and prints it. */
/*  PrintInfo recieves a pointer to the info of a node and prints it. */
/*  If RBTreePrint is never called the print functions don't have to be */
/*  defined and NullFunction can be used.  */
/**/
/*  OUTPUT:  This function returns a pointer to the newly created */
/*  red-black tree. */
/**/
/*  Modifies Input: none */
/***********************************************************************/

rb_red_blk_node* RBTreeCreate(int (*CompFunc) (const int,const int),
			      void (*DestFunc) (int),
			      void (*InfoDestFunc) (int),
			      void (*PrintFunc) (const double),
			      void (*PrintInfo)(double),
			      rb_red_blk_node* sm) 
{
  rb_red_blk_node* newTree;
  rb_red_blk_node* temp;
  assert(sm!=NULL);
  newTree=(rb_red_blk_node*) &sm[0];
  newTree->Compare= (int(*)(const int, const int)) CompFunc;
  newTree->DestroyKey= DestFunc;
  newTree->PrintKey= PrintFunc;
  newTree->PrintInfo= PrintInfo;
  newTree->DestroyInfo= InfoDestFunc;

  /*  see the comment in the rb_red_blk_node structure in red_black_tree.h */
  /*  for information on nil and root */
  temp=newTree->nil= (rb_red_blk_node*) get_node(&sm[0]);
  temp->parent=temp->left=temp->right=temp;
  temp->red=0;
  temp->key=0;
  temp=newTree->root= (rb_red_blk_node*) get_node(&sm[0]);
  temp->parent=temp->left=temp->right=newTree->nil;
  temp->key=0;
  temp->red=0;
  newTree->lock=0;
  return(newTree);
}

rb_red_blk_node* RBTreeCreateFake(int (*CompFunc) (const int,const int),
				  void (*DestFunc) (int),
				  void (*InfoDestFunc) (int),
				  void (*PrintFunc) (const double),
				  void (*PrintInfo)(double),
				  rb_red_blk_node* sm) 
{
  rb_red_blk_node* newTree;
  rb_red_blk_node* temp;
  assert(sm!=NULL);
  newTree=(struct rb_red_blk_node*)malloc(sizeof(struct rb_red_blk_node));

  newTree->Compare= CompFunc;
  newTree->DestroyKey= DestFunc;
  newTree->PrintKey= PrintFunc;
  newTree->PrintInfo= PrintInfo;
  newTree->DestroyInfo= InfoDestFunc;
  newTree->list = sm[0].list;

  temp=newTree->nil= sm[0].nil;
  temp->parent=temp->left=temp->right=temp;
  temp->red=0;
  temp->key=0;
  temp=newTree->root=sm[0].root;
  temp->parent=temp->left=temp->right=newTree->nil;
  temp->key=0;
  temp->red=0;
  newTree->lock=0;
  return(newTree);
}

/***********************************************************************/
/*  FUNCTION:  LeftRotate */
/**/
/*  INPUTS:  This takes a tree so that it can access the appropriate */
/*           root and nil pointers, and the node to rotate on. */
/**/
/*  OUTPUT:  None */
/**/
/*  Modifies Input: tree, x */
/**/
/*  EFFECTS:  Rotates as described in _Introduction_To_Algorithms by */
/*            Cormen, Leiserson, Rivest (Chapter 14).  Basically this */
/*            makes the parent of x be to the left of x, x the parent of */
/*            its parent before the rotation and fixes other pointers */
/*            accordingly. */
/***********************************************************************/

void LeftRotate(rb_red_blk_node* tree, rb_red_blk_node* x) {
  rb_red_blk_node* y;
  rb_red_blk_node* nil=tree->nil;

  /*  I originally wrote this function to use the sentinel for */
  /*  nil to avoid checking for nil.  However this introduces a */
  /*  very subtle bug because sometimes this function modifies */
  /*  the parent pointer of nil.  This can be a problem if a */
  /*  function which calls LeftRotate also uses the nil sentinel */
  /*  and expects the nil sentinel's parent pointer to be unchanged */
  /*  after calling this function.  For example, when RBDeleteFixUP */
  /*  calls LeftRotate it expects the parent pointer of nil to be */
  /*  unchanged. */

  y=x->right;
  x->right=y->left;

  if (y->left != nil) y->left->parent=x; /* used to use sentinel here */
  /* and do an unconditional assignment instead of testing for nil */
  
  y->parent=x->parent;   

  /* instead of checking if x->parent is the root as in the book, we */
  /* count on the root sentinel to implicitly take care of this case */
  if( x == x->parent->left) {
    x->parent->left=y;
  } else {
    x->parent->right=y;
  }
  y->left=x;
  x->parent=y;

#ifdef DEBUG_ASSERT
  Assert(!tree->nil->red,"nil not red in LeftRotate");
#endif
}


/***********************************************************************/
/*  FUNCTION:  RighttRotate */
/**/
/*  INPUTS:  This takes a tree so that it can access the appropriate */
/*           root and nil pointers, and the node to rotate on. */
/**/
/*  OUTPUT:  None */
/**/
/*  Modifies Input?: tree, y */
/**/
/*  EFFECTS:  Rotates as described in _Introduction_To_Algorithms by */
/*            Cormen, Leiserson, Rivest (Chapter 14).  Basically this */
/*            makes the parent of x be to the left of x, x the parent of */
/*            its parent before the rotation and fixes other pointers */
/*            accordingly. */
/***********************************************************************/

void RightRotate(rb_red_blk_node* tree, rb_red_blk_node* y) {
  rb_red_blk_node* x;
  rb_red_blk_node* nil=tree->nil;

  /*  I originally wrote this function to use the sentinel for */
  /*  nil to avoid checking for nil.  However this introduces a */
  /*  very subtle bug because sometimes this function modifies */
  /*  the parent pointer of nil.  This can be a problem if a */
  /*  function which calls LeftRotate also uses the nil sentinel */
  /*  and expects the nil sentinel's parent pointer to be unchanged */
  /*  after calling this function.  For example, when RBDeleteFixUP */
  /*  calls LeftRotate it expects the parent pointer of nil to be */
  /*  unchanged. */

  x=y->left;
  y->left=x->right;

  if (nil != x->right)  x->right->parent=y; /*used to use sentinel here */
  /* and do an unconditional assignment instead of testing for nil */

  /* instead of checking if x->parent is the root as in the book, we */
  /* count on the root sentinel to implicitly take care of this case */
  x->parent=y->parent;
  if( y == y->parent->left) {
    y->parent->left=x;
  } else {
    y->parent->right=x;
  }
  x->right=y;
  y->parent=x;

#ifdef DEBUG_ASSERT
  Assert(!tree->nil->red,"nil not red in RightRotate");
#endif
}

/***********************************************************************/
/*  FUNCTION:  TreeInsertHelp  */
/**/
/*  INPUTS:  tree is the tree to insert into and z is the node to insert */
/**/
/*  OUTPUT:  none */
/**/
/*  Modifies Input:  tree, z */
/**/
/*  EFFECTS:  Inserts z into the tree as if it were a regular binary tree */
/*            using the algorithm described in _Introduction_To_Algorithms_ */
/*            by Cormen et al.  This funciton is only intended to be called */
/*            by the RBTreeInsert function and not by the user */
/***********************************************************************/

void TreeInsertHelp(rb_red_blk_node* tree, rb_red_blk_node* z) {
  /*  This function should only be called by InsertRBTree (see above) */
  rb_red_blk_node* x;
  rb_red_blk_node* y;
  rb_red_blk_node* nil=tree->nil;
  
  z->left=z->right=nil;
  y=tree->root;
  x=tree->root->left;
  while( x != nil) {
    y=x;
    if (1 == tree->Compare(x->key,z->key)) { /* x.key > z.key */
      x=x->left;
    } else { /* x,key <= z.key */
      x=x->right;
    }
  }
  z->parent=y;
  if ( (y == tree->root) ||
       (1 == tree->Compare(y->key,z->key))) { /* y.key > z.key */
    y->left=z;
  } else {
    y->right=z;
  }

#ifdef DEBUG_ASSERT
  Assert(!tree->nil->red,"nil not red in TreeInsertHelp");
#endif
}

/*  Before calling Insert RBTree the node x should have its key set */

/***********************************************************************/
/*  FUNCTION:  RBTreeInsert */
/**/
/*  INPUTS:  tree is the red-black tree to insert a node which has a key */
/*           pointed to by key and info pointed to by info.  */
/**/
/*  OUTPUT:  This function returns a pointer to the newly inserted node */
/*           which is guarunteed to be valid until this node is deleted. */
/*           What this means is if another data structure stores this */
/*           pointer then the tree does not need to be searched when this */
/*           is to be deleted. */
/**/
/*  Modifies Input: tree */
/**/
/*  EFFECTS:  Creates a node node which contains the appropriate key and */
/*            info pointers and inserts it into the tree. */
/***********************************************************************/

rb_red_blk_node * RBTreeInsert(rb_red_blk_node* tree, int key, double utilization, int position) {
  rb_red_blk_node * y;
  rb_red_blk_node * x;
  rb_red_blk_node * newNode;

  x=(rb_red_blk_node*) get_node(tree);
  x->key=key;
  x->utilization[position]=utilization;

  TreeInsertHelp(tree,x);
  newNode=x;
  x->red=1;
  while(x->parent->red) { /* use sentinel instead of checking for root */
    if (x->parent == x->parent->parent->left) {
      y=x->parent->parent->right;
      if (y->red) {
	x->parent->red=0;
	y->red=0;
	x->parent->parent->red=1;
	x=x->parent->parent;
      } else {
	if (x == x->parent->right) {
	  x=x->parent;
	  LeftRotate(tree,x);
	}
	x->parent->red=0;
	x->parent->parent->red=1;
	RightRotate(tree,x->parent->parent);
      } 
    } else { /* case for x->parent == x->parent->parent->right */
      y=x->parent->parent->left;
      if (y->red) {
	x->parent->red=0;
	y->red=0;
	x->parent->parent->red=1;
	x=x->parent->parent;
      } else {
	if (x == x->parent->left) {
	  x=x->parent;
	  RightRotate(tree,x);
	}
	x->parent->red=0;
	x->parent->parent->red=1;
	LeftRotate(tree,x->parent->parent);
      } 
    }
  }
  tree->root->left->red=0;
  return(newNode);

#ifdef DEBUG_ASSERT
  Assert(!tree->nil->red,"nil not red in RBTreeInsert");
  Assert(!tree->root->red,"root not red in RBTreeInsert");
#endif
}

/***********************************************************************/
/*  FUNCTION:  TreeSuccessor  */
/**/
/*    INPUTS:  tree is the tree in question, and x is the node we want the */
/*             the successor of. */
/**/
/*    OUTPUT:  This function returns the successor of x or NULL if no */
/*             successor exists. */
/**/
/*    Modifies Input: none */
/**/
/*    Note:  uses the algorithm in _Introduction_To_Algorithms_ */
/***********************************************************************/
  
rb_red_blk_node* TreeSuccessor(rb_red_blk_node* tree,rb_red_blk_node* x) { 
  rb_red_blk_node* y;
  rb_red_blk_node* nil=tree->nil;
  rb_red_blk_node* root=tree->root;

  if (nil != (y = x->right)) { /* assignment to y is intentional */
    while(y->left != nil) { /* returns the minium of the right subtree of x */
      y=y->left;
    }
    return(y);
  } else {
    y=x->parent;
    while(x == y->right) { /* sentinel used instead of checking for nil */
      x=y;
      y=y->parent;
    }
    if (y == root) return(nil);
    return(y);
  }
}

/***********************************************************************/
/*  FUNCTION:  Treepredecessor  */
/**/
/*    INPUTS:  tree is the tree in question, and x is the node we want the */
/*             the predecessor of. */
/**/
/*    OUTPUT:  This function returns the predecessor of x or NULL if no */
/*             predecessor exists. */
/**/
/*    Modifies Input: none */
/**/
/*    Note:  uses the algorithm in _Introduction_To_Algorithms_ */
/***********************************************************************/

rb_red_blk_node* TreePredecessor(rb_red_blk_node* tree, rb_red_blk_node* x) {
  rb_red_blk_node* y;
  rb_red_blk_node* nil=tree->nil;
  rb_red_blk_node* root=tree->root;

  if (nil != (y = x->left)) { /* assignment to y is intentional */
    while(y->right != nil) { /* returns the maximum of the left subtree of x */
      y=y->right;
    }
    return(y);
  } else {
    y=x->parent;
    while(x == y->left) { 
      if (y == root) return(nil); 
      x=y;
      y=y->parent;
    }
    return(y);
  }
}

/***********************************************************************/
/*  FUNCTION:  InorderTreePrint */
/**/
/*    INPUTS:  tree is the tree to print and x is the current inorder node */
/**/
/*    OUTPUT:  none  */
/**/
/*    EFFECTS:  This function recursively prints the nodes of the tree */
/*              inorder using the PrintKey and PrintInfo functions. */
/**/
/*    Modifies Input: none */
/**/
/*    Note:    This function should only be called from RBTreePrint */
/***********************************************************************/
void print_utilization(struct rb_red_blk_node* node)
{
  int i=0;
  for(i=0;i<10;i++)
    printf(" %f", node->utilization[i]);
  printf("\n");
}

void InorderTreePrint(rb_red_blk_node* tree, rb_red_blk_node* x) {
#ifdef DEBUG
  rb_red_blk_node* nil=tree->nil;
  rb_red_blk_node* root=tree->root;
  if (x != tree->nil) {
    InorderTreePrint(tree,x->left);
    // printf("info=");
    // 
    printf("  key="); 
    tree->PrintKey(x->key);
    //    tree->PrintInfo(x->utilization[0]);
    print_utilization(x);
    /* printf("  l->key="); */
    /* if( x->left == nil) printf("NULL"); else tree->PrintKey(x->left->key); */
    /* printf("  r->key="); */
    /* if( x->right == nil) printf("NULL"); else tree->PrintKey(x->right->key); */
    /* printf("  p->key="); */
    /* if( x->parent == root) printf("NULL"); else tree->PrintKey(x->parent->key); */
    /* printf("  red=%i\n",x->red); */
    InorderTreePrint(tree,x->right);
  }
#endif
}


/***********************************************************************/
/*  FUNCTION:  TreeDestHelper */
/**/
/*    INPUTS:  tree is the tree to destroy and x is the current node */
/**/
/*    OUTPUT:  none  */
/**/
/*    EFFECTS:  This function recursively destroys the nodes of the tree */
/*              postorder using the DestroyKey and DestroyInfo functions. */
/**/
/*    Modifies Input: tree, x */
/**/
/*    Note:    This function should only be called by RBTreeDestroy */
/***********************************************************************/

void TreeDestHelper(rb_red_blk_node* tree, rb_red_blk_node* x) {
  rb_red_blk_node* nil=tree->nil;
  if (x != nil) {
    TreeDestHelper(tree,x->left);
    TreeDestHelper(tree,x->right);
    free_node(tree,x);
  }
}


/***********************************************************************/
/*  FUNCTION:  RBTreeDestroy */
/**/
/*    INPUTS:  tree is the tree to destroy */
/**/
/*    OUTPUT:  none */
/**/
/*    EFFECT:  Destroys the key and frees memory */
/**/
/*    Modifies Input: tree */
/**/
/***********************************************************************/

void RBTreeDestroy(rb_red_blk_node* tree) {
  TreeDestHelper(tree,tree->root->left);
  free_node(tree,tree->root);
  free_node(tree,tree->nil);
  free_node(tree,tree);
}


/***********************************************************************/
/*  FUNCTION:  RBTreePrint */
/**/
/*    INPUTS:  tree is the tree to print */
/**/
/*    OUTPUT:  none */
/**/
/*    EFFECT:  This function recursively prints the nodes of the tree */
/*             inorder using the PrintKey and PrintInfo functions. */
/**/
/*    Modifies Input: none */
/**/
/***********************************************************************/

void RBTreePrint(rb_red_blk_node* tree) {
  InorderTreePrint(tree,tree->root->left);
}


/***********************************************************************/
/*  FUNCTION:  RBExactQuery */
/**/
/*    INPUTS:  tree is the tree to print and q is a pointer to the key */
/*             we are searching for */
/**/
/*    OUTPUT:  returns the a node with key equal to q.  If there are */
/*             multiple nodes with key equal to q this function returns */
/*             the one highest in the tree */
/**/
/*    Modifies Input: none */
/**/
/***********************************************************************/
  
rb_red_blk_node* RBExactQuery(rb_red_blk_node* tree, int q) {
  rb_red_blk_node* x=tree->root->left;
  rb_red_blk_node* nil=tree->nil;
  int compVal;
  if (x == nil) return(0);
  compVal=tree->Compare(x->key, q);
  while(0 != compVal) {/*assignemnt*/
    if (1 == compVal) { /* x->key > q */
      x=x->left;
    } else {
      x=x->right;
    }
    if ( x == nil) return(0);
    compVal=tree->Compare(x->key, q);
  }
  return(x);
}


/***********************************************************************/
/*  FUNCTION:  RBDeleteFixUp */
/**/
/*    INPUTS:  tree is the tree to fix and x is the child of the spliced */
/*             out node in RBTreeDelete. */
/**/
/*    OUTPUT:  none */
/**/
/*    EFFECT:  Performs rotations and changes colors to restore red-black */
/*             properties after a node is deleted */
/**/
/*    Modifies Input: tree, x */
/**/
/*    The algorithm from this function is from _Introduction_To_Algorithms_ */
/***********************************************************************/

void RBDeleteFixUp(rb_red_blk_node* tree, rb_red_blk_node* x) {
  rb_red_blk_node* root=tree->root->left;
  rb_red_blk_node* w;

  while( (!x->red) && (root != x)) {
    if (x == x->parent->left) {
      w=x->parent->right;
      if (w->red) {
	w->red=0;
	x->parent->red=1;
	LeftRotate(tree,x->parent);
	w=x->parent->right;
      }
      if ( (!w->right->red) && (!w->left->red) ) { 
	w->red=1;
	x=x->parent;
      } else {
	if (!w->right->red) {
	  w->left->red=0;
	  w->red=1;
	  RightRotate(tree,w);
	  w=x->parent->right;
	}
	w->red=x->parent->red;
	x->parent->red=0;
	w->right->red=0;
	LeftRotate(tree,x->parent);
	x=root; /* this is to exit while loop */
      }
    } else { /* the code below is has left and right switched from above */
      w=x->parent->left;
      if (w->red) {
	w->red=0;
	x->parent->red=1;
	RightRotate(tree,x->parent);
	w=x->parent->left;
      }
      if ( (!w->right->red) && (!w->left->red) ) { 
	w->red=1;
	x=x->parent;
      } else {
	if (!w->left->red) {
	  w->right->red=0;
	  w->red=1;
	  LeftRotate(tree,w);
	  w=x->parent->left;
	}
	w->red=x->parent->red;
	x->parent->red=0;
	w->left->red=0;
	RightRotate(tree,x->parent);
	x=root; /* this is to exit while loop */
      }
    }
  }
  x->red=0;

#ifdef DEBUG_ASSERT
  Assert(!tree->nil->red,"nil not black in RBDeleteFixUp");
#endif
}


/***********************************************************************/
/*  FUNCTION:  RBDelete */
/**/
/*    INPUTS:  tree is the tree to delete node z from */
/**/
/*    OUTPUT:  none */
/**/
/*    EFFECT:  Deletes z from tree and frees the key and info of z */
/*             using DestoryKey and DestoryInfo.  Then calls */
/*             RBDeleteFixUp to restore red-black properties */
/**/
/*    Modifies Input: tree, z */
/**/
/*    The algorithm from this function is from _Introduction_To_Algorithms_ */
/***********************************************************************/

void RBDelete(rb_red_blk_node* tree, rb_red_blk_node* z) 
{
  rb_red_blk_node* y;
  rb_red_blk_node* x;
  rb_red_blk_node* nil=tree->nil;
  rb_red_blk_node* root=tree->root;

  y= ((z->left == nil) || (z->right == nil)) ? z : TreeSuccessor(tree,z);
  x= (y->left == nil) ? y->right : y->left;
  if (root == (x->parent = y->parent)) { /* assignment of y->p to x->p is intentional */
    root->left=x;
  } else {
    if (y == y->parent->left) {
      y->parent->left=x;
    } else {
      y->parent->right=x;
    }
  }
  if (y != z) { /* y should not be nil in this case */

#ifdef DEBUG_ASSERT
    Assert( (y!=tree->nil),"y is nil in RBDelete\n");
#endif
    /* y is the node to splice out and x is its child */

    if (!(y->red)) RBDeleteFixUp(tree,x);
  
    y->left=z->left;
    y->right=z->right;
    y->parent=z->parent;
    y->red=z->red;
    z->left->parent=z->right->parent=y;
    if (z == z->parent->left) {
      z->parent->left=y; 
    } else {
      z->parent->right=y;
    }
    free_node(tree,z); 
  } else {
    if (!(y->red)) RBDeleteFixUp(tree,x);
    free_node(tree,y);
  }
  
#ifdef DEBUG_ASSERT
  Assert(!tree->nil->red,"nil not black in RBDelete");
#endif
}

int delete_key(struct rb_red_blk_node* root, int key)
{
  struct rb_red_blk_node* node = RBExactQuery(root,key);
  if(node!=NULL)
    {
      spin_lock(&root->lock);
      RBDelete(root,node);
      printf("Deleting key %d\n",key);
      root->number=root->number-1;
      spin_unlock(&root->lock);
    }
  return 0;
}

int update_key(struct rb_red_blk_node* root, int key, double value, int position)
{
  struct rb_red_blk_node* node = RBExactQuery(root,key);
  if(node!=NULL)
    {
      node->utilization[position] = value;
    }
  else if(value!=0)
    {
      spin_lock(&root->lock);
#ifdef PRINT
      printf("Adding %d \n",key);
#endif 
      RBTreeInsert(root,key,value,position);
      root->number++;
      spin_unlock(&root->lock);
    }
  return 0;
}
