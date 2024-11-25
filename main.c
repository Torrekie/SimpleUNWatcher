#include <TargetConditionals.h>
#include <Availability.h>

#if TARGET_OS_EMBEDDED
#error "This implementation is not for Embedded!"
#elif __MAC_OS_X_VERSION_MAX_ALLOWED < __MAC_10_10
#error "This implementation is not for system earlier Mac OS X 10.10"
#else
/* Mac OS X 10.10 or above */
#include <stdio.h>
#include <string.h>
#include <stdlib.h> /* exit() */
#include <stdbool.h>
#include <unistd.h> /* confstr() */
#include <err.h>
#include <errno.h>
#include <limits.h> /* PATH_MAX */
#include <dirent.h> /* opendir(), readdir(), closedir() */

#include "parse.h"

static const char *getext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

static void perrorx(const char *str)
{
	perror(str);
	exit(1);
}

int main(int argc, char *argv[])
{
	size_t size;
	char user_dir[PATH_MAX], db_dir[PATH_MAX], db_path[PATH_MAX];
	DIR *d;
	struct dirent *dir;
	bool db_found = 0;

	if (argc < 2)
		errx(1, "Usage: %s <bundleID>", argv[0]);

	printf("Bundle ID: %s\n", argv[1]);

	/* Get system default user data dir (typically 'getconf DARWIN_USER_DIR') */
	size = confstr(_CS_DARWIN_USER_DIR, user_dir, sizeof(user_dir));
	if (size == 0)
		perrorx("confstr");

	printf("User Dir: %s\n", user_dir);

	/* user_dir + com.apple.notificationcenter/db2/ = user notification db path */
	sprintf(db_dir, "%s%s", user_dir, "com.apple.notificationcenter/db2/");

	d = opendir(db_dir);
	if (!d)
		perrorx("opendir");

	printf("NotificationCenter Database Dir: %s\n", db_dir);

	while ((dir = readdir(d)) != NULL) {
		/* match *.db or db */
		if (dir->d_type == DT_REG && (strcmp(dir->d_name, "db") == 0 || strcmp(getext(dir->d_name), "db") == 0)) {
			sprintf(db_path, "%s%s", db_dir, dir->d_name);
			db_found = true;
			break;
		}
	}
	closedir(d);

	if (!db_found) {
		fprintf(stderr, "NotificationCenter Database missing!\n");
		exit(1);
	}

	printf("NotificationCenter Database: %s\n", db_path);

	return parse_nc_bundleid(db_path, argv[1]);
}
#endif
