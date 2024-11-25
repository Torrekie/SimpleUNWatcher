#include "parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <CoreFoundation/CoreFoundation.h>

static void printBinaryPlistAsXML(CFDataRef data) {
	CFPropertyListRef plist = CFPropertyListCreateWithData(NULL, data, kCFPropertyListImmutable, NULL, NULL);
	if (!plist) {
		fprintf(stderr, "Failed to parse binary plist.\n");
		return;
	}

	CFDataRef xmlData = CFPropertyListCreateXMLData(NULL, plist);
	if (!xmlData) {
		fprintf(stderr, "Failed to convert plist to XML.\n");
	} else {
		printf("%.*s\n", (int)CFDataGetLength(xmlData), CFDataGetBytePtr(xmlData));
		CFRelease(xmlData);
	}

	CFRelease(plist);
}

int nc_db_init(const char *path_db)
{
	char *zErrMsg = NULL;
	int ret;

	ret = sqlite3_open(path_db, &gNotificationDB);
	if (ret)
		fprintf(stderr, "Cannot open Notification Center DB: %s\n", sqlite3_errmsg(gNotificationDB));

	return ret;
}

nc_app_id nc_app_id_bundleid(const char *bundle)
{
	sqlite3_stmt *stmt;
	const char *query = "SELECT app_id FROM app WHERE identifier = ?;";
	int rc;
	int app_id;

	/* Prepare the SQL statement */
	rc = sqlite3_prepare_v2(gNotificationDB, query, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(gNotificationDB));
		return -1;
	}

	/* Bind the identifier/bundleid value to the prepared statement */
	sqlite3_bind_text(stmt, 1, bundle, -1, SQLITE_STATIC);

	/* Execute the query and fetch the result */
	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
		app_id = sqlite3_column_int(stmt, 0); /* Get the app_id */
		printf("app_id: %d\n", app_id);
	}

	if (rc != SQLITE_DONE) {
		fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(gNotificationDB));
	}

	sqlite3_finalize(stmt);

	return app_id;
}

int nc_get_data_plists(nc_app_id app_id, CFDataRef **data, int *data_count)
{
	sqlite3_stmt *stmt;
	const char *query = "SELECT data FROM record WHERE app_id = ?;";
	int rc = SQLITE_OK;
	int count = 0;
	CFDataRef *dataRefs = NULL;

	rc = sqlite3_prepare_v2(gNotificationDB, query, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(gNotificationDB));
		return rc;
	}

	sqlite3_bind_int(stmt, 1, app_id);

	// Dynamically allocate memory for CFDataRef array
	dataRefs = (CFDataRef *)malloc(sizeof(CFDataRef) * 10); // Initial allocation for up to 10 entries
	if (!dataRefs) {
		fprintf(stderr, "Memory allocation failed.\n");
		sqlite3_finalize(stmt);
		return SQLITE_NOMEM;
	}

	int capacity = 10; // Current capacity of the array

	// Execute the query and fetch all matching results
	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
		const void *buffer = sqlite3_column_blob(stmt, 0); // Get the binary plist data
		int size = sqlite3_column_bytes(stmt, 0); // Get the size of the binary data

		if (buffer && size > 0) {
			CFDataRef dataRef = CFDataCreate(NULL, buffer, size);
			if (!dataRef) {
				fprintf(stderr, "Failed to create CFDataRef.\n");
				continue; // Skip this entry if CFDataRef creation fails
			}

			// Resize the array if needed
			if (count >= capacity) {
				capacity *= 2;
				CFDataRef *newDataRefs = realloc(dataRefs, sizeof(CFDataRef) * capacity);
				if (!newDataRefs) {
					fprintf(stderr, "Memory reallocation failed.\n");
					CFRelease(dataRef); // Clean up the current CFDataRef
					break; // Exit on memory failure
				}
				dataRefs = newDataRefs;
			}

			// Add the CFDataRef to the array
			dataRefs[count++] = dataRef;
		}
	}

	if (rc == SQLITE_DONE && count == 0) {
		fprintf(stderr, "No rows found for app_id %d.\n", app_id);
		rc = SQLITE_NOTFOUND;
	} else if (rc != SQLITE_DONE) {
		fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(gNotificationDB));
	}

	// Clean up
	sqlite3_finalize(stmt);

	// If no data was retrieved, clean up and return failure
	if (count == 0) {
		free(dataRefs);
		*data = NULL;
		*data_count = 0;
		rc = SQLITE_NOTFOUND;
	}

	// Set the output parameters
	*data = dataRefs;
	*data_count = count;

	return rc;
}


int parse_nc_bundleid(const char *path_db, const char *bundle)
{
	if (nc_db_init(path_db))
		return 1;

	nc_app_id id = nc_app_id_bundleid(bundle);
	printf("nc_app_id_bundleid: %d\n", id);

	int rc, count;
	CFDataRef *data;

	rc = nc_get_data_plists(id, &data, &count);
	printf("nc_get_data_plist: %d %d\n", rc, count);

	for (int i = 0; i < count; i++) {
		printBinaryPlistAsXML(data[i]);
		if (data[i]) CFRelease(data[i]);
	}

	if (data) free(data);

	return 0;
}
