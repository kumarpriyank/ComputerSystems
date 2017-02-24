  /*
   * file:        homework.c
   * description: skeleton file for CS 5600/7600 file system
   *
   * CS 5600, Computer Systems, Northeastern CCIS
   * Peter Desnoyers, November 2016
   */

  #define FUSE_USE_VERSION 27

  #include <stdlib.h>
  #include <stddef.h>
  #include <unistd.h>
  #include <fuse.h>
  #include <fcntl.h>
  #include <string.h>
  #include <stdio.h>
  #include <errno.h>
  #include <sys/select.h>
  #include <time.h>
  #include "fsx600.h"
  #include "blkdev.h"


  extern int homework_part;       /* set by '-part n' command-line option */
  // Inodes List
  struct fs_inode *fs_inodes;
  
  // The initial block
  int size_of_sb = 1;
  int no_of_Inodes = 0;

  /* 
   * disk access - the global variable 'disk' points to a blkdev
   * structure which has been initialized to access the image file.
   *
   * NOTE - blkdev access is in terms of 1024-byte blocks
   */
  extern struct blkdev *disk;

  /* by defining bitmaps as 'fd_set' pointers, you can use existing
   * macros to handle them. 
   *   FD_ISSET(##, inode_map);
   *   FD_CLR(##, block_map);
   *   FD_SET(##, block_map);
   */
  fd_set *inode_map;              /* = malloc(sb.inode_map_size * FS_BLOCK_SIZE); */
  fd_set *block_map;

  /* init - this is called once by the FUSE framework at startup. Ignore
   * the 'conn' argument.
   * recommended actions:
   *   - read superblock
   *   - allocate memory, read bitmaps and inodes
   */
  void* fs_init(struct fuse_conn_info *conn)
  {
      // Reading the super block
      struct fs_super sb;
      if (disk->ops->read(disk, 0, 1, &sb) < 0)
          exit(1);

      // The number of blocks
      int block_size = sb.num_blocks;

      // Setting Inode Map Size
      no_of_Inodes = (sb.inode_region_sz *  FS_BLOCK_SIZE) / sizeof(struct fs_inode);

      // Allocate memory for the inode map and then read the inode map into it
      inode_map = (fd_set *)malloc(sb.inode_map_sz * FS_BLOCK_SIZE); 
      if (disk->ops->read(disk, size_of_sb, sb.inode_map_sz, inode_map) < 0)
          exit(1);

      // Allocate memory for the block map and then read the contents of the inode block into it
      block_map = (fd_set *)malloc(sb.block_map_sz * FS_BLOCK_SIZE);
      if (disk->ops->read(disk, size_of_sb + sb.inode_map_sz, sb.block_map_sz, block_map) < 0)
          exit(1);

     // Get the list of the inodes that are there so that we can read the list and load the respective blocks 
      fs_inodes = (struct fs_inode *)malloc(sb.inode_region_sz * FS_BLOCK_SIZE);
      if (disk->ops->read(disk, size_of_sb + sb.inode_map_sz + sb.block_map_sz, sb.inode_region_sz, fs_inodes) < 0)
          exit(1);
      
      return NULL;
  }

  /* Note on path translation errors:
   * In addition to the method-specific errors listed below, almost
   * every method can return one of the following errors if it fails to
   * locate a file or directory corresponding to a specified path.
   *
   * ENOENT - a component of the path is not present.
   * ENOTDIR - an intermediate component of the path (e.g. 'b' in
   *           /a/b/c) is not a directory
   */
  int translate(const char *path, int inum) {

    struct fs_dirent *dirents = NULL;
    char *_path = strdup(path);
    char* token = strtok(_path, "/");
    if (!strcmp(_path,"/"))
        token = "/";

    while(token!=NULL){
  	
      // Get the token and check if it is a directory
      if(S_ISDIR(fs_inodes[inum].mode)) {
        
        // Root node do break
        if(!strcmp(token,"/"))
           break;
    
        // Allocate the directory entries
        dirents = (struct fs_dirent*)malloc(FS_BLOCK_SIZE);
        if(disk->ops->read(disk, fs_inodes[inum].direct[0], 1, dirents) < 0)
           exit(1);
       // Iterating over the directory entries to verify that the name matches and the entry is valid
       int y=0;
       for(y=0;y<32;y++) {
         // If a valid entry is found and the names match update the inode pointer and break
         if(dirents[y].valid && !strcmp(dirents[y].name, token)) {
           inum = dirents[y].inode;
           break;
          }
       }
       // If the entry is not present in the directoy return error
       if(y==32)
          return -ENOENT;
       // Get another token from the path
       token = strtok(NULL, "/");
     }
     else
         return -ENOTDIR;
    }
    free(_path);
    return inum;
  }

  /* getattr - get file or directory attributes. For a description of
   *  the fields in 'struct stat', see 'man lstat'.
   *
   * Note - fields not provided in fsx600 are:
   *    st_nlink - always set to 1
   *    st_atime, st_ctime - set to same value as st_mtime
   *
   * errors - path translation, ENOENT
   */
  static int fs_getattr(const char *path, struct stat *sb)
  {
      int inum = translate(path, 1);

      if (inum < 0) {
          return inum;
      } 
      else {  

         sb->st_dev = 0;
         sb->st_rdev = 0;
         int b = fs_inodes[inum].size / FS_BLOCK_SIZE;
         if(!((fs_inodes[inum].size % FS_BLOCK_SIZE) == 0))
            b++;
         sb->st_blocks =b;
         sb->st_ino = inum;
         sb->st_mode = fs_inodes[inum].mode;
         sb->st_uid = fs_inodes[inum].uid;
         sb->st_gid = fs_inodes[inum].gid;
         sb->st_atime = fs_inodes[inum].mtime;
         sb->st_ctime = fs_inodes[inum].ctime;
         sb->st_mtime = fs_inodes[inum].mtime;
         sb->st_size = fs_inodes[inum].size;
         sb->st_blksize = FS_BLOCK_SIZE;
         sb->st_nlink = 1;
         return 0;
     }
  }

  /* readdir - get directory contents.
   *
   * for each entry in the directory, invoke the 'filler' function,
   * which is passed as a function pointer, as follows:
   *     filler(buf, <name>, <statbuf>, 0)
   * where <statbuf> is a struct stat, just like in getattr.
   *
   * Errors - path resolution, ENOTDIR, ENOENT
   */
   static int fs_readdir(const char *path, void *ptr, fuse_fill_dir_t filler,
     off_t offset, struct fuse_file_info *fi){

      char *_path = strdup(path);
      // Call to the transalate path is made here
      int inum = translate(path, 1);

      if(inum < 0)
          return inum; 
      else { 
      	 

      	  if(!S_ISDIR(fs_inodes[inum].mode))
            return -ENOTDIR; 
    	   
          struct fs_dirent *dirents = (struct fs_dirent*)malloc(FS_BLOCK_SIZE);
          if(disk->ops->read(disk, fs_inodes[inum].direct[0], 1, dirents) < 0)
              exit(1);

          int y=0;
          for(y=0;y<32;y++) {
            struct stat sb;
            if(dirents[y].valid){
              
  	     char * new_str ;
               if(strcmp(path,"/")) {
                  new_str = malloc(strlen(path)+strlen("/")+strlen(dirents[y].name)+1);
                  new_str[0] = '\0';
                  strcat(new_str, path);
                  strcat(new_str,"/");
                  strcat(new_str, dirents[y].name);
               } else {
                  new_str = malloc(strlen(path)+strlen(dirents[y].name)+1);
                  new_str[0] = '\0';
                  strcat(new_str, path);
                  strcat(new_str, dirents[y].name);
               }
              fs_getattr(new_str, &sb);
              filler(ptr, dirents[y].name, &sb, 0);
              free(new_str);
            }          
          }
      }
      return 0;
  }


  /* see description of Part 2. In particular, you can save information 
   * in fi->fh. If you allocate memory, free it in fs_releasedir.
   */

  static int fs_opendir(const char *path, struct fuse_file_info *fi)
  {
      return 0;
  }

  static int fs_releasedir(const char *path, struct fuse_file_info *fi)
  {
      return 0;
  }


  /*
   *   Gets the empty inode number from the inode map
   */
  static int get_inode_from_map() {
    int inodeNum=1;
    while(inodeNum < no_of_Inodes){
      inodeNum++;
      if(!FD_ISSET(inodeNum, inode_map))
        break;
      }
    return inodeNum;
  }

  /*
   *   Gets the empty block number from the block map
   */
  static int get_block_from_map() {
    int blockNum = 0;
    while(1){
      blockNum++;
      if(!FD_ISSET(blockNum, block_map))
        break;
    }
    return blockNum;
  }

  // helper function that return the parent path of the given path
  char* getParentPath(char *_path) {

   int lastIndex = 0;
   int x=0;
  
   for(x=strlen(_path)-1; x>=0;x--) {
      
     if(_path[x] == '/'){
       lastIndex = x;
       break;
     }
   }
  
   char *parentPath;

   if(lastIndex == 0)
     parentPath = "/";
   else{
     parentPath = malloc(lastIndex  + 1);
     int z=0;
     for (z=0;z<lastIndex;z++) {
      parentPath[z] = _path[z];
     }
     //memcpy(parentPath, _path, lastIndex);
     parentPath[lastIndex] = '\0';
     //parentPath[lastIndex+1] = '\0';
   }
      return parentPath;
  }

  // helper function that return the dir or file name  of the given path
  char* getNameFromPath(char *_path) {
  	
      char* token = strtok(_path, "/");
      char* str[FS_BLOCK_SIZE] = {};
      int i=0;
      while(token != NULL){
          str[i] = token;
          i++;
          token = strtok(NULL, "/");
      }
      char* dirName = str[i-1];
      return dirName;
       
  }

  /* mknod - create a new file with permissions (mode & 01777)
   *
   * Errors - path resolution, EEXIST
   *          in particular, for mknod("/a/b/c") to succeed,
   *          "/a/b" must exist, and "/a/b/c" must not.
   *
   * If a file or directory of this name already exists, return -EEXIST.
   * If this would result in >32 entries in a directory, return -ENOSPC
   * if !S_ISREG(mode) return -EINVAL [i.e. 'mode' specifies a device special
   * file or other non-file object]
   */
  static int fs_mknod(const char *path, mode_t mode, dev_t dev)
  {
    // Have to copy twice, one copy to feed getParentPath, 
    // the other copy for getNameFromPath
    char* _path = strdup(path);
    char* _path1 = strdup(path);
      
    // If the path is a / return Already exist
    if (!strcmp(_path, "/")) {
      return -EEXIST;
    }
      
    // If the mode is the ISREG
    if(!S_ISREG(mode)) {
      return -EINVAL;
    }

    char* parentPath = getParentPath(_path);
    char* dirName = getNameFromPath(_path1);
   
    // Check if the parent exists
    int inum = translate(parentPath,1);

    if(inum < 0)
      return inum;
      
    // Check if the file exists
    int inumFile = translate(path,1);

    if(inumFile > 0) 
      return -EEXIST;
      
    // Get the inode number for the current file from the inode map
    int inodeNum = get_inode_from_map();

    // Get the block number for the current file from the block map
    int blockNum = get_block_from_map();

    if(inodeNum==0 || blockNum==0) {
      return -ENOSPC;
    }

    // Getting the super block 
    struct fs_super sb;
    if (disk->ops->read(disk, 0, 1, &sb) < 0)
      exit(1);

    // Getting the dirents of paremts and checking for more than 32 entries in it
    struct fs_dirent *dirents = (struct fs_dirent*)malloc(FS_BLOCK_SIZE);
    if(disk->ops->read(disk, fs_inodes[inum].direct[0], 1, dirents) < 0) {
      exit(1);
    }

    int m=0;

    for(m=0;m<32;m++){
         
      // BReaking on any invalid entry found as that can be used
      if(!dirents[m].valid) {
        
        // Setting the inode in the inode map
        FD_SET(inodeNum, inode_map);
           
        // Sets  the block bitmap bit
        FD_SET(blockNum, block_map);

        dirents[m].valid = 1;
        dirents[m].isDir = 0;
        dirents[m].inode = inodeNum;
        strcpy(dirents[m].name, dirName);

        // Writing the inode map to the disk
        if (disk->ops->write(disk, size_of_sb, sb.inode_map_sz, inode_map)<0){
          exit(1);
        }
        break;
      }
    }

    // If no invalid entry is found then return no space error
    if(m == 32)
      return -ENOSPC;

    // Write the parent directory entries to the disk
    if(disk->ops->write(disk, fs_inodes[inum].direct[0], 1, dirents) < 0){
      exit(1);
    }

    // Write the block to the disk
    if (disk->ops->write(disk, size_of_sb + sb.inode_map_sz, sb.block_map_sz, block_map) < 0){
      exit(1);
    }

    // Get the userid and group id  from the fuse context 
    struct fuse_context *ctx = fuse_get_context();
    time_t ltime; /* calendar time */
    ltime=time(NULL); /* get current cal time */

    // Get the current time stamp
    fs_inodes[inodeNum].uid =ctx->uid;
    fs_inodes[inodeNum].gid = ctx->gid;
    fs_inodes[inodeNum].mode =  S_IFDIR | mode;
    fs_inodes[inodeNum].ctime = ltime;
    fs_inodes[inodeNum].mtime = ltime;
    fs_inodes[inodeNum].size = 0;   //FS_BLOCK_SIZE;

    // Point the inode direct block to the block found
    fs_inodes[inodeNum].direct[0] = blockNum;
    fs_inodes[inodeNum].indir_1 = 0;
    fs_inodes[inodeNum].indir_2 = 0;
    fs_inodes[inodeNum].pad[3] = 0;

    // Write the data to the specified blocks 
    if (disk->ops->write(disk, size_of_sb + sb.inode_map_sz + sb.block_map_sz, sb.inode_region_sz, fs_inodes) < 0)
      exit(1);
   
    return 0;
  }


  /* mkdir - create a directory with the given mode.
   * Errors - path resolution, EEXIST
   * Conditions for EEXIST are the same as for create. 
   * If this would result in >32 entries in a directory, return -ENOSPC
   *
   * Note that you may want to combine the logic of fs_mknod and
   * fs_mkdir. 
   */ 
  static int fs_mkdir(const char *path, mode_t mode)
  {
    // Have to copy twice, one copy to feed getParentPath, 
    // the other copy for getNameFromPath
    char* _path = strdup(path);
    char* _path1 = strdup(path);
      
    if (!strcmp(_path,"/"))
      return -EEXIST;

    if(translate(path,1)> 0)
      return -EEXIST;

    char* dirName = getNameFromPath(_path1);
    char* parentPath = getParentPath(_path);

    int inum = translate(parentPath,1);

      if(inum < 0)
         return inum;

      // Reading the super block
      struct fs_super sb;
      if (disk->ops->read(disk, 0, 1, &sb) < 0)
          exit(1);

      // Get the free inode number from inode map
      int inodeNum = get_inode_from_map();
      
      // Set the inode  map
      FD_SET(inodeNum, inode_map);

      // Get the free block number from the block bitmap
      int blockNum = get_block_from_map();
      
      // Sets  the block bitmap bit
      FD_SET(blockNum, block_map);

      // Get the userid and group id  from the fuse context 
      struct fuse_context *ctx = fuse_get_context();
      time_t ltime; // calendar time 
      ltime=time(NULL); // get current cal time 

      // Get the current time stamp
      fs_inodes[inodeNum].uid =ctx->uid;
      fs_inodes[inodeNum].gid = ctx->gid;
      fs_inodes[inodeNum].mode =  S_IFDIR | mode;
      fs_inodes[inodeNum].ctime = ltime;
      fs_inodes[inodeNum].mtime = ltime;
      fs_inodes[inodeNum].size = 0;
      
      // Point the inode direct block to the block found
      fs_inodes[inodeNum].direct[0] = blockNum;
      fs_inodes[inodeNum].indir_1 = 0;
      fs_inodes[inodeNum].indir_2 = 0;
      fs_inodes[inodeNum].pad[3] = 0;
      
      // Creating a directory entry in parent and writing it to the data base
      struct fs_dirent *dirents = (struct fs_dirent*)malloc(FS_BLOCK_SIZE);
      if(disk->ops->read(disk, fs_inodes[inum].direct[0], 1, dirents) < 0)
          exit(1);

      int y = 0;
      for(y=0;y<32;y++) {
        struct stat sb;
        if(!dirents[y].valid){
          dirents[y].valid = 1;
          dirents[y].isDir = 1;
          dirents[y].inode = inodeNum;
          strcpy(dirents[y].name, dirName);
          //strcpy(dirents[y].name, str[i-1]);
          break;
          }
      }
      
      // Write the inode bitmap to the disk
      if (disk->ops->write(disk, size_of_sb, sb.inode_map_sz, inode_map)<0)
          exit(1);

      // Write the block to the disk
      if (disk->ops->write(disk, size_of_sb + sb.inode_map_sz, sb.block_map_sz, block_map) < 0)
          exit(1);
      
      // Write the data to the specified blocks 
      if (disk->ops->write(disk, size_of_sb + sb.inode_map_sz + sb.block_map_sz, sb.inode_region_sz, fs_inodes) < 0)
          exit(1);

      // Write the contents to the disk
      if(disk->ops->write(disk, fs_inodes[inum].direct[0], 1, dirents) < 0)
          exit(1); 

      return 0;
  }

  /* truncate - truncate file to exactly 'len' bytes
   * Errors - path resolution, ENOENT, EISDIR, EINVAL
   *    return EINVAL if len > 0.
   */
  static int fs_truncate(const char *path, off_t len)
  {
     if (len != 0)
          return -EINVAL;               /* invalid argument */

     int inum = translate(path, 1);

     // Inum not  valid
     if(inum < 0)
       return inum;

     // If it is a directory cannot be truncated
     if(S_ISDIR(fs_inodes[inum].mode)) {
       return -EISDIR;
     }

     // Get the inode size i.e. the size of the file
     int fileSize = fs_inodes[inum].size;

      // Reading the super block
     struct fs_super sb;
     if (disk->ops->read(disk, 0, 1, &sb) < 0)
       exit(1);

     int noOfBlocks = fileSize / FS_BLOCK_SIZE;

     if(!(fileSize % FS_BLOCK_SIZE == 0))
       noOfBlocks++;

     int blcCountCpy = noOfBlocks;

     char* emptyBuff = malloc(FS_BLOCK_SIZE);
     memset(emptyBuff, '\0', FS_BLOCK_SIZE);

     while(noOfBlocks > 0) {

       off_t offset = 0;
       int physicalBlock  = offset_to_block(inum, offset);

       if(disk->ops->write(disk, physicalBlock, 1, emptyBuff)< 0)
         exit(1);

       offset +=FS_BLOCK_SIZE;

       noOfBlocks--;

       FD_CLR(physicalBlock, block_map);
     }

       // Write the block map to the disk
       if (disk->ops->write(disk, size_of_sb + sb.inode_map_sz, sb.block_map_sz, block_map) < 0)
           exit(1);

     time_t ltime; /* calendar time */
     ltime=time(NULL); /* get current cal time */

     fs_inodes[inum].ctime = ltime;
     fs_inodes[inum].mtime = ltime;
     fs_inodes[inum].size = 0;

     // Write the inodes list to the disk again
     // Write the data to the specified blocks
     if (disk->ops->write(disk, size_of_sb + sb.inode_map_sz + sb.block_map_sz, sb.inode_region_sz, fs_inodes) < 0)
         exit(1);

     return 0;
  }

  /* unlink - delete a file
   *  Errors - path resolution, ENOENT, EISDIR
   * Note that you have to delete (i.e. truncate) all the data.
   */
  static int fs_unlink(const char *path)
  {
     // Validate the path
     int inum = translate(path, 1);

     // If invalid path return error
     if(inum < 0)
       return inum;

     // Truncate the file and if its truncated well
     int isTruncated = fs_truncate(path, 0);

     // Error while truncating
     if(isTruncated < 0)
       return isTruncated;
	
     // Reading the super block
     struct fs_super sb;
     if (disk->ops->read(disk, 0, 1, &sb) < 0)
         exit(1);

     // Set the inode map to 0 i.e. clear the inode map
     if(FD_ISSET(inum,inode_map))
       FD_CLR(inum, inode_map);

     if (disk->ops->write(disk, size_of_sb, sb.inode_map_sz, inode_map)<0)
         exit(1);

     char* _path = strdup(path);
     char* parentPath = getParentPath(_path);

     // Get Parent Path
     int parentInum = translate(parentPath, 1);

     // Unnecessary but still
     if(parentInum < 0)
       return parentInum;
	 
     char *fileName = getNameFromPath(_path);

     struct fs_dirent *dirents = (struct fs_dirent*)malloc(FS_BLOCK_SIZE);
     if(disk->ops->read(disk, fs_inodes[parentInum].direct[0], 1, dirents) < 0)
         exit(1);

     int y = 0;
     for(y=0;y<32;y++) {
       if(dirents[y].valid && !strcmp(dirents[y].name,fileName)){
         dirents[y].valid = 0;
         break;
         }
     }

     // Write the contents to the disk
     if(disk->ops->write(disk, fs_inodes[parentInum].direct[0], 1, dirents) < 0)
         exit(1);

      // Get the userid and group id  from the fuse context
     struct fuse_context *ctx = fuse_get_context();
     time_t ltime; /* calendar time */
     ltime=time(NULL); /* get current cal time */

     fs_inodes[inum].mtime = ltime;

     // Write the inodes list to the disk again
     // Write the data to the specified blocks
     if (disk->ops->write(disk, size_of_sb + sb.inode_map_sz + sb.block_map_sz, sb.inode_region_sz, fs_inodes) < 0)
         exit(1);

     return 0;
  }

  /* rmdir - remove a directory
   *  Errors - path resolution, ENOENT, ENOTDIR, ENOTEMPTY
   */
  static int fs_rmdir(const char *path)
  {
    int inum = translate(path,1);

    if(inum < 0)
      return inum;

    // DIRECTORY CHECK 
    if(!S_ISDIR(fs_inodes[inum].mode))
            return -ENOTDIR; 


      // Check if it is directory and it is empty
      struct fs_dirent *dirents = (struct fs_dirent*)malloc(FS_BLOCK_SIZE);
      if(disk->ops->read(disk, fs_inodes[inum].direct[0], 1, dirents) < 0)
          exit(1);

      int y = 0;
      for(y=0;y<32;y++) {
        if(dirents[y].valid)
          break;
      }

      if(y < 32){
        return -ENOTEMPTY;
      }

      // Update the inode map
      FD_CLR(inum, inode_map);

      // Get the block 
      int blockNumber = fs_inodes[inum].direct[0];

      // Update the block map
      FD_CLR(blockNumber, block_map);
      
      // Reading the super block
      struct fs_super sb;
      if (disk->ops->read(disk, 0, 1, &sb) < 0)
          exit(1);

      // Write the block map to the disk
      if (disk->ops->write(disk, size_of_sb + sb.inode_map_sz, sb.block_map_sz, block_map) < 0)
          exit(1);

      // Write the inode bitmap to the disk
      if (disk->ops->write(disk, size_of_sb, sb.inode_map_sz, inode_map)<0)
          exit(1);
      
     char* _path = strdup(path); 
     char* token = strtok(_path, "/");
     char *parentPath;
     char* str[FS_BLOCK_SIZE] = {};
     int i=0;

     while(token!=NULL){
       str[i] = token;
       i++;
       token = strtok(NULL, "/");
     }
    
     parentPath = malloc(FS_BLOCK_SIZE);
     parentPath[0] = '\0';
     int j=0;
     
     if(i==1)
       strcat(parentPath, "/");

     for(j=0; j<i-1; j++) {
       strcat(parentPath, "/");
       strcat(parentPath, str[j]);
     }
      
     char* dirName = str[i-1];

     int inumParent = translate(parentPath,1);
     
     if(inumParent < 0)
        return inumParent;
     
     // Creating a directory entry in parent and writing it to the data base
     struct fs_dirent *direntsParent = (struct fs_dirent*)malloc(FS_BLOCK_SIZE);
     if(disk->ops->read(disk, fs_inodes[inumParent].direct[0], 1, direntsParent) < 0)
        exit(1);
     
     int y1=0;
     for(y1=0;y1<32;y1++) {
        if(!strcmp(direntsParent[y1].name,str[i-1])){
          direntsParent[y1].valid = 0;
          break;
          }
      }

      // Write the contents to the disk
      if(disk->ops->write(disk, fs_inodes[inumParent].direct[0], 1, direntsParent) < 0)
          exit(1);

      return 0;
  }

