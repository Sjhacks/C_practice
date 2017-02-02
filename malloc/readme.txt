my_mallopt and my_malloc_error aren't implemented

the testing routine is in the main:
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

output:
total number of allocated memory:0, total free space:400, largest contiguous free space:400

Allocation and deallocation successful!