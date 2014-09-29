/*
 * This file is part of TissueStack.
 *
 * TissueStack is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * TissueStack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with TissueStack.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "tissuestack.h"
#include "execution.h"
#include "networking.h"
#include "imaging.h"
#include "services.h"

#include <signal.h>
#include <sys/prctl.h>
#include <getopt.h>
#include <sys/wait.h>

std::unique_ptr<tissuestack::execution::TissueStackOfflineExecutor> OfflineExecutor(
		tissuestack::execution::TissueStackOfflineExecutor::instance());

static std::string out_file = "";
static pid_t parent = -1;
static std::vector<pid_t> pids = {-1,-1,-1};
static short number_of_children_running = 0;

void handle_signals(int sig) {
	switch (sig) {
		case SIGHUP:
			if (getpid() == parent) break;
		case SIGINT:
		case SIGQUIT:
		case SIGABRT:
		case SIGKILL:
		case SIGTERM:
			if (getpid() == parent)
			{
				sleep(2);
				std::cerr << "\nConversion Process Received Crtl + C!\n" << std::flush;
			}
			else
				kill(parent, SIGINT);
			if (!out_file.empty())
				unlink(out_file.c_str());
			exit(EXIT_FAILURE);
		case SIGCHLD:
			if (getpid() == parent)
			{
				number_of_children_running--;
				break;
			}
			if (!out_file.empty())
				unlink(out_file.c_str());
			exit(EXIT_FAILURE);
		default:
			break;
	}
};

void install_signal_handler()
{
	struct sigaction act;
	int i;

	act.sa_handler = handle_signals;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	i = 1;
	while (i < 32) {
		if (i != 11)
			sigaction(i, &act, nullptr);
		i++;
	}
};

int		main(int argc, char **argv)
{
	parent = getpid();

	std::string in_file = "";

	int c = 0;
	while (1)
	{
		static struct option long_options[] = {
			{"in",  required_argument, 0, 'i'},
			{"out", required_argument, 0, 'o'},
			{0, 0, 0, 0}
		};

		int option_index = 0;
		c = getopt_long (argc, argv, "i:o:", long_options, &option_index);
		if (c == -1)
			break;

		const std::string tmp =
			(optarg == NULL) ?
				"" :
				std::string(optarg, strlen(optarg));

		switch (c)
		{
			case 'i':
				in_file = tmp;
				break;

			case 'o':
				out_file = tmp;
				break;

			case '?':
				exit (0);   /* getopt_long already printed an error message. */
				break;

			default:
				std::cout << "Usage: " << argv[0] <<
					" -i IN_FILE (*.mnc,*.nii,*.nii.gz) -o OUT_FILE\n";
				exit(0);
		}
	}

	// check for mandatory params
	if (in_file.empty() || out_file.empty())
	{
		std::cerr << "Usage: " << argv[0] <<
			" -i IN_FILE (*.mnc,*.nii,*.nii.gz) -o OUT_FILE\n";
		exit(-1);
	}

	try
	{
		// install the signal handler
		install_signal_handler();
	} catch (std::exception & bad)
	{
		std::cerr << "Failed to install signal handlers!" << std::endl;
		exit(-1);
	}

	try
	{
		InitializeMagick(NULL);

		// check out the image data first and see how many dimensions we have
		std::vector<std::string> dimensions;
		tissuestack::services::TissueStackConversionTask * conversion = nullptr;

		try
		{
		   conversion =
				new tissuestack::services::TissueStackConversionTask(
					"0",
					in_file,
					out_file);
		   if (conversion)
		   {
			   dimensions = conversion->getInputImageData()->getDimensionOrder();
			   if (dimensions.empty()) // check if we have dimensions
			   {
					std::cerr << "Failed to convert: Data Set has 0 dimensions!" << std::endl;
					exit(EXIT_FAILURE);
			   }

			   // now touch a file to be as big as we need it for the final conversion product
			   if (tissuestack::utils::System::fileExists(out_file))
			   {
				   std::cerr <<
						"Failed to convert: Unable to touch future RAW file because of already existing file!"
						<< std::endl;
					exit(EXIT_FAILURE);
			   }
		   }
		} catch (const std::exception & any)
		{
			if (conversion) delete conversion;

			std::cerr << "Failed to convert: " << any.what() << std::endl;
			exit(EXIT_FAILURE);
		}

		// our strategy is that we use processes in cases where there are at least 3 cores
		if (dimensions.size() < 3 || tissuestack::utils::System::getNumberOfCores() < 3)
		{
			try
		   {
			   // delegate to the offline executor
				OfflineExecutor->convert(conversion);
				exit(EXIT_SUCCESS);
		   } catch (const std::exception & any)
			{
				if (conversion) delete conversion;

				std::cerr << "Failed to convert: " << any.what() << std::endl;
				exit(EXIT_FAILURE);
			}
		}

		// now touch a file to be as big as we need it for the final conversion product
		std::cout << "Touching file for multi-process conversion (1 process per dimension)..." << std::endl;
		if (!tissuestack::utils::System::touchFile(
				out_file, conversion->getFutureRawFileSize()))
		{
			std::cerr << "Failed to convert: Unable to touch future RAW file!" << std::endl;
			exit(EXIT_FAILURE);
		}
		std::cout << "Created raw file for multi-process conversion." << std::endl;

		// we fork for each dimension
		number_of_children_running =
			dimensions.size() > pids.size() ? pids.size() : dimensions.size();
		for (unsigned short i=0; i<number_of_children_running;i++)
		{
		   pids[0] = fork();
		   if (pids[0] == -1) // ERROR
		   {
			   std::cerr << "Failed to fork child processes to convert data!" << std::endl;
			   unlink(out_file.c_str());
			   if (conversion) delete conversion;
			   exit(EXIT_FAILURE);
		   }
		   else if (pids[0] == 0) // CHILD
		   {
			   prctl(PR_SET_PDEATHSIG, SIGHUP);

			   try
			   {
				   // delegate to the offline executor
					OfflineExecutor->convert(
						conversion,
						dimensions[i],
						i == 0 ? true : false); // first dimension will write header
					exit(EXIT_SUCCESS);
			   } catch (const std::exception & any)
				{
					std::cerr << "Failed to convert: " << any.what() << std::endl;
					exit(EXIT_FAILURE);
				}
		   }
		}

		// ONLY PARENT IS ALLOWED TO GET TO HERE !

		// the periodic check whether the children have terminated or not
		while (number_of_children_running > 0)
			sleep(1);
		 std::cout << "\nConversion finished successfully." << std::endl;
	} catch (const std::exception & any)
	{
		std::cerr << "Failed to convert: " << any.what() << std::endl;
	}
	exit(0);
}