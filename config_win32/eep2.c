// read data from registry

	// check the size of the stored data
	LONG error;
	DWORD dataSize;
	error = RegQueryValueEx(dataRef, AUTH_REGISTRY_VALUE_NAME, NULL, NULL, NULL, &dataSize);
	if (error != ERROR_SUCCESS)
	{
		RegCloseKey(dataRef);
		return AUTH_CHECK_SIZE_CALC_ERR;
	}
	if (dataSize != AUTH_DATA_SIZE)
	{
		RegCloseKey(dataRef);
		return AUTH_CHECK_DATA_SIZE_ERR;
	}

	// copy the data out of the file into a buffer in memory
	error = RegQueryValueEx(dataRef, AUTH_REGISTRY_VALUE_NAME, NULL, NULL, buf, &dataSize);
	// file read error
	if ( (error != ERROR_SUCCESS) || (dataSize != CORRECT_DATA_SIZE) )
	{
		RegCloseKey(dataRef);
		return AUTH_CHECK_DATA_GET_ERR;
	}

	// all done reading, close the auth file
	error = RegCloseKey(dataRef);



// write the the data to registry

	// now flush the authorization data out to disk to save it
	LONG error;
	DWORD numBytes = AUTH_DATA_SIZE;
	error = RegSetValueEx(dataRef, AUTH_REGISTRY_VALUE_NAME, 0, REG_BINARY, buf, numBytes);
	// if there was an error or the number of bytes written was not what we tried to write, exit with an error
	if (error != ERROR_SUCCESS)
	{
		RegCloseKey(dataRef);
		return AUTH_WRITEDATA_WRITE_FILE_ERR;
	}

	// close the file
	error = RegCloseKey(dataRef);
	// the file must close without error, otherwise our data probably was not saved
	if (error != ERROR_SUCCESS)
		return AUTH_WRITEDATA_CLOSE_FILE_ERR;
