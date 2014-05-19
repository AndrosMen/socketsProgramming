#include <sys/types.h>
#include <strings.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include "server.h"
#include "defaults.h"

int debugLevel = 0;

int main(int argc, char *args[])
{
	system("clear");
	printf("Server Started at %s\n", args[1]);
	start_server("0.0.0.0", atoi(args[1]), 5);
	return 0;
}
