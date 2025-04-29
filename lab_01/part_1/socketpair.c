#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

int main()
{
  const int num_childs = 1;

  const char *msg_p[3] = 
  {
    "AAA",
    "BBB",
    "CCC"
  };
  const char *msg_c[3] = 
  {
    "DDD",
    "EEE",
    "FFF"
  };

  int fdsock[2];

  char buf[5];
  buf[4] = '\0';
  pid_t pid[num_childs];

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, fdsock) == -1) 
  {
    perror("cant socketpair");
    exit(1);
  }

  for (int i = 0; i < num_childs; i++)
  {
    if ((pid[i] = fork()) == -1)
    {
      perror("cant fork");
      exit(1);
    }
    if (pid[i] == 0)
    {
      printf("child %d write: %s\n", getpid(), msg_p[i]);
      write(fdsock[1], msg_p[i], sizeof(msg_p[i]));
      read(fdsock[1], buf, sizeof(buf));
      printf("child %d recieve: %s\n", getpid(), buf);
      return 0;
    }
    else {
      read(fdsock[0], buf, sizeof(buf));
      printf("parent recieve: %s from child %d\n", buf, pid[i]);
      write(fdsock[0], msg_c[i], sizeof(msg_c[i]));
    printf("parent write: %s\n", msg_c[i]);
    }
  }

  return 0;
}