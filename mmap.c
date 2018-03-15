#include <sys/mman.h>
#include <sys/types.h>
#include <sys/time.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <err.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>

static struct timespec ts_start, ts_end;
static int alarm_timeout;
static volatile int alarm_fired;

#define timespecsub(vvp, uvp)						\
	do {								\
		(vvp)->tv_sec -= (uvp)->tv_sec;				\
		(vvp)->tv_nsec -= (uvp)->tv_nsec;			\
		if ((vvp)->tv_nsec < 0) {				\
			(vvp)->tv_sec--;				\
			(vvp)->tv_nsec += 1000000000;			\
		}							\
	} while (0)

static void
alarm_handler(int signum)
{
	alarm_fired = 1;
}

static void
benchmark_start(void)
{
	int error;

	alarm_fired = 0;
	if (alarm_timeout) {
		signal(SIGALRM, alarm_handler);
		alarm(alarm_timeout);
	}
	error = clock_gettime(CLOCK_REALTIME, &ts_start);
	assert(error == 0);
}

static void
benchmark_stop(void)
{
	int error;

	error = clock_gettime(CLOCK_REALTIME, &ts_end);
	assert(error == 0);
}

uintmax_t
test_mmap_read(uintmax_t num, uintmax_t mmap_size, const char *path)
{
	char buf[mmap_size];
	char *addr = 0, *fp, *memp;
	uintmax_t i, j;
	off_t off = 0;
  int fd;
	size_t junk, c_size = 64, r, range;
	size_t len = mmap_size;
	unsigned long long memsize;

  	srand(time(NULL));
  
	fd = open(path, O_RDONLY);

	if (fd < 0)
		err(-1, "test_open_read: %s", path);

	fp = (char*) mmap(addr, len, PROT_READ, MAP_PRIVATE, fd, off);

	if (fp == MAP_FAILED )
		err(-1, "mmap:");

	range = len / c_size; // divide working space into 64 byte blocks (cache-line size)
   	
	memp = fp;

	benchmark_start();
	for (i = 0; i < num; i++) { // randomly read from the working space
    	memp = fp;
	 	if (alarm_fired)
		  	break;
    	r = rand() % range; // random number in the range
    	memp += r * c_size;
		junk += memp[0];
	}
	benchmark_stop();
	close(fd);
	return (i); // per iterations
}

struct test {
	const char	*t_name;
	uintmax_t	(*t_func)(uintmax_t, uintmax_t, const char *);
	int		 t_flags;
};

#define	FLAG_PATH	0x00000001

static const struct test tests[] = {
	{ "mmap_read", test_mmap_read, .t_flags = FLAG_PATH },
};

static const int tests_count = sizeof(tests) / sizeof(tests[0]);

static void
usage(void)
{
	int i;

	fprintf(stderr, "mmap [-i iterations] [-l loops] "
	    "[-s seconds] -f filesize -p path-to-file\n");
	exit(-1);
}

int main(int argc, char *argv[])
{
	struct timespec ts_res;
	const struct test *the_test;
	const char *path;
	char *tmp_dir, *tmp_path, *endp;
	long long ll;
	int ch, fd, error, i, j, k, rv, opt_path, opt_size;
	uintmax_t iterations, loops, size;

  opt_path = 0;
  opt_size = 0;
	alarm_timeout = 1;
	iterations = 0;
	size = 0;
	loops = 10;
	path = NULL;
	tmp_path = NULL;

	while ((ch = getopt(argc, argv, "i:l:p:s:f:")) != -1) {
		switch (ch) {
		case 'i':
			ll = strtol(optarg, &endp, 10);
			if (*endp != 0 || ll < 1)
				usage();
			iterations = ll;
			break;

		case 'l':
			ll = strtol(optarg, &endp, 10);
			if (*endp != 0 || ll < 1 || ll > 100000)
				usage();
			loops = ll;
			break;

		case 'p':
      opt_path = 1;
			path = optarg;
			break;

		case 's':
			ll = strtol(optarg, &endp, 10);
			if (*endp != 0 || ll < 1 || ll > 60*60)
				usage();
			alarm_timeout = ll;
			break;

		case 'f':
      opt_size = 1;
			ll = strtol(optarg, &endp, 10);
			if (*endp != 0 || ll < 1)
				usage();
			size = ll;
			break;

		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

  if ((opt_path + opt_size) < 2) {
    usage();
  }

	if (iterations < 1 && alarm_timeout < 1)
		usage();
	if (iterations < 1)
		iterations = UINT64_MAX;
	if (loops < 1)
		loops = 1;

	/*
	 * Validate if a path is required, it is
	 * defined.
	 */
  the_test = &tests[0];
	if ((the_test->t_flags & FLAG_PATH) && (path == NULL)) {
		tmp_dir = strdup("/tmp/syscall_timing.XXXXXXXX");
		if (tmp_dir == NULL)
			err(1, "strdup");
		tmp_dir = mkdtemp(tmp_dir);
		if (tmp_dir == NULL)
			err(1, "mkdtemp");
		rv = asprintf(&tmp_path, "%s/testfile", tmp_dir);
		if (rv <= 0)
			err(1, "asprintf");
	}
	error = clock_getres(CLOCK_REALTIME, &ts_res);
	assert(error == 0);
	printf("Clock resolution: %ju.%09ju\n", (uintmax_t)ts_res.tv_sec,
	    (uintmax_t)ts_res.tv_nsec);
	printf("test\tloop\ttime\titerations\tper-iteration\n");


	uintmax_t calls, nsecsperit;

	the_test = &tests[0];
	if (tmp_path != NULL) {
		fd = open(tmp_path, O_WRONLY | O_CREAT, 0700);
		if (fd < 0)
			err(1, "cannot open %s", tmp_path);
		error = ftruncate(fd, 1000000);
		if (error != 0)
			err(1, "ftruncate");
		error = close(fd);
		if (error != 0)
			err(1, "close");
		path = tmp_path;
	}

	/*
	 * Run one warmup, then do the real thing (loops) times.
	 */
	the_test->t_func(iterations, size, path);
	calls = 0;
	for (k = 0; k < loops; k++) {
		calls = the_test->t_func(iterations, size,
		    path);
		timespecsub(&ts_end, &ts_start);
		printf("%s\t%d\t", the_test->t_name, k);
		printf("%ju.%09ju\t%ju\t", (uintmax_t)ts_end.tv_sec,
		    (uintmax_t)ts_end.tv_nsec, calls);

	/*
	 * Note.  This assumes that each iteration takes less than
	 * a second, and that our total nanoseconds doesn't exceed
	 * the room in our arithmetic unit.  Fine for system calls,
	 * but not for long things.
	 */
		nsecsperit = ts_end.tv_sec * 1000000000;
		nsecsperit += ts_end.tv_nsec;
		nsecsperit /= calls;
		printf("0.%09ju\n", (uintmax_t)nsecsperit);
	}

	return 0;
}

