#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#define align4(x) (((((x) -1) >> 2) << 2) + 4)
#define BLOCK_SIZE 20

void *base=NULL;

//block structure
struct s_block {
    size_t size;
    struct s_block *next;
    struct s_block *prev;
    int free;
    void *ptr;
    /* A pointer to the allocated block */
    char data [1];
};
typedef struct s_block *t_block;


/* Split block according to size in the case there isn't a perfect fit */
void split_block (t_block b, size_t s)
{
    t_block new;
    new = (t_block )(b->data + s);
    new ->size = b->size - s - BLOCK_SIZE ;
    new ->next = b->next;
    new ->prev = b;
    new ->free = 1;
    new ->ptr = new ->data;
    b->size = s;
    b->next = new;
    if (new ->next)
    new ->next ->prev = new;
}

/* Add a new block to the heap */
/* return NULL on error */
 t_block extend_heap (t_block last , size_t s)
{
    int sb;
    t_block b;
    b = sbrk (0);
    sb = (int)sbrk( BLOCK_SIZE + s);
    if (sb < 0)
        return (NULL );
    b->size = s;
    b->next = NULL;
    b->prev = last;
    b->ptr = b->data;
    if (last)
    last ->next = b;
    b->free = 0;
    return (b);
}

//finds block of appropriate size
t_block find_block (t_block *last , size_t size ){
    t_block b=base;
    while (b && !(b->free && b->size >= size )) {
        *last = b;
        b = b->next;
    }
    return (b);
}

void *my_malloc(size_t size ){
    t_block b,last;
    size_t s;
    s = align4(size );
    
    if (base) {
        /* First find a block */
        last = base;
        b = find_block (&last ,s);
        if (b) {
            /* can the block be split */
            if ((b->size - s) >= ( BLOCK_SIZE + 4))
            split_block (b,s);
            b->free =0;
        }
        else {
        /* No fitting block , extend the heap */
        b = extend_heap (last ,s);
        if (!b)
        return(NULL );
        }
    } else {
    /* first time */
    b = extend_heap (NULL ,s);
    if (!b)
        return(NULL );
    base = b;
    }
    return(b->data );
}

t_block get_block(void *p){
    char *tmp;
    tmp = p;
    return (p = tmp -= BLOCK_SIZE);
}

int valid_addr(void *p){
    if(base && p > base && p<sbrk(0)){
        return 1;
    }
    return 0;
}

void my_free(void *p){
    t_block b;
    
    if (valid_addr(p)){
        b = get_block (p);
        b->free = 1;
    }
}

void my_mallinfo(){
    t_block b =base;
    
    int a = 0;
    int f = 0;
    int l =0;
    
    while (b && !(b->free)){
        a = a + b->size;
        b = b->next;
    }
    while(b && b->free){
        f = f + b->size;
        if(b->size > l){
            l = b->size;
        }
        b=b->next;
    }
    printf("total number of allocated memory:%d, total free space:%d, largest contiguous free space:%d \n", a, f, l);
}

int main(){
 
 int *p= (int) my_malloc(200*sizeof(int));
 char *q= (char) my_malloc(100*sizeof(char));
 int *r= (int) my_malloc(150*sizeof(int));
 int *k= (int) my_malloc(1000*sizeof(int));
 char *y= (char) my_malloc(400*sizeof(char));
 my_free(p);
 my_free(r);
 my_mallinfo();
 printf("Allocation and deallocation successful!");
}