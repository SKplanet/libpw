#include "./mycommon.h"

void help(void)
{
	cout << g_name << " since " << g_start << endl;
	cout << g_version << endl;
	cout << endl;
	cout << "[OPTIONS...]" << endl;
	cout << "  -h Help screen" << endl;
	cout << "  -V Print version: " << g_version << endl;
	cout << "  -v Verbose" << endl;
}

bool getOpts(int argc, char* argv[])
{
	int opt;
	const auto optstr = "hVv";

	while ( -1 not_eq (opt = getopt(argc, argv, optstr)))
	{
		switch(opt)
		{
		case 'h':
		{
			help();
			exit(EXIT_SUCCESS);
			break;
		}
		case 'V':
		{
			cout << g_version;
			exit(EXIT_SUCCESS);
			break;
		}
		case 'v':
		{
			pw::Log::s_setTrace(true);
			break;
		}//case 'v'
		}//switch(opt)
	}// while
}

int main(int argc, char* argv[])
{
	PWINIT();
	pw::Log::s_setTrace(false);

	if ( not getOpts(argc, argv) ) return EXIT_FAILURE;

	/*
	 * Your logic here!
	 */

	return EXIT_SUCCESS;
}
