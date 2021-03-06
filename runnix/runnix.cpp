/*
 *	Nickolas Pylarinos
 *	runnix.cpp
 *
 *	runnix is a console utility for running
 *	linux apps without opening the WSL
 *
 *	AKNOWLEDGEMENTS:
 *		Many thanks to GitHub user "0xbadfca11"
 *		for his code related to getting the WslLaunch function
 *		from the wslapi.dll.
 */

/*
	TODO:
	1) implement elf support in explorer.exe
*/

/* includes */
#include <SDKDDKVer.h>
#include <process.h>
#include <windows.h>
#include <atlbase.h>
#include <wslapi.h>
#include <iostream>
#include <vector>
#include <string>

/* defines */
#define SW_VERSION			"1.1"
#define DEFAULT_DISTRO		"Ubuntu"

#define WIN32_LEAN_AND_MEAN
#define _ATL_NO_AUTOMATIC_NAMESPACE

/* using */
using namespace std;

/**
 *	print_usage
 *	
 *	lets not document this...
 */
void print_usage(void)
{
	printf("runnix v%s | by npyl\n\n", SW_VERSION);
	printf("Usage: runnix [executable] --args1 [arg1a] [arg1b] ... --args2 [arg2a] [arg2b] ... --options [option1] [option2] ...\n\n");
	printf("executable:\tlinux executable or script stored outside WSL\n");
	printf("args1:\t\tstuff you can pass to bash such as DISPLAY=:0.0 before executable\n");
	printf("args2:\t\tthe arguments to be passed to your executable\n");
	printf("options:\tspecific flags to be passed to runnix\n\n");
	printf("valid options are:\n");
	printf("-h\t\tprint usage\n");
	printf("-d [str]\tallows you to set the distro you want to run your executable in.\n");
	printf("\t\tNote: [str] must be a unique name representing a distro e.g. Fabrikam.Distro.10.01 or Ubuntu.  Default is Ubuntu.\n");
}

/**
 *	parse_arguments
 *
 *	Custom **homemade** arguments parser for runnix
 *
 *	Switches order:
 *	runnix [executable] --args1 ... --args2 ... --options ...
 *
 *	Rules:
 *	You CANT change the order of the switches
 */
void parse_arguments(string &executable, vector<char*> &args1, vector<char*> &args2, vector<char*> &options, int argc, char ** argv)
{
	bool haveArgs1 = false;		int firstArgs1Index = 0;
	bool haveArgs2 = false;		int firstArgs2Index = 0;
	bool haveOptions = false;	int firstOptionIndex = 0;
	
	executable = argv[1];

	/* Search the arguments list for switches */
	for (int i = 0; i < argc; i++)
	{
		if (strcmp(argv[i], "--args1") == 0)
		{
			haveArgs1 = true;
			firstArgs1Index = i + 1;
		}
		if (strcmp(argv[i], "--args2") == 0)
		{
			haveArgs2 = true;
			firstArgs2Index = i + 1;
		}
		if (strcmp(argv[i], "--options") == 0)
		{
			haveOptions = true;
			firstOptionIndex = i + 1;
		}
	}

	/* Check all valid cases */
	if (haveArgs1 && haveArgs2 && haveOptions)
	{
		for (int i = firstArgs1Index; i < firstArgs2Index - 1; i++)
		{
			args1.push_back(argv[i]);
		}
		for (int i = firstArgs2Index; i < firstOptionIndex - 1; i++)
		{
			args2.push_back(argv[i]);
		}
	}
	else if (haveArgs1 && haveArgs2)
	{
		for (int i = firstArgs1Index; i < firstArgs2Index - 1; i++)
		{
			args1.push_back(argv[i]);
		}
		for (int i = firstArgs2Index; i < argc; i++)
		{
			args2.push_back(argv[i]);
		}
	}
	else if (haveArgs1 && haveOptions)
	{
		for (int i = firstArgs1Index; i < firstOptionIndex - 1; i++)
		{
			args1.push_back(argv[i]);
		}
	}
	else if (haveArgs2 && haveOptions)
	{
		for (int i = firstArgs2Index; i < firstOptionIndex - 1; i++)
		{
			args2.push_back(argv[i]);
		}
	}
	else if (haveArgs1)
	{
		for (int i = firstArgs1Index; i < argc; i++)
		{
			args1.push_back(argv[i]);
		}
	}
	else if (haveArgs2)
	{
		for (int i = firstArgs2Index; i < argc; i++)
		{
			args2.push_back(argv[i]);
		}
	}
	else {
		/* no other switches */
		/* OK. */
	}

	/* Get options */
	if (haveOptions)
	{
		for (int i = firstOptionIndex; i < argc; i++)
			options.push_back(argv[i]);
	}
}

