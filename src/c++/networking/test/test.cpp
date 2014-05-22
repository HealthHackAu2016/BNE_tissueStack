#include "networking.h"

int main(int argc, char * args[])
{
  //tissuestack::networking::Server server;

  try
  {
	  /*
	  std::unique_ptr<tissuestack::utils::Timer> t(tissuestack::utils::Timer::getInstance(tissuestack::utils::Timer::Type::CLOCK_GET_TIME));
	  t->start();
	  server.start();
	  server.stop();
	  std::cout << "Time: " << static_cast<double>(t->stop()) << std::endl;
	   */

	  // NOTE: try templated RequestFilter with in and out type handed in: applyFilter takes in and spits out out
	  std::vector<std::shared_ptr<tissuestack::common::RequestFilter> > filter_chain
	  {
		  std::shared_ptr<tissuestack::common::RequestFilter>(new tissuestack::networking::HttpRequestSanityFilter())
	  };

	  std::unique_ptr<std::string> someInput(new std::string("GET dsdfd/?hehe=&=lala=&hihi=99=hshsh=lol  HTTP/1.1"));
	  std::unique_ptr<const tissuestack::common::Request> req(new tissuestack::networking::RawHttpRequest(someInput.get()));
	  for (auto filter : filter_chain) {
		  req.reset(filter->applyFilter(req.get()));
	  }
	  const tissuestack::networking::HttpRequest * const result =
			  static_cast<const tissuestack::networking::HttpRequest * const>(req.get());
	  std::cout << "Content: *" << *result->getContent() << "*" << std::endl;
	  std::cout << "Dump: " << result->dumpParameters() << std::endl;

	  /*
	  tissuestack::common::TissueStackProcessingStrategy * TissueStackProcessingStrategy =
			  new tissuestack::common::TissueStackProcessingStrategy();
	  const tissuestack::common::RequestProcessor<tissuestack::common::TissueStackProcessingStrategy> * const TissueStackProcessor =
			  tissuestack::common::RequestProcessor<tissuestack::common::TissueStackProcessingStrategy>::instance(TissueStackProcessingStrategy);
	  TissueStackProcessor->init();

	  delete TissueStackProcessor;
	  */
  }  catch (tissuestack::common::TissueStackException& ex)
  {
	  std::cerr << ex.what() << std::endl;
	  return -1;
  }  catch (std::exception& bad)
  {
	  std::cerr << "Something bad happened: " << bad.what() << std::endl;
	  return -1;
  }

  return 0;
}
