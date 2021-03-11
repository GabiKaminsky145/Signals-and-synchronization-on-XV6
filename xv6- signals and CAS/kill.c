#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char **argv)
{
  // int i;

  if(argc < 3){
    printf(2, "usage: kill pid signum...\n");
    exit();
  }
  // for(i=1; i<argc; i=i+2){        // kill pid signum gives us argc = 3. if argc % 2 = 0 that means something is wrong 
  //   if ( (argc % 2 == 0)){
  //     printf(1, "wrong number of arguments\n");
  //     exit();
  //   }      
    kill(atoi(argv[1]), atoi(argv[2]));
    exit();
  }