/**
 *	parse_options
 *
 *	parse runnix specific options
 */
void parse_options(vector<char*> &options, bool &printUsage, bool &customDistro, string &distroName)
{
	/* check if options is empty */
	if (options.size() == 0)
		return;

	/* parsing here */
	for (int i = 0; i < options.size(); i++)
	{
		if (strcmp(options[i], "-h") == 0)
		{
			printUsage = true;
			break;
		}

		if (strcmp(options[i], "-d") == 0)
		{
			customDistro = true;
			distroName = options[i + 1];
		}
	}
}

/**
 *	s2ws
 *
 *	convert std::string to PCWSTR
 *	taken from https://stackoverflow.com/questions/27220/how-to-convert-stdstring-to-lpcwstr-in-c-unicode
 */
std::wstring s2ws(const std::string& s)
{
	int len;
	int slength = (int)s.length() + 1;
	len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
	wchar_t* buf = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
	std::wstring r(buf);
	delete[] buf;
	return r;
}

/**
 *	get_unix_path
 *
 *	gets path in ntfs form and converts it to unix form with
 *	the appropriate prefix to make it usable for bash
 */
string get_unix_path(string path)
{
	char windows_drive = '\0';
	string drive_prefix = "/mnt/";
	string temp_path  = "";
	
	/* copy contents of path to tmp_path */
	temp_path = path;

	/* identify Windows drive */
	windows_drive = temp_path[0];

	/* set drive prefix */
	drive_prefix += tolower(windows_drive);

	/* apply correct prefix to tmp_path */
	temp_path = drive_prefix + temp_path.substr(2, temp_path.length());

	/* Replace every '\' with '/' */
	for (unsigned i = 0; i < temp_path.length(); i++)
	{
		if (temp_path[i] == '\\')
			temp_path[i] = '/';
	}

	return temp_path;
}

/**
 *	construct_command
 *
 *	constructs command to be executed in WSL by WslLaunch
 *	by combining args1, args2, runnix options and executable's 
 *	path
 */
string construct_command(string &executable, vector<char*> &args1, vector<char*> &args2)
{
	string command = "";

	/* add the arguments that go before the executable/script */
	for (int i = 0; i < args1.size(); i++)
	{
		command += args1.at(i);
		command += " ";
	}

	/* add the executable */
	command += executable;
	command += " ";

	/* add the arguments that go after the executable */
	for (int i = 0; i < args2.size(); i++)
	{
		command += args2.at(i);
		command += " ";
	}
	
	return command;
}

/**
 *	WslExecute
 *
 *	Execute a command inside the WSL
 */
ULONG WslExecute(PCWSTR distro, PCWSTR command)
{
	ULONG res = 1;

	if (auto WslLaunchPtr = AtlGetProcAddressFn(LoadLibraryExW(L"wslapi", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32), WslLaunch))
	{
		HANDLE	proc	= nullptr,
				stdIn	= GetStdHandle(STD_INPUT_HANDLE),
				stdOut	= GetStdHandle(STD_OUTPUT_HANDLE),
				stdErr	= GetStdHandle(STD_ERROR_HANDLE);

		res = WslLaunchPtr(distro, command, FALSE, stdIn, stdOut, stdErr, &proc);
		
		/* close handle */
		CloseHandle(proc);
	}

	return res;
}

int main(int argc, char * argv[])
{
	if (argc < 2) {
		printf("runnix: too few arguments\n");
		print_usage();
		return 1;
	}

	string executable;
	vector<char*> args1;
	vector<char*> args2;
	vector<char*> options;
	string command;

	bool printUsage = false;
	bool customDistro = false;
	string distroName = DEFAULT_DISTRO;

	int res = 0;

	/* Parse arguments */
	parse_arguments(executable, args1, args2, options, argc, argv);

	/* Parse runnix options */
	parse_options(options, printUsage, customDistro, distroName);
	
	/* Print Usage and exit */
	if (printUsage)
	{
		print_usage();
		return 0;
	}
	
	/* Get unix path */
	executable = get_unix_path(executable);

	/* Create command to execute in WSL */
	command = construct_command(executable, args1, args2);

	/* Run executable */
	res = WslExecute(s2ws(distroName).c_str(), s2ws(command).c_str());

	/* Check result */
	if (res != S_OK)
	{
		printf("runnix: couldn't run %s. Error: ", executable.c_str());
		perror(NULL);
		printf("\n");
		return 1;
	}
	
	return 0;
}