
#include <seom/server.h>

#include <getopt.h>
#include <signal.h>

static const char rev[] = "$Revision$";
static const int version[3] = { 0, 1, 0 };

static char seomOutput[1024];
static struct option seomServerOptions[] = {
	{ "port",    1, 0, 'p' },
	{ "outdir",  1, 0, 'o' },	
	{ "version", 0, 0, 'v' },
	{ "help",    0, 0, 'h' },
	{ "daemon",  0, 0, 'd' },
	{ 0, 0, 0, 0 }
};

static void sighandler(int signum)
{
	if (signum == SIGINT) {
		printf("Closing cleanly...\n");
		exit(0);
	}
}

static void help(void)
{
	printf(
		"seomServer version %d.%d.%d\n"
		"\t-p, --port:    port number\n"
		"\t-o, --outdir:  output directory prefix\n"
		"\t-v, --version: print version info\n"
		"\t-h, --help:    print help text\n"
		"\t-d, --daemon:  background server process; unimplimented!\n",
		version[0], version[1], version[2]
	);
}

static char *date(void)
{
	static char sbuf[32];
	time_t tim = time(NULL);
	struct tm *tm = localtime(&tim);
	snprintf(sbuf, 32, "%d-%02d-%02d--%02d:%02d:%02d",
		tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
	return sbuf;
}

int main(int argc, char *argv[])
{
	int opindex = 0;
	int port = 9000;
	
	snprintf(seomOutput, sizeof(seomOutput), "%s/seom", getenv("HOME"));

	for (;;)  {
		int c = getopt_long(argc, argv, "p:o:vhd", seomServerOptions, &opindex);
		
		if (c == -1) {
			break;
		} else {
			switch(c) {
			case 'p':
				port = atoi(optarg);
				break;
			case 'o':
				snprintf(seomOutput, sizeof(seomOutput), "%s", optarg);
				break;
			case 'v':
				printf("%d.%d.%d (%s)\n", version[0], version[1], version[2], rev);
				exit(0);
			case 'h':
				help();
				exit(0);
			case 'd':
				printf("Daemonizing unimplimented; please check back.\n");
				break;
			default:
				break;
			}
		}
 	}
 	
	printf("[%s]: running on port %d using output directory %s\n", date(), port, seomOutput);

	signal(SIGINT, sighandler);
	
	seomServer *server = seomServerCreate(port, seomOutput);
	for (;;) {
		seomServerDispatch(server);
	}
	seomServerDestroy(server);
	
	return 0;
}
