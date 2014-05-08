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

int debugLevel = CONFIG_DEFAULT_VERBOSE;

int main()
{
	debugLevel = 5;
	start_server("0.0.0.0", 8080, 5);
	return 0;
}
