#include "LF_Chrome.c"

// LF_[Module]_[Function]  = public
// LF_[Module]__[Function] = private

int
main ()
{
  printf ("langfinger v0.0.1\n");

  struct LF_Chrome_data_struct chrome_data;
  LF_Chrome_populate_data (&chrome_data);

  if (chrome_data.profile_path_list->length == 0)
    {
      printf ("No profiles found.\n");
      return 0;
    }

  for (int i = 0; i < chrome_data.profile_path_list->length; i++)
    {
      char *profile_path
          = LF_Utils_arraylist_get (chrome_data.profile_path_list, i);
      printf ("Profile: %s\n", profile_path);
    }

  LF_Chrome_free_data (&chrome_data);

  return 0;
}
