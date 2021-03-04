#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/*
A signal is a software generated interrupt that is sent to a process by
the OS because of when user press ctrl-c (SIGINT) or another process tell something to this process.
There are fix set of signals that can be sent to a process. signal are identified by integers.
Signal number have symbolic names. For example SIGCHLD is number of the signal sent to the parent process when child terminates.
*/

/*
A SIGTERM jel egy általános jel, amelyet a program lezárásához vezetnek. 
A SIGKILL-től eltérően ez a jel (SIGTERM) blokkolható, kezelhető és figyelmen kívül hagyható. 
A program megszüntetésének udvarias kérése, módja.
SIGINT - Interrupt the process
*/

void handler(int signumber)
{
  printf("Signal with number %i has arrived\n", signumber);
}

int main()
{

  signal(SIGTERM, handler); //handler = SIG_IGN - ignore the signal (not SIGKILL,SIGSTOP),
                            //handler = SIG_DFL - back to default behavior

  pid_t child = fork();
  if (child > 0)
  {
    //pause(); //waits till a signal arrive
    sleep(3); //csak úgy vár
    kill(child, SIGTERM);
    printf("Signal sent (%i)\n", SIGTERM);
    int status;
    wait(&status); //gyerekei futását várja meg
    printf("Parent process ended\n");
  }
  else
  {
    pause();
    printf("Waits 3 seconds, then send a SIGTERM %i signal\n", SIGTERM);
    //sleep(3);
    //kill(getppid(), SIGTERM);
    //1. parameter the pid number of process, we send the signal
    // 		if -1, then eacho of the processes of the same uid get the signal
    // 		we kill our bash as well! The connection will close
    //2. parameter the name or number of signal
    printf("Child process ended\n");
  }
  return 0;
}