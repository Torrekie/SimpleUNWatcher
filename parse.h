#ifndef __SUNW_PARSE_H
#define __SUNW_PARSE_H

#include <sqlite3.h>
#include <sys/types.h>
#include <CoreFoundation/CoreFoundation.h>

typedef int nc_app_id;

__BEGIN_DECLS
/* Global variable for opened DB */
sqlite3 *gNotificationDB;

/* init gNotificationDB by path */
int nc_db_init(const char *path_db);

/* Get matching app_id field of bundleid from gNotificationDB */
nc_app_id nc_app_id_bundleid(const char *bundle);

/* Get recorded notifications by app_id in plist */
int nc_get_data_plists(nc_app_id app_id, CFDataRef **data, int *data_count);

int parse_nc_bundleid(const char *path_db, const char *bundle);
__END_DECLS

#endif
