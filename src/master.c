#include <stdio.h>
#include <unistd.h>
#include <windows.h>
#include <wincrypt.h>
#include <stdint.h>

#include "config.h"

#define MASTER_PRIVATEKEY    L"private.dlk"
#define MASTER_PUBLICKEY     L"public.dkl"
#define MASTER_MAXHEADERSIZE 3145728

char *masterName = NULL;

void usage(void)
{
	fprintf(stderr, "Uso: %s [opcao] <argumento>\n"
		"Opcoes:\n\n"
		"  -g  Gera um novo par de chaves.\n"
		"  -h  Converte os dados de um arquivo em formato de cabecalho.\n", masterName);
}

void export_key(HCRYPTKEY key, DWORD type, const LPWSTR filename)
{
	BYTE data[DILMA_RSABITSIZE];
	DWORD size = DILMA_RSABITSIZE;
	if(CryptExportKey(key, 0, type, 0, data, &size))
	{
		HANDLE file = CreateFileW(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if(file != INVALID_HANDLE_VALUE)
		{
			DWORD written;
			WriteFile(file, data, size, &written, NULL);
			CloseHandle(file);
		}
	}
}

void gen_rsa_key(void)
{
	HCRYPTPROV provider;
	HCRYPTKEY key;

	if(CryptAcquireContextW(&provider, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
	{
		printf(" :: Gerando par de chaves RSA\n");
		if(CryptGenKey(provider, CALG_RSA_KEYX, (DILMA_RSABITSIZE << 16) | CRYPT_EXPORTABLE, &key))
		{
			printf(" :: Exportando a chave privada\n");
			export_key(key, PRIVATEKEYBLOB, MASTER_PRIVATEKEY);

			printf(" :: Exportando a chave publica\n");
			export_key(key, PUBLICKEYBLOB, MASTER_PUBLICKEY);

			CryptDestroyKey(key);
		}
		CryptReleaseContext(provider, 0);
	}
}

void string_to_hex(char *string, char *hex)
{
	int size = strlen(string);

	if(size > 8)
		size = 8;

	for(int max = 0; max < size; max++)
		snprintf(&hex[max*2], MAX_PATH, "%.2x", string[max]);

	hex[size] = '\0';
}

void create_header(char *filename)
{
	HANDLE file = CreateFileA(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	if(file != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER sz;

		GetFileSizeEx(file, &sz);
		if(sz.QuadPart > 0 && sz.QuadPart < MASTER_MAXHEADERSIZE)
		{
			char newName[MAX_PATH], varName[MAX_PATH], varSize[MAX_PATH];

			string_to_hex(filename, newName);
			strcpy(varName, newName);
			strcpy(varSize, newName);
			strncat(newName, ".h\0", 3);

			HANDLE headerFile = CreateFileA(newName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if(headerFile != INVALID_HANDLE_VALUE)
			{
				char buffer[MAX_PATH];//gambiarra
				DWORD size, sizeVar = sz.QuadPart;//outra gambiarra
				unsigned char byte;//super gambiarra

				size = snprintf(buffer, MAX_PATH, "//arquivo gerado automaticamente\nconst BYTE data%s[%lu] = {", varName, sizeVar);
				WriteFile(headerFile, buffer, size, &size, NULL);

				//GAMBIARRA MASTER
				for(int max = 0; sz.QuadPart--; max++)
				{
					if(max != 0)
						WriteFile(headerFile, ", ", 2, &size, NULL);

					if((max % 16) == 0)
						WriteFile(headerFile, "\n\t", 2, &size, NULL);

					ReadFile(file, &byte, 1, &size, NULL);
					snprintf(buffer, 4, "0x%.2x", byte);
					WriteFile(headerFile, buffer, 4, &size, NULL);
				}

				size = snprintf(buffer, MAX_PATH, "};\n\nDWORD size%s = %lu;", varSize, sizeVar);
				WriteFile(headerFile, buffer, size, &size, NULL);
				CloseHandle(headerFile);

				printf(" :: Cabecalho salvo como %s\n", newName);
				printf(" :: data var: data%s\n", varName);
				printf(" :: size var: size%s\n", varSize);
			}
		}
		CloseHandle(file);
	}
}

int main(int argc, char **argv)
{
	masterName = argv[0];
	opterr = 0;

	if(argc < 2)
		usage();

	int gen = 0, opt;
	char *header = NULL;
	while((opt = getopt(argc, argv, "gh:")) != -1)
	{
		switch(opt)
		{
			case 'g':
				gen = 1;
				break;

			case 'h':
				header = optarg;
				break;

			default:
				usage();
				break;
		}
	}

	if(gen)
	{
		gen_rsa_key();
	} else if(header)
	{
		create_header(header);
	}

	return 0;
}