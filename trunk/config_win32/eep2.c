// read data from registry

	// check the size of the stored data
	LONG error;
	DWORD dataSize = 0;
	error = RegQueryValueEx(valueKey, DFX_REGISTRY_VALUE_NAME, NULL, NULL, NULL, &dataSize);
	if ( (error != ERROR_SUCCESS) || (dataSize != DFX_REGISTRY_VALUE_DATA_SIZE) )
	{
		RegCloseKey(valueKey);
		if (error == ERROR_SUCCESS)
			error = ERROR_INCORRECT_SIZE;
		return ERROR_INCORRECT_SIZE;
	}

	// copy the data out of the file into a buffer in memory
	error = RegQueryValueEx(valueKey, DFX_REGISTRY_VALUE_NAME, NULL, NULL, dataBuf, &dataSize);
	// file read error
	if ( (error != ERROR_SUCCESS) || (dataSize != DFX_REGISTRY_VALUE_DATA_SIZE) )
	{
		RegCloseKey(valueKey);
		if (error == ERROR_SUCCESS)
			error = ERROR_INCORRECT_SIZE;
		return error;
	}

	// all done reading, close the registry key
	error = RegCloseKey(valueKey);



// write the the data to registry

	// now flush the registry data out to disk to save it
	LONG error;
	DWORD dataSize = DFX_REGISTRY_VALUE_DATA_SIZE;
	error = RegSetValueEx(valueKey, DFX_REGISTRY_VALUE_NAME, 0, REG_BINARY, dataBuf, dataSize);
	// if there was an error or the number of bytes written was not what we tried to write, exit with an error
	if (error != ERROR_SUCCESS)
	{
		RegCloseKey(valueKey);
		return error;
	}

	// close the file
	error = RegCloseKey(valueKey);
	// the file must close without error, otherwise our data probably was not saved
	if (error != ERROR_SUCCESS)
		return error;
