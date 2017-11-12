/*
 *	Nickolas Pylarinos
 *	runnix.cpp
 *
 *	runnix is a console utility for running
 *	linux apps on Windows with just a double click!
 */

/* includes */
#include <process.h>
#include <winapifamily.h>
#include <wtypes.h>
#include <wslapi.h>
#include <iostream>
#include <string>

/* defines */
#define SW_VERSION "0.1"

/* using */
using namespace std;

void print_usage(void)
{
	/*
	*	Supported options
	*	-l	run a command that exists in a path in WSL
	*	-w	run a command that exists in a path in Windows ( default case )
	*/

	printf("Usage: runnix [command] [options]\n\n");
	printf("command: linux executable or script stored outside WSL\n");
	printf("options:\n\n\t-h	Print usage\n");
}

/**
 *	get_unix_path
 *
 *	gets path in ntfs form and converts it to unix form with
 *	the appropriate prefix to make it usable for bash
 */
string get_unix_path(string path)
{
	/* Windows c drive prefix */
	string windows_c = "/mnt/c";
	string ntfs_path = "";
	string unix_path = "";

	/* copy contents of path to ntfs_path */
	ntfs_path = path;

	/* Remove the "C:" from ntfs_path */
	ntfs_path = ntfs_path.substr(2, ntfs_path.length());

	/* Replace every '\' with '/' */
	for (unsigned i = 0; i < ntfs_path.length(); i++)
	{
		if (ntfs_path[i] == '\\')
			ntfs_path[i] = '/';
	}

	/* Create the unix_path */
	unix_path = windows_c + ntfs_path;

	return unix_path;
}

int main(int argc, char * argv[])
{
	printf("runnix v%s | by npyl\n\n", SW_VERSION);

	if (argc < 1) {
		printf("Too few arguments!\n");
		return 1;
	}

	if (argc > 1)
	{
		/* Get unix path ( -l ) */
		//string executable = argv[1];

		/* Get unix path ( -w ) */
		string executable = get_unix_path(argv[1]);
		int res = 0;
		
		/*	run executable with correct parameters */
		WslLaunch(L"ubuntu", L"conky", 1, 0, 0, 0, nullptr);
//		res = _execl("C:\\Windows\\System32\\bash.exe", "-c", executable.c_str());

		/* Check result */
		if (res > 0)
			printf("runnix: Run successfully using awesome posix APIs!\n");
		else {
			printf("runnix: couldn't run %s. Error %i: ", executable.c_str(), res);
			perror(NULL);
			printf("\n");
		}
	}

	getchar();
	return 0;
}