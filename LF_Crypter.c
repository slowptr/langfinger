#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>

#define LF_Crypter__CRYPT32_MODULE_NAME "crypt32.dll"

typedef BOOL (WINAPI *LF_Crypter__crypt32_unprotect_data_t) (
    DATA_BLOB *pDataIn, LPWSTR *ppszDataDescr, DATA_BLOB *pOptionalEntropy,
    PVOID pvReserved, CRYPTPROTECT_PROMPTSTRUCT *pPromptStruct, DWORD dwFlags,
    DATA_BLOB *pDataOut);
typedef BOOL (WINAPI *LF_Crypter__crypt32_string_to_binary_t) (
    LPCSTR pszString, DWORD cchString, DWORD dwFlags, BYTE *pbBinary,
    DWORD *pcbBinary, DWORD *pdwSkip, DWORD *pdwFlags);

HANDLE
LF_Crypter__load_crypt32 (void)
{
  HMODULE module_handle = LoadLibrary (LF_Crypter__CRYPT32_MODULE_NAME);
  if (module_handle == NULL)
    {
      fprintf (stderr, "unable to load %s (%lu)\n",
               LF_Crypter__CRYPT32_MODULE_NAME, GetLastError ());
      exit (-1);
    }
  return module_handle;
}

void
LF_Crypter__unload_crypt32 (HMODULE module_handle)
{
  FreeLibrary (module_handle);
}

char *
LF_Crypter_decrypt_dpapi (char *encryption_key, int encryption_key_len,
                          int *len)
{
  HMODULE crypt32 = LF_Crypter__load_crypt32 ();

  LF_Crypter__crypt32_unprotect_data_t crypt32_unprotect_data;
  crypt32_unprotect_data
      = (LF_Crypter__crypt32_unprotect_data_t)GetProcAddress (
          crypt32, "CryptUnprotectData");
  if (crypt32_unprotect_data == NULL)
    {
      fprintf (stderr, "unable to find CryptUnprotectData (%lu)\n",
               GetLastError ());
      exit (-1);
    }

  LF_Crypter__crypt32_string_to_binary_t crypt32_string_to_binary;
  crypt32_string_to_binary
      = (LF_Crypter__crypt32_string_to_binary_t)GetProcAddress (
          crypt32, "CryptStringToBinaryA");
  if (crypt32_string_to_binary == NULL)
    {
      fprintf (stderr, "unable to find CryptStringToBinaryA (%lu)\n",
               GetLastError ());
      exit (-1);
    }

  DATA_BLOB data_in;
  DATA_BLOB data_out;
  memset (&data_in, 0, sizeof (data_in));
  memset (&data_out, 0, sizeof (data_out));

  // string to binary to get the length
  crypt32_string_to_binary (encryption_key, 0, CRYPT_STRING_BASE64, NULL,
                            &data_in.cbData, NULL, NULL);

  // allocate memory for the binary string
  data_in.pbData = malloc (data_in.cbData);

  // string to binary to get the binary string
  crypt32_string_to_binary (encryption_key, 0, CRYPT_STRING_BASE64,
                            data_in.pbData, &data_in.cbData, NULL, NULL);

  data_in.cbData -= 5;
  memmove (data_in.pbData, data_in.pbData + 5, data_in.cbData);
  if (!crypt32_unprotect_data (&data_in, NULL, NULL, NULL, NULL, 0, &data_out))
    {
      fprintf (stderr, "unable to decrypt data (%lu)\n", GetLastError ());
      exit (-1);
    }
  LF_Crypter__unload_crypt32 (crypt32);

  *len = data_out.cbData;
  char *decrypted_data = malloc (*len + 1);
  memcpy (decrypted_data, data_out.pbData, *len);
  decrypted_data[*len] = '\0';

  LocalFree (data_out.pbData);
  LocalFree (data_in.pbData);

  if (decrypted_data == NULL)
    {
      fprintf (stderr, "unable to decrypt dpapi data\n");
      exit (-1);
    }

  return decrypted_data;
}
