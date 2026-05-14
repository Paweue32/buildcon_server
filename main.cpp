#define CPPHTTPLIB_OPENSSL_SUPPORT

#include <iostream>
#include <http/httplib.h>
#include "env.cpp"

int main(int argc, char **argv) {
	if(argc != 2) {
		std::cerr << "Usage: ./main [message]" << std::endl;
		return EXIT_FAILURE;
	}

	httplib::SSLServer svr(cert_path, priv_key_path);

	svr.Get("/", [argv](const auto& req, auto& res) {
		res.set_content(argv[1], "text/plain");
	});

	std::cout << "Listening on https://localhost:443" << std::endl;
	svr.listen("0.0.0.0", 443);

	return EXIT_SUCCESS;
}

/*#define CPPHTTPLIB_OPENSSL_SUPPORT

#include <iostream>
#include <fstream>
#include <http/httplib.h>
#include <thread>

using namespace std::chrono_literals;

int main(int argc, char **argv) {
	std::cout << "It runs!" << std::endl;

	if(argc != 2) {
		std::cerr << "Usage: ./main [link]" << std::endl;
		return EXIT_FAILURE;
	}

	httplib::Client cli(argv[1]);
	cli.set_ca_cert_path("/etc/ssl/certs/ca-certificates.crt");
	if(!cli.is_valid()) {
		std::cerr << "Target is invalid" << std::endl;
		return EXIT_FAILURE;
	}
	
	std::cout << "So fat so good" << std::endl;
	httplib::Result res = cli.Get("/");
	std::cout << "Request succeeded 😎" << std::endl;

	if(!res) {
		std::cerr << "Error: " << httplib::to_string(res.error()) << std::endl;

		if(res.error() == httplib::Error::SSLConnection) {
			std::cerr << "OpenSSL Error: " << ERR_get_error() << std::endl;
		}

		return EXIT_FAILURE;
	}

	if(res->status != 200) {
		std::cerr << "Request error\nStatus: " << res->status << std::endl;
	}
	else {
		std::cout << "Money, status 🤑" << std::endl;
	}

	std::ofstream plik("stronka.html");
	plik << res->body;
	plik.close();

	return EXIT_SUCCESS;
}*/
