#define DFX_REGISTRY_KEY_NAME	"Software\\Destroy FX\\Super Destroy FX transgendered bi-splendored XTREME plugin pack\\1.0"
#define DFX_REGISTRY_VALUE_NAME	"da data"

//-----------------------------------------------------------------------------
// try to open the stored preference data
int DFX_AccessRegistryValue(bool inWriting, HKEY inBaseKey, HKEY * inValueKey)
{
	LONG error;

	if (inWriting)
	{
		error = RegCreateKeyEx(inBaseKey, DFX_REGISTRY_KEY_NAME, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE , NULL, inValueKey, NULL);
	}
	else
	{
		error = RegOpenKeyEx(inBaseKey, DFX_REGISTRY_KEY_NAME, 0, KEY_READ, inValueKey);
	}

	return error;
}

//-----------------------------------------------------------------------------
// open the preference data for reading or writing
int DFX_ReadWriteRegistryValue(bool inWriting, HKEY * inValueKey)
{
	// attempt the first (prefered) domain (user domain)
	int error = DFX_AccessRegistryValue(inWriting, HKEY_CURRENT_USER, inValueKey);
	// failing that, attempt the backup domain (local domain)
	if (error != ERROR_SUCCESS)
		error = DFX_AccessRegistryValue(inWriting, HKEY_LOCAL_MACHINE, inValueKey);

	return error;
}
