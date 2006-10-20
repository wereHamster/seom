
#include <seom/server.h>

#include <getopt.h>
#include <signal.h>
#include <syslog.h>

static const char rev[] = "$Revision$";
static const char pidpath[] = "/var/run/seom-server.pid";
static const int version[3] = { 0, 1, 0 };
static int daemonmode = 0;

static char seomOutput[1024];
static struct option seomServerOptions[] = {
	{ "outdir",  1, 0, 'o' },	
	{ "version", 0, 0, 'v' },
	{ "help",    0, 0, 'h' },
	{ "daemon",  0, 0, 'd' },
	{ 0, 0, 0, 0 }
};

static void logmessage(char* msg)
{
	if (daemonmode)
		syslog(LOG_USER | LOG_INFO, msg);
	
	else
		printf(msg);
}

static void sighandler(int signum)
{
	if (signum == SIGINT || signum == SIGTERM) {
		unlink(pidpath);
		logmessage("Closing cleanly.\n");
		exit(0);
	}
}

static void help(void)
{
	printf(
		"seom-server version %d.%d.%d\n"
		"\t-o, --outdir:  output directory prefix\n"
		"\t-v, --version: print version info\n"
		"\t-h, --help:    print help text\n"
		"\t-d, --daemon:  background server process\n",
		version[0], version[1], version[2]
	);
}

/*
static char *date(void)
{
	static char sbuf[32];
	time_t tim = time(NULL);
	struct tm *tm = localtime(&tim);
	snprintf(sbuf, 32, "%d-%02d-%02d--%02d:%02d:%02d",
		tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
	return sbuf;
}
*/

int main(int argc, char *argv[])
{
	char msg[1024];
	int opindex = 0;
	
	memset(msg, 0, sizeof(msg));

	snprintf(seomOutput, sizeof(seomOutput), "%s/seom", getenv("HOME"));

	for (;;)  {
		int c = getopt_long(argc, argv, "o:vhd", seomServerOptions, &opindex);
		
		if (c == -1) {
			break;
		} else {
			switch(c) {
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
				daemonmode = 1;
				break;
			default:
				break;
			}
		}
 	}
	
	seomServer *server = seomServerCreate(seomOutput);

	if(daemonmode) {
		FILE* pidfile;
		struct stat sstat;
		pid_t pid = 0;
		pid_t sid = 0;

		if(!stat(pidpath, &sstat)) {
			snprintf(msg, sizeof(msg), "Pidfile %s already exists.", pidpath);
			logmessage(msg);

			exit(1);
		}

		pid = fork();
		
		if(pid < 0) {
			logmessage("Couldn't fork process.");

			exit(2);
		}
		
		if(pid > 0)
			exit(0);

		sid = setsid();
		
		if(sid < 0) {
			logmessage("Couldn't setsid().");

			exit(3);
		}

		else {
			pidfile = fopen(pidpath, "w+");

			if(pidfile)
				fprintf(pidfile, "%d\n", getpid());

			else {
				snprintf(msg, sizeof(msg), "Couldn't open pidfile %s.", pidpath);
				logmessage(msg);
				
				exit(4);
			}

			fclose(pidfile);

			umask(0);
			chdir("/");
			close(STDIN_FILENO);
			close(STDOUT_FILENO);
			close(STDERR_FILENO);
		}
	}

	signal(SIGINT, sighandler);
	signal(SIGTERM, sighandler);

	snprintf(msg, sizeof(msg), "Using output directory %s [PID: %d]\n", seomOutput, getpid());
	logmessage(msg);

	for (;;) {
		seomServerDispatch(server);
	}

	seomServerDestroy(server);
	
	return 0;
}
