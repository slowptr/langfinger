#include "sqlite3.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct LF_SQLite_entry_struct
{
  int index;
  char *value;
  int value_len;
};

void
LF_SQLite_exec (char *db_path, const char *query,
                void (*callback) (void *, struct LF_SQLite_entry_struct *),
                void *data)
{
  sqlite3 *db;
  sqlite3_stmt *stmt;

  int rc = sqlite3_open_v2 (db_path, &db, SQLITE_OPEN_READONLY, NULL);
  if (rc != SQLITE_OK)
    {
      fprintf (stderr, "unable to open database: %s\n", sqlite3_errmsg (db));
      sqlite3_close (db);
      return;
    }

  rc = sqlite3_prepare_v2 (db, query, -1, &stmt, NULL);
  if (rc != SQLITE_OK)
    {
      fprintf (stderr, "unable to prepare statement: %s\n",
               sqlite3_errmsg (db));
      sqlite3_close (db);
      return;
    }

  while (sqlite3_step (stmt) == SQLITE_ROW)
    {
      int column_count = sqlite3_column_count (stmt);
      struct LF_SQLite_entry_struct *entry
          = malloc (sizeof (struct LF_SQLite_entry_struct) * column_count);
      for (int i = 0; i < column_count; i++)
        {
          entry[i].index = i;
          entry[i].value = strdup ((char *)sqlite3_column_text (stmt, i));
          entry[i].value_len = sqlite3_column_bytes (stmt, i);
        }
      callback (data, entry);
      for (int i = 0; i < column_count; i++)
        {
          free (entry[i].value);
        }
      free (entry);
    }

  sqlite3_finalize (stmt);
  sqlite3_close (db);
}
