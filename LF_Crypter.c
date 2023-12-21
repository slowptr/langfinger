#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>

// libcrypto.dll & libssl.dll (OpenSSL) are required for AES-256-GCM decryption
#include <openssl/aes.h>
#include <openssl/evp.h>

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

char *
LF_Crypter_decrypt_aes_256_gcm (char *decrypted_key, char *iv, char *tag,
                                int tag_len)
{
  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new ();
  if (ctx == NULL)
    {
      fprintf (stderr, "unable to create cipher context\n");
      exit (-1);
    }

  if (!EVP_DecryptInit_ex (ctx, EVP_aes_256_gcm (), NULL,
                           (unsigned char *)decrypted_key,
                           (unsigned char *)iv))
    {
      fprintf (stderr, "unable to initialize cipher context\n");
      exit (-1);
    }

  int len = 0;
  unsigned char *decrypted_data = malloc (512); // for now
  if (!EVP_DecryptUpdate (ctx, decrypted_data, &len, (unsigned char *)tag,
                          tag_len))
    {
      fprintf (stderr, "unable to decrypt data\n");
      exit (-1);
    }

  int final_len = 0;
  EVP_DecryptFinal_ex (ctx, decrypted_data + len,
                       &final_len); // fails but is right?

  decrypted_data = realloc (decrypted_data, len + final_len + 1);
  decrypted_data[len + final_len] = '\0';

  EVP_CIPHER_CTX_free (ctx);
  return (char *)decrypted_data; // unsigned/signed...
}

void
LF_Crypter_separate_iv_tag (char *encrypted_data, int encrypted_data_len,
                            char **iv, char **tag, int *tag_len)
{
  const size_t iv_len = 12;
  const size_t iv_offset = 3;
  *tag_len = (encrypted_data_len - iv_offset - iv_len) - AES_BLOCK_SIZE;

  *iv = malloc (iv_len);
  memcpy (*iv, encrypted_data + iv_offset, iv_len);

  *tag = malloc (*tag_len);
  if (encrypted_data_len >= AES_BLOCK_SIZE + iv_offset + iv_len)
    {
      memcpy (*tag, encrypted_data + iv_offset + iv_len, *tag_len);
    }
  else
    {
      fprintf (stderr, "unable to separate iv and tag\n");
      exit (-1);
    }
}
