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
#include "logging.h"

tissuestack::logging::TissueStackLogger::TissueStackLogger() : _log_path(LOG_PATH)
{
	// check if path exists and create it if necessary
	if (!tissuestack::utils::System::directoryExists(this->_log_path) &&
		!tissuestack::utils::System::createDirectory(this->_log_path, 0775))
			THROW_TS_EXCEPTION(tissuestack::common::TissueStackServerException,
				"Unable to create the log path!");

	// create and open files
	this->_info_log = fopen(std::string(this->_log_path + "/info.log").c_str(), "a");
	this->_error_log = fopen(std::string(this->_log_path + "/error.log").c_str(), "a");
	this->_debug_log = fopen(std::string(this->_log_path + "/debug.log").c_str(), "a");

	if (this->_info_log == NULL || this->_error_log == NULL || this->_debug_log == NULL)
		THROW_TS_EXCEPTION(tissuestack::common::TissueStackServerException, "Unable to create the log files");

	this->all("TissueStackLogger initialized\n");
};

void tissuestack::logging::TissueStackLogger::purgeInstance()
{
	delete tissuestack::logging::TissueStackLogger::_instance;
	tissuestack::logging::TissueStackLogger::_instance = nullptr;
}

void tissuestack::logging::TissueStackLogger::log(FILE * log_file, const char * log_args, va_list args)
{
	std::lock_guard<std::mutex> lock(this->_log_mutex);

	std::string timestamp = tissuestack::utils::System::getSystemTimeFormatted(TIMESTAMP_FORMAT);
	fprintf(log_file, "[%s]\t ", timestamp.c_str());

	vfprintf(log_file, log_args, args);
	if (log_args && strlen(log_args) > 0 && log_args[strlen(log_args)-1] != '\n')
		fprintf(log_file, "\n");
	fflush(log_file);
}

tissuestack::logging::TissueStackLogger::~TissueStackLogger()
 {
	// close file handles
	fclose(this->_info_log);
	fclose(this->_error_log);
	fclose(this->_debug_log);
};

const bool tissuestack::logging::TissueStackLogger::doesInstanceExist()
{
	return (tissuestack::logging::TissueStackLogger::_instance != nullptr);
}


 tissuestack::logging::TissueStackLogger * tissuestack::logging::TissueStackLogger::instance()
 {
	if (tissuestack::logging::TissueStackLogger::_instance == nullptr)
		tissuestack::logging::TissueStackLogger::_instance = new tissuestack::logging::TissueStackLogger();

	return tissuestack::logging::TissueStackLogger::_instance;
 }

void tissuestack::logging::TissueStackLogger::info(const char * log_args, ...)
{
	va_list args;
	va_start(args, log_args);
	this->log(this->_info_log, log_args, args);
	va_end(args);
}

void tissuestack::logging::TissueStackLogger::error(const char * log_args, ...)
{
	va_list args;
	va_start(args, log_args);
	this->log(this->_error_log, log_args, args);
	va_end(args);
}

void tissuestack::logging::TissueStackLogger::debug(const char * log_args, ...)
{
	va_list args;
	va_start(args, log_args);
	this->log(this->_debug_log, log_args, args);
	va_end(args);
}

void tissuestack::logging::TissueStackLogger::all(const std::string & log_line)
{
	this->info(log_line.c_str());
	this->error(log_line.c_str());
	this->debug(log_line.c_str());
}

const std::string tissuestack::logging::TissueStackLogger::TIMESTAMP_FORMAT = "%Y-%m-%d %H:%M:%S";
tissuestack::logging::TissueStackLogger * tissuestack::logging::TissueStackLogger::_instance = nullptr;
