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