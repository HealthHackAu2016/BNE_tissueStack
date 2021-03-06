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
#include "database.h"

const bool tissuestack::database::SessionDataProvider::addSession(
	const std::string session, const unsigned long long int expiry_in_millis)
{
	try
	{
		if (session.empty() || expiry_in_millis == 0) return false;

		const std::string sql =
			std::string(
			"INSERT INTO session (id, expiry) VALUES('")
				+ session + "',"
				+ std::to_string(expiry_in_millis)
				+ ");";
		if (tissuestack::database::TissueStackPostgresConnector::instance()->executeTransaction({sql}) == 1)
			return true;
	}
	catch(const std::exception & bad)
	{
		tissuestack::logging::TissueStackLogger::instance()->error("Failed to store session: %s\n", bad.what());
	}
	return false;
}

const bool tissuestack::database::SessionDataProvider::hasSessionExpired(
	const std::string session, const unsigned long long int now, const unsigned long long int extension)
{
	if (session.empty()) return true;

	try
	{
		std::string sql =
			std::string("SELECT * FROM session WHERE id='")
				+ session + "';";
		const pqxx::result result =
			tissuestack::database::TissueStackPostgresConnector::instance()->executeNonTransactionalQuery({sql});
		if (result.size() != 1) return true;

		const unsigned long long int present_expiry = result[0]["expiry"].as<unsigned long long int>();
		if (present_expiry < now)
		{
			tissuestack::database::SessionDataProvider::invalidateSession(session);
			return true;
		}

		if (extension == 0) return false;

		try
		{
			sql =
					std::string("UPDATE session SET expiry=")
					+ std::to_string(now+extension)
					+ " WHERE id='"
					+ session + "';";
			tissuestack::database::TissueStackPostgresConnector::instance()->executeTransaction({sql});

			return false;
		} catch(const std::exception & bad)
		{
			// we can ignore that
		}
	}
	catch(const std::exception & bad)
	{
		tissuestack::logging::TissueStackLogger::instance()->error("Failed to check session expiry: %s\n", bad.what());
	}
	return true;
}

const bool tissuestack::database::SessionDataProvider::invalidateSession(
	const std::string session)
{
	if (session.empty()) return false;

	try
	{
		const std::string sql =
				std::string("DELETE FROM session WHERE id='")
				+ session + "';";
		if (tissuestack::database::TissueStackPostgresConnector::instance()->executeTransaction({sql}) == 1)
			return true;
	} catch(const std::exception & bad)
	{
		tissuestack::logging::TissueStackLogger::instance()->error("Failed to invalidate session: %s\n", bad.what());
	}
	return false;
}

void tissuestack::database::SessionDataProvider::deleteSessions(
		const unsigned long long int expiry_in_millis)
{
	try
	{
		const std::string sql =
				std::string("DELETE FROM session WHERE expiry < ")
				+ std::to_string(expiry_in_millis) + ";";
		tissuestack::database::TissueStackPostgresConnector::instance()->executeTransaction({sql});
	} catch(const std::exception & bad)
	{
		tissuestack::logging::TissueStackLogger::instance()->error("Failed to clean up expired sessions: %s\n", bad.what());
	}
}
