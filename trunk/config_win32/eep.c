#define AUTH_REGISTRY_KEY_NAME	"Software\\Destroy FX\\Super Destroy FX transgendered bi-splendored XTREME plugin pack\\1.0"
#define AUTH_REGISTRY_VALUE_NAME	"da data"

//-----------------------------------------------------------------------------
// given a file system domain ref num, try to open the stored authorization data
int shakeleg(bool writing, HKEY baseKey, HKEY *valueKey)
{
  LONG error;

  if (writing)
    {
      DWORD moreInfo;
      error = RegCreateKeyEx(baseKey, AUTH_REGISTRY_KEY_NAME, 0, NULL, REG_OPTION_NON_VOLATILE, 
			     KEY_ALL_ACCESS, NULL, valueKey, &moreInfo);
      if (error != ERROR_SUCCESS)
	return AUTH_OPENDATA_CREATE_FILE_ERR;
      //if (moreInfo == REG_CREATED_NEW_KEY) printf("The key did not exist and was created.\n");
      //if (moreInfo == REG_OPENED_EXISTING_KEY) printf("The key existed and was simply opened without being changed.\n");
    }
  else
    {
      error = RegOpenKeyEx(baseKey, AUTH_REGISTRY_KEY_NAME, 0, KEY_READ, valueKey);
      if (error != ERROR_SUCCESS)
	return AUTH_OPENDATA_OPEN_FILE_ERR;
    }

  return DFX_AUTH_NOERR;	// success
}

//-----------------------------------------------------------------------------
// open the authorization file for reading or writing
int shakejeans(bool writing, HKEY *valueKey)
{
	HKEY searchKey1 = writing ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
	HKEY searchKey2 = writing ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;

	// attempt the first (prefered) domain (the local machine)
	int goodleg = shakeleg(writing, searchKey1, valueKey);
	// failing that, attempt the backup domain (the user domain)
	if (goodleg != DFX_AUTH_NOERR)
		goodleg = shakeleg(writing, searchKey2, valueKey);
	// failing that, exit with the error
	if (goodleg != DFX_AUTH_NOERR)
		return goodleg;

	return DFX_AUTH_NOERR;	// success
}
