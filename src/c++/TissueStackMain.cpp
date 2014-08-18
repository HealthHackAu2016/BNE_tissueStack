#include "logging.h"
#include "singleton.h"
#include "database.h"

#include "server.h"
#include "networking.h"
#include "execution.h"
#include <memory>
#include <signal.h>

extern "C"
{
	// global pointer to give signal handler chance to call server stop
	tissuestack::networking::Server<tissuestack::common::TissueStackProcessingStrategy> * _ts_server_instance = nullptr;

	void handle_signals(int sig) {
		tissuestack::LoggerSingleton->info("Signal Handler Received Signal : %i\n", sig);

		switch (sig) {
			case SIGHUP:
			case SIGQUIT:
			case SIGTERM:
			case SIGINT:
			_ts_server_instance->stop();
			break;
		}
	};

	void install_signal_handler(tissuestack::networking::Server<tissuestack::common::TissueStackProcessingStrategy> * ts_server_instance)
	{
		if (ts_server_instance == nullptr) return;
		_ts_server_instance = ts_server_instance;

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
}

int main(int argc, char * args[])
{
	std::unique_ptr<tissuestack::networking::Server<tissuestack::common::TissueStackProcessingStrategy> >
				TissueStackServer;
	try
	{
		// create an instance of a tissue stack server and wrap it in a smart pointer
		TissueStackServer.reset(new tissuestack::networking::Server<tissuestack::common::TissueStackProcessingStrategy>(4242));
	} catch (tissuestack::common::TissueStackException& ex) {
		tissuestack::LoggerSingleton->error(
				"Failed to instantiate TissueStackServer with Default Strategy for the following reason: %s\n",
				ex.what());
		exit(-1);
	} catch (std::exception & bad)
	{
		tissuestack::LoggerSingleton->error(
				"Failed to instantiate TissueStackServer with Default Strategy for unexpected reason:\n%s\n", bad.what());
		exit(-1);
	}

	try
	{
		// install the signal handler
		install_signal_handler(TissueStackServer.get());
	} catch (std::exception & bad)
	{
		tissuestack::logging::TissueStackLogger::instance()->error("Failed to install the signal handlers:\n%s\n", bad.what());
		exit(-1);
	}

	try
	{
		// start the server socket
		TissueStackServer->start();
	} catch (tissuestack::common::TissueStackServerException& ex) {
		tissuestack::LoggerSingleton->error(
				"Failed to start the TissueStack SocketServer for the following reason: %s\n",
				ex.what());
		exit(-1);
	} catch (std::exception & bad)
	{
		tissuestack::LoggerSingleton->error(
				"Failed to start the TissueStack SocketServer for unexpected reason:\n%s\n", bad.what());
		exit(-1);
	}

	// start database connection
	try
	{
		// TODO: there has to 1 text file for database connection parameters that is handed over at start
		if (!tissuestack::database::TissueStackPostgresConnector::instance()->isConnected())
		{
			tissuestack::LoggerSingleton->error("Database is not connected!\n");
			tissuestack::database::TissueStackPostgresConnector::instance()->purgeInstance();
			exit(-1);
		}
		tissuestack::LoggerSingleton->info("Database connection established!\n");
		// TODO: move commented part into 'rest' section. also solidify with catch and tests for database connectivity
		/*
		const std::vector<tissuestack::database::Configuration *> conf =
				tissuestack::database::ConfigurationDataProvider::queryAllConfigurations();
		for (tissuestack::database::Configuration * c : conf)
		{
			tissuestack::LoggerSingleton->debug("%s\n", c->getJson().c_str());
			delete c;
		}
		const tissuestack::database::Configuration * conf =
				tissuestack::database::ConfigurationDataProvider::queryConfigurationById("bla");
		if (conf)
		{
			tissuestack::LoggerSingleton->debug("%s\n", conf->getJson().c_str());
			delete conf;
		}
		*/
	} catch (std::exception & bad)
	{
		tissuestack::LoggerSingleton->error("Could not create databases connection:\n%s\n", bad.what());
		exit(-1);
	}

	// instantiate singletons
	try
	{
		tissuestack::common::RequestTimeStampStore::instance(); // for request time stamp checking
	} catch (std::exception & bad)
	{
		tissuestack::LoggerSingleton->error("Could not instantiate RequestTimeStampStore:\n%s\n", bad.what());
		exit(-1);
	}

	try
	{
		tissuestack::imaging::TissueStackDataSetStore::instance(); // the data set store
		//tissuestack::imaging::TissueStackDataSetStore::instance()->dumpDataSetStoreIntoDebugLog();
	} catch (std::exception & bad)
	{
		tissuestack::common::RequestTimeStampStore::instance()->purgeInstance();
		tissuestack::LoggerSingleton->error("Could not instantiate TissueStackDataSetStore:\n%s\n", bad.what());
		exit(-1);
	}

	try
	{
		tissuestack::imaging::TissueStackLabelLookupStore::instance(); // for label lookups
		//tissuestack::imaging::TissueStackLabelLookupStore::instance()->dumpAllLabelLookupsToDebugLog();
	} catch (std::exception & bad)
	{
		tissuestack::imaging::TissueStackDataSetStore::instance()->purgeInstance();
		tissuestack::common::RequestTimeStampStore::instance()->purgeInstance();
		tissuestack::LoggerSingleton->error("Could not instantiate TissueStackLabelLookupStore:\n%s\n", bad.what());
		exit(-1);
	}

	try
	{
		tissuestack::imaging::TissueStackColorMapStore::instance(); // the colormap store
		//tissuestack::imaging::TissueStackColorMapStore::instance()->dumpAllColorMapsToDebugLog();
	} catch (std::exception & bad)
	{
		tissuestack::imaging::TissueStackLabelLookupStore::instance()->purgeInstance();
		tissuestack::imaging::TissueStackDataSetStore::instance()->purgeInstance();
		tissuestack::common::RequestTimeStampStore::instance()->purgeInstance();
		tissuestack::LoggerSingleton->error("Could not instantiate TissueStackLabelLookupStore:\n%s\n", bad.what());
		exit(-1);
	}

	try
	{
		InitializeMagick("/tmp");
		// accept requests and process them until we receive a SIGSTOP
		TissueStackServer->listen();
	} catch (std::exception & bad)
	{
		if (!TissueStackServer->isStopping())
			tissuestack::LoggerSingleton->error("TissueStackServer listen() was aborted for the following reason:\n%s\n", bad.what());
		TissueStackServer->stop();
		exit(-1);
	}
}