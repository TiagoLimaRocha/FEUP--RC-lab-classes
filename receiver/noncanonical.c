/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "macros.h"

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP = FALSE;

typedef struct
{
  uint8_t flags[5];
} Control_data;

uint8_t make_ctrl_bcc(uint8_t A, uint8_t C)
{
  uint8_t bcc = A ^ C;
  return bcc;
}

int chk_ctrl_bcc(uint8_t A, uint8_t C, uint8_t BCC)
{
  if ((A ^ C) == BCC)
    return TRUE;

  return FALSE;
}

int verify_ctrl(Control_data *ct)
{
  if (ct->flags[LL_START_F] != LL_F ||
      ct->flags[LL_END_F] != LL_F ||
      chk_ctrl_bcc(ct->flags[LL_CTRL_A], ct->flags[LL_CTRL_C], ct->flags[LL_CTRL_BCC]))
  {
    printf("LL: Order of control packet is incorrect or BCC is incorrect\n");
    return FALSE;
  }

  return TRUE;
}

int read_ctrl_pck(int fd)
{
  int counter = 0;

  Control_data ct;

  while (counter < 5)
  {
    res = read(fd, ct.flags[counter], 1); /* returns after 5 chars have been input */
    buf[res] = 0;                         /* so we can printf... */
    printf(":%s:%d\n", buf, res);
    counter++;
  }
}

int main(int argc, char **argv)
{
  int fd, c, res;
  struct termios oldtio, newtio;
  char buf[255];

  if ((argc < 2) ||
      ((strcmp("/dev/ttyS0", argv[1]) != 0) &&
       (strcmp("/dev/ttyS4", argv[1]) != 0)))
  {
    printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS4\n");
    exit(1);
  }

  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */

  fd = open(argv[1], O_RDWR | O_NOCTTY);
  if (fd < 0)
  {
    perror(argv[1]);
    exit(-1);
  }

  if (tcgetattr(fd, &oldtio) == -1)
  { /* save current port settings */
    perror("tcgetattr");
    exit(-1);
  }

  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;

  /* set input mode (non-canonical, no echo,...) */
  newtio.c_lflag = 0;

  newtio.c_cc[VTIME] = 0; /* inter-character timer unused */
  newtio.c_cc[VMIN] = 5;  /* blocking read until 5 chars received */

  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) pr�ximo(s) caracter(es)
  */

  tcflush(fd, TCIOFLUSH);

  if (tcsetattr(fd, TCSANOW, &newtio) == -1)
  {
    perror("tcsetattr");
    exit(-1);
  }

  printf("New termios structure set\n");

  while (STOP == FALSE)
  { /* loop for input */

    if (buf[0] == 'z')
      STOP = TRUE;
  }

  /* 
    O ciclo WHILE deve ser alterado de modo a respeitar o indicado no gui�o 
  */

  sleep(1);

  tcsetattr(fd, TCSANOW, &oldtio);
  close(fd);
  return 0;
}
