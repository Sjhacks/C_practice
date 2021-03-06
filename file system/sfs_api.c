#include "sfs_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "disk_emu.h"
#include <strings.h>
#include <unistd.h>

int seen = 0;

#define DISK_FILE "sfs_disk.disk"
#define BLOCK_SIZE 512
#define MAX_BLOCKS 100

#define MAX_INODES 5
#define MAX_FILES 3

super_block_t sb;
dir_entry_t root_dir[MAX_INODES];

inode_t inode_table[MAX_INODES];
fd_table_t fd_table[MAX_FILES];

unsigned short free_blocks[MAX_BLOCKS];

char file_data[] = "abcdefghijklmnopqrstuvwxyz";

void init_superblock(){

        // init the superblock
        sb.magic = 1234;
        sb.block_size = BLOCK_SIZE;
        sb.fs_size = MAX_BLOCKS*BLOCK_SIZE;
        sb.inode_table_len = MAX_INODES;
        sb.root_dir_inode = 0;
}

void add_root_dir_inode(){

        //first entry in the inode table is the root
        inode_table[0].mode =  0x755;
        inode_table[0].link_cnt = 1;
        inode_table[0].uid = 0;
        inode_table[0].gid = 0;
        inode_table[0].size = 45;
        inode_table[0].data_ptrs[0] = 2; //root dir is stored in the 3rd block


}

void add_dummy_file_inode(){

        //second entry is the dummy file
        inode_table[1].mode =  0x755;
        inode_table[1].link_cnt = 1;
        inode_table[1].uid = 0;
        inode_table[1].gid = 0;
        inode_table[1].size = strlen(file_data);
        inode_table[1].data_ptrs[0] = 3; //dummy file data is stored in the 4th block

}

void add_dummy_file_dir_entry()  {
        
        root_dir[0].inode_idx = 1;
        strcpy(root_dir[0].name, "dummyfile.txt");

}

void add_dummy_file_data() { 
       
        write_blocks(3,1, &file_data);
}

void zero_everything(){

        bzero(&sb, sizeof(super_block_t));
        bzero(&fd_table[0], sizeof(fd_table_t)*MAX_FILES);
        bzero(&inode_table[0], sizeof(inode_t)*MAX_INODES);
        bzero(&root_dir, sizeof(dir_entry_t));
        bzero(&free_blocks[0], sizeof(unsigned int)*MAX_BLOCKS);

}

void mksfs(int fresh) {
	//Implement mksfs here	
	if (fresh == 1) {

                //begin
                printf("Initalizing sfs\n");
		init_fresh_disk(DISK_FILE, BLOCK_SIZE, MAX_BLOCKS);
                zero_everything();


		// write superblock to the first block
                printf("Writing superblocks\n");
                init_superblock();
		write_blocks(0, 1, &sb);
	
		
		// write the inode table to the 2nd block
                printf("Writing inode table\n");
                add_root_dir_inode();
                add_dummy_file_inode();
		write_blocks(1, 1, &inode_table);

                // write root directory data to the 3rd block
                printf("Writing root dir\n");
                add_dummy_file_dir_entry();
                write_blocks(2,1, &root_dir);

                // write file data to the 4th block
                printf("Writing file data\n");
                add_dummy_file_data();


                //mark blocks as used
                printf("Writing free blocks\n");
		free_blocks[0] = 1; //superblock
		free_blocks[1] = 1; //inode table
		free_blocks[2] = 1; //root dir data
		free_blocks[3] = 1; //dummy file data
		free_blocks[MAX_BLOCKS-1] = 1; //free blocks

                // write the free blocks to the disk
		write_blocks(MAX_BLOCKS - 1, 1, &free_blocks);

	} else {

		init_disk(DISK_FILE, BLOCK_SIZE, MAX_BLOCKS);
                // pull back data from disk to mem
	}

	return;
}

int sfs_getnextfilename(char *fname) {
	
        static int seen = 0;

        printf("Calling sfs get next file\n");
	
        if(!seen){

                seen = 1;
		strcpy(fname, root_dir[0].name);
	        return 1;
	}
        
        seen = 0;
	return 0;
}


int sfs_getfilesize(const char* path) {

	//Implement sfs_getfilesize here	
	int inode_idx =  root_dir[0].inode_idx;
        inode_t inode = inode_table[inode_idx];

        return inode.size;
}

