#include <iostream>

#include "http_connection.h"
#include "../settings.h"


std::string settingsFileName = "settings.ini";
Settings& settings = Settings::init();
std::vector<std::string> appSettings;
std::unique_ptr<Database> database;

void httpServer(tcp::acceptor& acceptor, tcp::socket& socket)
{
	acceptor.async_accept(socket,
		[&](beast::error_code ec)
		{
			if (!ec)
				std::make_shared<HttpConnection>(std::move(socket))->start();
			httpServer(acceptor, socket);
		});
}


int main(int argc, char* argv[])
{
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);

	try
	{
		settings.setFileName(settingsFileName);
		appSettings = std::move(settings.readSettings());
		auto const address = net::ip::make_address(appSettings[HTTPServer_Address]);
		unsigned short port = std::stoi(appSettings[HTTPServer_Port]);
		database = std::make_unique<Database>(appSettings[DB_Host], appSettings[DB_Port], appSettings[DB_DBName], appSettings[DB_Username], appSettings[DB_Password]);

		net::io_context ioc{ 1 };

		tcp::acceptor acceptor{ ioc, { address, port } };
		tcp::socket socket{ ioc };
		HttpConnection::setDatabase(database.get());
		httpServer(acceptor, socket);

		std::cout << "Open browser and connect to http://localhost:" + std::to_string(port) + " to see the web server operating" << std::endl;

		ioc.run();
	}
	catch (std::exception const& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}