#include <stdio.h>
#include "apue.h"
#include <dirent.h>
#include <limits.h>

/* function type that is called for each filename */
typedef int Myfunc(const char *, const struct stat *, int);

static Myfunc myfunc;
static int myftw(char *, Myfunc *);  /* FTW: file walk tree */
static int dopath(Myfunc *);

static long nreg, ndir, nblk, nchr, nfifo, nslink, nsock, ntot, nbyte;
static char *pattern;	/* global variable for pattern */

int main(int argc, char *argv[])
{
  
  int ret;
  
  if (argc != 3)
    err_quit("usage: ftw <starting-pathname> <pattern>");
   
  pattern = argv[2];	/* Get the pattern from the command line */
  ret = myftw(argv[1], myfunc); /* does it all */
  
  ntot = nreg + ndir + nblk + nchr + nfifo + nslink + nsock;
  
  if (ntot == 0)
    ntot = 1; /* avoid divide by 0; print 0 for all counts */
  
  printf("Total number of files: %ld\n", nreg);
  printf("Total number of bytes: %ld\n", nbyte);
  
  exit(ret);
}

static char *fullpath;	/* contains full pathname for every file */
static size_t pathlen;

/* we return whatever func() returns */
static int myftw(char *pathname, Myfunc *func)
{
   /* malloc PATH_MAX+1 bytes */
  fullpath = path_alloc(&pathlen);
  
  if (pathlen <= strlen(pathname))
  {
    pathlen = strlen(pathname) * 2;
    
    if ((fullpath = realloc(fullpath, pathlen)) == NULL)
      err_sys("realloc failed");      
  }
  
  strcpy(fullpath, pathname);
  return(dopath(func));
}

/* Descend through the hierarchy, starting at "fullpath". 
 * If "fullpath" is anything other than a directory, we lstat() it,
 * call func(), and return. For a directory, we call ourself
 * recursively for each name in the directory. 
 */  
#define FTW_F 1 		/* file other than directory */
#define FTW_D 2 		/* directory */
#define FTW_DNR 3		/* directory that can't be read */
#define FTW_NS  4		/* file that we can't stat */

/* we return whatever func() returns */
static int dopath(Myfunc* func)
{
  struct stat statbuf;
  struct dirent *dirp;
  DIR *dp;
  int ret, n;

   
  if (lstat(fullpath, &statbuf) < 0) /* stat error */
    return(func(fullpath, &statbuf, FTW_NS));
   
  if (S_ISDIR(statbuf.st_mode) == 0) /* not a directory */
    return(func(fullpath, &statbuf, FTW_F));
   
  /*
	* It's directory. First call func() for the directory, 
	* than process each filename in the directory
	*/
  if ((ret = func(fullpath, &statbuf, FTW_D)) != 0)
    return(ret);
  
  n=strlen(fullpath);
  if (n + NAME_MAX + 2 > pathlen)	/* expand path buffer */
  {  
      pathlen *= 2;
      if ((fullpath = realloc(fullpath, pathlen)) == NULL)
      err_sys("realloc failed");
  }
  
  fullpath[n++] = '/';
  fullpath[n] = 0;
 
  if ((dp = opendir(fullpath)) == NULL)  /* can’t read directory */
    return(func(fullpath, &statbuf, FTW_DNR));
  
  while ((dirp = readdir(dp)) != NULL) 
  {
    if (strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0) continue; /* ignore dot and dot-dot */

    strcpy(&fullpath[n], dirp->d_name); /* append name after "/" */ 
    if ((ret = dopath(func)) != 0)    /* recursive */
      break;						 /* Time to leave */ 
  }
  
  fullpath[n-1] = 0; /* erase everything from slash onward */
  
  if (closedir(dp) < 0)
    err_ret("can’t close directory %s", fullpath);
  
  return(ret);
}

static int myfunc(const char *pathname, const struct stat *statptr, int type)  /* pass the dirp to this function; pass in the filename */
{ 
  switch (type) 
  {
  case FTW_F:
    switch (statptr->st_mode & S_IFMT) 
	{
      case S_IFREG: /* this represents the total number of files */
	  /* do all the function manipulation here */
        if (pattern != NULL && strstr(pathname, pattern) != NULL) 
		{
		  nreg++;
          nbyte += statptr->st_size;
		  printf("Match found: %s\n", pathname);
          printf("Size in bytes %lld\n", (long long) statptr->st_size);
        }
        break;
      case S_IFBLK: nblk++; break;
      case S_IFCHR: nchr++; break;
      case S_IFIFO: nfifo++; break;
      case S_IFLNK: nslink++; break;
      case S_IFSOCK: nsock++; break;
      /* directories should have type = FTW_D */
      case S_IFDIR: err_dump("for S_IFDIR for %s", pathname);
    }
    break;
    
  case FTW_D: 
  	ndir++; 
  	break;
  case FTW_DNR: err_ret("can’t read directory %s", pathname); 
  	break;
  case FTW_NS: err_ret("stat error for %s", pathname); 
  	break;
  default: 
  	err_dump("unknown type %d for pathname %s", type, pathname);
  }
  return(0);
}