int sfs_fopen(char *name) {
	
	//Implement sfs_fopen here	
	int exists = 1;
        int i = 0;
        int j = 0;
        int k=0;

        printf("Opening %s\n", name);

	if (! exists ) {
                printf("Create file\n");
                for ( i=0; i < MAX_BLOCKS; i++ ){
                    if (free_blocks[i] == 0){
                        while (inode_table[j].link_cnt == 0){
                            j++;
                        }
                    inode_table[j].mode =  0x755;
                    inode_table[j].link_cnt = 1;
                    inode_table[j].uid = 0;
                    inode_table[j].gid = 0;
                    inode_table[j].size = 0;
                    inode_table[j].data_ptrs[0] = i;
                    write_blocks(1, 1, &inode_table);
                    
                    root_dir[j-1].inode_idx = j;
                    strcpy(root_dir[j-1].name, name);
                    write_blocks(2,1, &root_dir);
                    }
                    
                    else{
                        printf("no free space\n");
                    }
                }
	}
        while(strcmp(root_dir[k].name, name) != 0){
            k++;
        }
        //set the fd table
        fd_table[3].inode_idx = root_dir[k].inode_idx;
        fd_table[3].rd_write_ptr = 0;

	return 3;
}

int sfs_fclose(int fileID){

	//Implement sfs_fclose here	
        fd_table[fileID].inode_idx = 0;
        fd_table[fileID].rd_write_ptr = 0;

	return 0;
}

int sfs_fread(int fileID, char *buf, int length){

	//Implement sfs_fread here	
        unsigned int inode_idx = fd_table[fileID].inode_idx;
        unsigned int cur_pos = fd_table[fileID].rd_write_ptr;
        char buffer[BLOCK_SIZE];

        inode_t file_inode = inode_table[inode_idx];
        
        //trying to read beyond the file
        if (cur_pos + length > file_inode.size){
                length = file_inode.size - cur_pos;
        }

        if (length){

                //find how many blocks to read
                int block_idx = file_inode.data_ptrs[0];
                read_blocks(block_idx, 1, &buffer[0]);

                //check if length > block in that case read more

                memcpy(buf, buffer, length);
                return length;
        }

	return 0;
}

int sfs_fwrite(int fileID, const char *buf, int length){

	//unsigned int inode_idx = fd_table[fileID].inode_idx;
        //unsigned int cur_pos = fd_table[fileID].rd_write_ptr;
        //char buffer[BLOCK_SIZE];
        int i=0 ;
        int j=0;
        
        for (i = 0; i > MAX_BLOCKS-1; i++){
            if(free_blocks[i] == 0){
                free_blocks[i] = 1;
                write_blocks(MAX_BLOCKS - 1, 1, &free_blocks);
                
                for(j=0; j<MAX_BLOCKS-1; j++){
                    if(inode_table[fileID].data_ptrs[j]==0){
                        inode_table[fileID].data_ptrs[j]= i;
                        write_blocks(1, 1, &inode_table);
                        break;
                    }
                }
                break;
            }
        }
        write_blocks(i, 1, &buf);
        
       
	
	return 0;
}

int sfs_fseek(int fileID, int loc){

    //Implement sfs_fseek here
    fd_table[fileID].rd_write_ptr = loc;
    return 0;
}

int sfs_remove(char *file) {

    int k= 0;
    while(strcmp(root_dir[k].name, file) != 0){
        k++;
    }
    if ( k >= MAX_FILES ){
        printf("file not found\n");
        return 0;
    }
    free_blocks[inode_table[root_dir[k].inode_idx].data_ptrs[0]] = 0;
    
    inode_table[root_dir[k].inode_idx].mode =  0;
    inode_table[root_dir[k].inode_idx].link_cnt = 0;
    inode_table[root_dir[k].inode_idx].uid = 0;
    inode_table[root_dir[k].inode_idx].gid = 0;
    inode_table[root_dir[k].inode_idx].size = 0;
    inode_table[root_dir[k].inode_idx].data_ptrs[0] = 0;
    
    root_dir[k].inode_idx = 0;
    strcpy(root_dir[k].name, "");
    
    return 0;
}
