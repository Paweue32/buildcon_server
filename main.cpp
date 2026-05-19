#define CPPHTTPLIB_OPENSSL_SUPPORT

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <queue>
#include <filesystem>
#include <http/httplib.h>
#include <boost/json.hpp>
#include "env.cpp"

namespace fs = std::filesystem;

std::map<std::string, std::vector<std::string>> dependency_graph;
std::map<std::string, std::string> master_pack;
void load_dependency_graph();

int main(int argc, char **argv) {
	// Test accessibility for cert_path and priv_key_path
	try {
		std::ifstream test_file_object;
		
		test_file_object.open(cert_path);
		if(!test_file_object.good()) {
			throw std::runtime_error("test_file_object.good() returned false for cert_path");
		}
		test_file_object.close();

		test_file_object.open(priv_key_path);
		if(!test_file_object.good()) {
			throw std::runtime_error("test_file_object.good() returned false for priv_key_path");
		}
		test_file_object.close();
	} catch(const std::exception& e) {
		std::cerr << "Cannot open cert_path or priv_key_path.\nDetails: " << e.what() << std::endl;
		return EXIT_FAILURE; 
	}

	// Setup
	load_dependency_graph();
	httplib::SSLServer svr(cert_path, priv_key_path);

	svr.Post("/scout", [](const httplib::Request& req, httplib::Response& res) {
		// Checking if everything's alright
        if (req.get_header_value("Content-Type") != "application/json") {
            res.status = 400;
            res.set_content("{\"error\": \"Invalid Content-Type. Expected application/json\"}", "application/json");
            return;
        }

        try {
            boost::json::value received_data = boost::json::parse(req.body);

			if(!received_data.is_object()) {
				res.status = 400;
				res.set_content("0", "application/json");
				return;
			}

			boost::json::object request_content = received_data.as_object();

            if (request_content.count("items") && request_content["items"].is_array()) {
				boost::json::array request_items = request_content["items"].as_array();

				// Building the output JSON array with BFS
				boost::json::array response_items;
				std::queue<std::string> kolej;
				std::set<std::string> seen;

				for(const boost::json::value& item: request_items) {
					if(!item.is_string()) {
						res.status = 400;
						res.set_content("1", "application/json");
						return;
					}

					std::string s_item = std::string(item.as_string());
					if(master_pack.count(s_item)) {
						if(seen.count(master_pack[s_item])) {
							continue;
						}
						
						kolej.push(master_pack[s_item]);
						seen.insert(master_pack[s_item]);
						response_items.push_back(boost::json::string(master_pack[std::move(s_item)]));
					}
					else if(dependency_graph.count(s_item)) {
						if(seen.count(s_item)) {
							continue;
						}

						kolej.push(s_item);
						seen.insert(s_item);
						response_items.push_back(boost::json::string(std::move(s_item)));
					}
					else {
						res.status = 400;
						res.set_content("2", "application/json");
						return;
					}
				}

				while(kolej.size()) {
					for(const std::string& requirement: dependency_graph.at(kolej.front())) {
						if(seen.count(requirement)) {
							continue;
						}

						seen.insert(requirement);
						kolej.push(requirement);
						response_items.push_back(boost::json::string(requirement));
					}
					kolej.pop();
				}


                // Finalizing and sending the JSON response
                boost::json::object response_object = {{"items", std::move(response_items)}};
                res.status = 200;
                res.set_content(boost::json::serialize(response_object), "application/json");
            } else {
                res.status = 400;
                res.set_content("3", "application/json");
            }

        } catch (...) {
            res.status = 500;
            res.set_content("4", "application/json");
        }
    });


	svr.Get("/download", [](const httplib::Request& req, httplib::Response& res) {
		if(!req.has_param("item")) {
			res.status = 400;
			res.set_content("5", "application/json");
			return;
		}

		std::string item_param = req.get_param_value("item");

		try {
			if(!dependency_graph.count(item_param)) {
				res.status = 400;
				res.set_content("6", "application/json");
				return;
			}
			
			fs::path file_to_stream(std::string("libs/") + item_param + ".tar.gz");

			if(!fs::exists(file_to_stream)) {
				res.status = 400;
				res.set_content("7", "application/json");
				return;
			}

			res.status = 200;
			res.set_file_content(file_to_stream.string(), "application/gzip");
		} catch(...) {
			res.status = 500;
			res.set_content("8", "application/json");
		}
	});


	std::cout << "Listening on https://localhost:443" << std::endl;
	svr.listen("0.0.0.0", 443);

	return EXIT_SUCCESS;
}



void load_dependency_graph() {
	try {
		std::ifstream file("dependencies.json");
		boost::json::object connections = boost::json::parse(file).as_object();
		for(auto &[lib, deps]: connections) {
			if(deps.is_string()) {
				master_pack[std::string(lib)] = std::string(deps.as_string());
				continue;
			}

			auto dep_vec = deps.as_array();
			std::vector<std::string>& ref = dependency_graph[std::string(lib)];
			ref.reserve(dep_vec.size());
			for(auto &element: dep_vec) {
				ref.push_back(std::string(element.as_string()));
			}
		}
		file.close();
	}
	catch(const std::exception& e) {
		std::cerr << "Error: Failed to load dependencies into memory" << std::endl << e.what() << std::endl;
		exit(EXIT_FAILURE);
	}
}