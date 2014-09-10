#include "networking.h"

const std::string tissuestack::networking::TissueStackServicesRequest::SERVICE = "SERVICES";


tissuestack::networking::TissueStackServicesRequest::~TissueStackServicesRequest() {}

tissuestack::networking::TissueStackServicesRequest::TissueStackServicesRequest(
		std::unordered_map<std::string, std::string> & request_parameters,
		const std::string file_upload_start) : _request_parameters(request_parameters) {

	this->_subService = this->getRequestParameter("SUB_SERVICE", true);
	if (this->_subService.empty())
		THROW_TS_EXCEPTION(tissuestack::common::TissueStackInvalidRequestException,
				"TissueStack Services Request have to have a parameter 'SUB_SERVICE'!");

	this->_file_upload_start = file_upload_start;

	// we have passed all preliminary checks => assign us the new type
	this->setType(tissuestack::common::Request::Type::TS_SERVICES);
}

const bool tissuestack::networking::TissueStackServicesRequest::isObsolete() const
{
	return false;
}

const std::string tissuestack::networking::TissueStackServicesRequest::getRequestParameter(
		const std::string & which, const bool convertUpperCase) const
{
	std::string param = tissuestack::utils::Misc::findUnorderedMapEntryWithUpperCaseStringKey(this->_request_parameters, which);
	if (!param.empty() && convertUpperCase)
		std::transform(param.begin(), param.end(), param.begin(), toupper);

	return param;
}

const std::string tissuestack::networking::TissueStackServicesRequest::getSubService() const
{
	return this->_subService;
}

const std::string tissuestack::networking::TissueStackServicesRequest::getContent() const
{
	return std::string("TS_SERVICES");
}

const std::string tissuestack::networking::TissueStackServicesRequest::getFileUploadStart() const
{
	return this->_file_upload_start;
}