/* rename - rename a file or directory
 * Errors - path resolution, ENOENT, EINVAL, EEXIST
 *
 * ENOENT - source does not exist
 * EEXIST - destination already exists
 * EINVAL - source and destination are not in the same directory
 *
 * Note that this is a simplified version of the UNIX rename
 * functionality - see 'man 2 rename' for full semantics. In
 * particular, the full version can move across directories, replace a
 * destination file, and replace an empty directory with a full one.
 */
  static int fs_rename(const char *src_path, const char *dst_path)
  {
    // Have to copy twice, one copy to feed getParentPath, 
    // the other copy for getNameFromPath
    char *_src_path = strdup(src_path);
    char *_dst_path = strdup(dst_path);
    char *_src_path1 = strdup(src_path);
    char *_dst_path1 = strdup(dst_path);

    int src_inum = translate(_src_path,1);
    int dst_inum = translate(_dst_path,1);

    // Check for the path to be correct
    if(src_inum < 0)
        return -ENOENT;
    
    // Renaming same file do nothing
    if(src_inum == dst_inum)
      return 0;

    // Check for the path already existance
    if(dst_inum > 0) 
        return -EEXIST; 

    char *src_node_name = getNameFromPath(_src_path);
    char *dst_node_name = getNameFromPath(_dst_path);

    char *parent_dst = getParentPath(_dst_path1);
    char *parent_src = getParentPath(_src_path1);

    int par_src_inum = translate(parent_src,1);
    int par_dst_inum = translate(parent_dst,1);


    if(par_src_inum != par_dst_inum) {
        return -EINVAL;
    }

    // Reading the super block
    struct fs_super sb;
    if (disk->ops->read(disk, 0, 1, &sb) < 0)
      exit(1);

    
    struct fs_dirent *dirents = (struct fs_dirent*)malloc(FS_BLOCK_SIZE);
    if(disk->ops->read(disk, fs_inodes[par_src_inum].direct[0], 1, dirents) < 0)
      exit(1);

    // set name
    int y=0;
    for(y=0;y<32;y++) {
      
      if(!strcmp(dirents[y].name, src_node_name) && dirents[y].valid){
        strcpy(dirents[y].name, dst_node_name);
        break;
        }
    }
    
       // Write the contents to the disk
    if(disk->ops->write(disk, fs_inodes[par_src_inum].direct[0], 1, dirents) < 0)
      exit(1);


    time_t ltime; /* calendar time */
    ltime=time(NULL); /* get current cal time */
    fs_inodes[src_inum].mtime = ltime;
    fs_inodes[src_inum].ctime = ltime;
 

    // Write the data to the specified blocks
    if (disk->ops->write(disk, size_of_sb + sb.inode_map_sz + sb.block_map_sz, sb.inode_region_sz, fs_inodes) < 0)
        exit(1);

    return 0;
  }


  /* chmod - change file permissions
   * utime - change access and modification times
   *         (for definition of 'struct utimebuf', see 'man utime')
   *
   * Errors - path resolution, ENOENT.
   */
  static int fs_chmod(const char *path, mode_t mode)
   {

     // Reading the super block
     struct fs_super sb;
     if (disk->ops->read(disk, 0, 1, &sb) < 0)
         exit(1);

     // Check for the path to be correct
     int inum = translate(path,1);

     // Check if the path exists
     if(inum < 0 )
       return inum;

     // Get the userid and group id  from the fuse context
     struct fuse_context *ctx = fuse_get_context();
     time_t ltime;  /* calendar time */
     ltime=time(NULL);  /* get current cal time */

     // Update the inodes details like mode - permissions etc. And update time
     if(S_ISDIR(fs_inodes[inum].mode)){
       fs_inodes[inum].mode =  S_IFDIR | mode;
     }
     else
       fs_inodes[inum].mode =  S_IFREG | mode;

     fs_inodes[inum].ctime = ltime;

     // Write the data to the specified blocks
     if (disk->ops->write(disk, size_of_sb + sb.inode_map_sz + sb.block_map_sz, sb.inode_region_sz, fs_inodes) < 0)
         exit(1);

     return 0;
   }

  int fs_utime(const char *path, struct utimbuf *ut)
  {
    // Reading the super block
    struct fs_super sb;
    if (disk->ops->read(disk, 0, 1, &sb) < 0)
        exit(1);

    // Check for the path to be correct
    int inum = translate(path,1);

    // Check if the path exists 
    if(inum < 0 )
      return inum;
    
    fs_inodes[inum].ctime = ut->actime;
    fs_inodes[inum].mtime = ut->modtime;

    // Write the data to the specified blocks 
    if (disk->ops->write(disk, size_of_sb + sb.inode_map_sz + sb.block_map_sz, sb.inode_region_sz, fs_inodes) < 0)
        exit(1);
    
    return 0;
  }

 
 /*
  * translate from a block offset to a to physical block number
  */
  int offset_to_block(int inum, int blk_offset) {
      int block = 0;

      if(blk_offset < 6) { // direct pointer
          block = fs_inodes[inum].direct[blk_offset];

      } else if (blk_offset < 256 + 6) { // single indirect 
          blk_offset -= 6;
          int indir1[256] = {0};
          if(disk->ops->read(disk, fs_inodes[inum].indir_1, 1, indir1) < 0) {
              exit(1);
          }
          block = indir1[blk_offset];

      } else {
          blk_offset = blk_offset - 6 -256;
          int indir1[256] = {0};
          int indir2[256] = {0};
          if(disk->ops->read(disk, fs_inodes[inum].indir_2, 1, indir2) < 0) {
                  exit(1);
          }
          block = indir2[blk_offset/256];

          if(disk->ops->read(disk, block, 1, indir1) < 0) {
            exit(1);
          }
          block = indir1[blk_offset%256];
      } 
      return block;
  }

  /* read - read data from an open file.
   * should return exactly the number of bytes requested, except:
   *   - if offset >= file len, return 0
   *   - if offset+len > file len, return bytes from offset to EOF
   *   - on error, return <0
   * Errors - path resolution, ENOENT, EISDIR
   */
  static int fs_read(const char *path, char *buf, size_t len, off_t offset,
              struct fuse_file_info *fi)
  {
    //printf("%i\n", len);
    int inum = translate(path, 1);

    if(inum < 0) {
        return inum;
    }
    // CHECK ONCE MORE
    // struct fs_dirent *dirents = (struct fs_dirent*)malloc(FS_BLOCK_SIZE);
    // if(disk->ops->read(disk, fs_inodes[inum].direct[0], 1, dirents) < 0)
    //     exit(1);

    if(S_ISDIR(fs_inodes[inum].mode)) {
        return -EISDIR;
    } 

    if(offset >= fs_inodes[inum].size) {
        return 0;
    }

    if(offset+len > fs_inodes[inum].size) {
        len = fs_inodes[inum].size - offset;
    }

    int startByte = offset % FS_BLOCK_SIZE;
    int startDirect = offset / FS_BLOCK_SIZE;
    int bytes = FS_BLOCK_SIZE - startByte;
    char local_buf[FS_BLOCK_SIZE] = {'\0'};
    int i=0;
    int k=0;

    int startBlock = offset_to_block(inum, startDirect);
    if(disk->ops->read(disk, startBlock, 1, local_buf) < 0) {
        exit(1);
    }

    while(i < len) {
      buf[i] = local_buf[startByte];
      i++;
      startByte++;

      if((startByte%FS_BLOCK_SIZE) == 0) {
          startDirect++;
          for(k=0; k<FS_BLOCK_SIZE; k++) {
              local_buf[k] ='\0';
          }
          startBlock = offset_to_block(inum, startDirect);
          if(disk->ops->read(disk, startBlock, 1, local_buf) < 0) {
              exit(1);
          }
          startByte=0;
      }
    }

    return len;
  }

  // helper function that updates dir/indir start block number 
  void updateInodeBlocks(int inum, int blk_offset, int startBlock) 
  {
    if (blk_offset < 6) { // direct
      fs_inodes[inum].direct[blk_offset] = startBlock;
    }
    else if (blk_offset-6<256) { // indir_1
      blk_offset -= 6;
      int* indir1 = malloc(FS_BLOCK_SIZE);
      disk->ops->read(disk, fs_inodes[inum].indir_1, 1, indir1);
      indir1[blk_offset] = startBlock;
      disk->ops->write(disk, fs_inodes[inum].indir_1, 1, indir1);
      free(indir1);
    } else { // indir_2
      blk_offset = blk_offset - 256 - 6;
      int indir_ptr[256];
      int index;
      disk->ops->read(disk, fs_inodes[inum].indir_2, 1, indir_ptr);
      index = indir_ptr[blk_offset / 256];
      disk->ops->read(disk, index, 1, indir_ptr);
      indir_ptr[blk_offset % 256] = startBlock;
      disk->ops->write(disk, index, 1, indir_ptr);     
    }
  }

  /* write - write data to a file
   * It should return exactly the number of bytes requested, except on
   * error.
   * Errors - path resolution, ENOENT, EISDIR
   *  return EINVAL if 'offset' is greater than current file length.
   *  (POSIX semantics support the creation of files with "holes" in them, 
   *   but we don't)
   */
  static int fs_write(const char *path, const char *buf, size_t len,
           off_t offset, struct fuse_file_info *fi)
  {
    int inum = translate(path,1);

    struct fs_super sb;
    if (disk->ops->read(disk, 0, 1, &sb) < 0){
      exit(1);
    }

    if(inum < 0) {
        return inum;
    }

    if(S_ISDIR(fs_inodes[inum].mode)) {
        return -EISDIR;
    }

    if(offset > fs_inodes[inum].size) {
      return -EINVAL;
    }

    char* data_block[FS_BLOCK_SIZE];
    int startByte = offset % FS_BLOCK_SIZE;
    int startDirect = offset / FS_BLOCK_SIZE;
    int bytes = FS_BLOCK_SIZE - startByte;
    int startBlock = offset_to_block(inum, startDirect);
    int result = 0;
    int curPos = offset;

    if (bytes > len) {
      bytes = len;
    }

    // write to block in the same level
    if (startByte !=  0) { 
        startBlock = offset_to_block(inum, startDirect);
        disk->ops->read(disk, startBlock, 1, data_block);
        memcpy(data_block + startByte, buf, bytes);
        disk->ops->write(disk, startBlock, 1, data_block);

        buf += bytes;
        len -= bytes;
        curPos += bytes;
        startDirect++;
        result += bytes;
    }
  
    // setting the starting block for every new level base on the file size
    while (len) {
      bytes = len;

      if (curPos < fs_inodes[inum].size) { // direct 
        startBlock = offset_to_block(inum, startDirect);
        memcpy(data_block, buf, bytes);
        disk->ops->write(disk, startBlock, 1, data_block);
      } else { 
        if (startDirect==6) { // indir_1
          int indir1 = get_block_from_map();
          if(indir1 != 0) {
            FD_SET(indir1, block_map);
            fs_inodes[inum].indir_1 = indir1;
          } else {
            return -ENOSPC;
          }
        } else if (startDirect-6==256) { // indir_2
          int indir2 = get_block_from_map();
          if(indir2 != 0) {
            FD_SET(indir2, block_map);
            fs_inodes[inum].indir_2 = indir2;
          } else {
            return -ENOSPC;
          }
        }
            
        if (startDirect-6>=256 && (startDirect-256-6)%256==0) {
          int empty = get_block_from_map();
          if(empty != 0) {
            FD_SET(empty, block_map);
            int indir_ptr[256];
            if(disk->ops->read(disk, fs_inodes[inum].indir_2, 1, indir_ptr)<0){
              exit(1);
            }

            indir_ptr[(startDirect - 312) / 256] = empty;
            if(disk->ops->write(disk, fs_inodes[inum].indir_2, 1, indir_ptr)<0){
              exit(1);
            }
          } else {
            return -ENOSPC;
          }
        }

        // actually adding it to the inodes and writing the the block
        startBlock = get_block_from_map();
        if(startBlock != 0) {
          FD_SET(startBlock, block_map);
          if(disk->ops->write(disk, 1 + sb.inode_map_sz, sb.block_map_sz, block_map)<0){
            exit(1);
          }
          updateInodeBlocks(inum, startDirect, startBlock);

        } else {
          return -ENOSPC;
        }   

        memcpy(data_block, buf, bytes);
        if(disk->ops->write(disk, 1 + sb.inode_map_sz + sb.block_map_sz, sb.inode_region_sz, fs_inodes)<0){
          exit(1);
        }
      }
            
      buf += bytes;
      len -= bytes;
      curPos += bytes;
      startDirect++;
      result += bytes;
    }


    if (curPos > fs_inodes[inum].size) {
      fs_inodes[inum].size = curPos;
      disk->ops->write(disk, size_of_sb+sb.inode_map_sz+sb.block_map_sz, sb.inode_region_sz, fs_inodes);
    }

  return result;
  }

  static int fs_open(const char *path, struct fuse_file_info *fi)
  {
      return 0;
  }

  static int fs_release(const char *path, struct fuse_file_info *fi)
  {
      return 0;
  }

  /* statfs - get file system statistics
   * see 'man 2 statfs' for description of 'struct statvfs'.
   * Errors - none. 
   */
  static int fs_statfs(const char *path, struct statvfs *st)
  {
      /* needs to return the following fields (set others to zero):
       *   f_bsize = BLOCK_SIZE
       *   f_blocks = total image - metadata
       *   f_bfree = f_blocks - blocks used
       *   f_bavail = f_bfree
       *   f_namelen = <whatever your max namelength is>
       *
       * this should work fine, but you may want to add code to
       * calculate the correct values later.
       */
      st->f_bsize = FS_BLOCK_SIZE;
      st->f_blocks = 0;           /* probably want to */
      st->f_bfree = 0;            /* change these */
      st->f_bavail = 0;           /* values */
      st->f_namemax = 27;

      return 0;
  }


  /* operations vector. Please don't rename it, as the skeleton code in
   * misc.c assumes it is named 'fs_ops'.
   */
  struct fuse_operations fs_ops = {
      .init = fs_init,
      .getattr = fs_getattr,
      .opendir = fs_opendir,
      .readdir = fs_readdir,
      .releasedir = fs_releasedir,
      .mknod = fs_mknod,
      .mkdir = fs_mkdir,
      .unlink = fs_unlink,
      .rmdir = fs_rmdir,
      .rename = fs_rename,
      .chmod = fs_chmod,
      .utime = fs_utime,
      .truncate = fs_truncate,
      .open = fs_open,
      .read = fs_read,
      .write = fs_write,
      .release = fs_release,
      .statfs = fs_statfs,
  };

