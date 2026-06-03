# Library server for the BuildConnector project

BuildConnector is a library manager for C++. The server's role in the project is to provide the client with libraries' source code as well as instructions for their installation. However, the server is highly customizable and can allow you to automatically resolve dependency graphs and manage downloads for any package manager you make.

To learn more about the BuildConnector project, please refer to the repository [here](https://github.com/Paweue32/BuildConnector).

## Version

- **Version:** 1.2 (Stable)

## Requirements

- **C++ compiler** (supporting C++17 or higher)
- **The Boost Library** (version 1.75 or higher)
- **yhirose/cpp-httplib**
- **OpenSSL**
- **A certificate and private key for HTTPS** (preferably from a certificate authority)

## Setup

> ⚠️ **Important:** The `dependencies.json` file, the `libs/` directory, and the `cert_config.json` file must all exist in the same directory where your executable file is located.

Create a file named `dependencies.json`, where you will store the direct requirements for your packages. The content should be an object where keys are the package names. The values assigned to them can come in 2 forms:

1. String - If the package cannot be downloaded independently and is a part of a larger package, but you still want your users to be able to refer to the sub-package, this value should represent the name of its parent package. Keep in mind that the parent package also needs to be defined in this file.

1. Array of strings - If the package is a standalone one rather than a module, you should opt for this. Each string in the array should represent a **direct** dependency that your package has. Don't worry about deeper dependency layers, the server will construct a proper graph on its own. Specifically, if a package has no dependencies, this should be an empty array.

### Example
```json
{"boost" : [],
 "httplib" : [],
 "openssl" : [],
 "gtest" : [],
 "glfw" : [],
 "glew" : [],
 "wt" : ["boost"],
 "gmock" : "gtest",
 "crypto" : "openssl"}
```

<br>

Apart from the `dependencies.json` file, you will need a `libs/` directory where your packages' contents will reside. Each key from `dependencies.json` should have a corresponding compressed archive here (aside from the sub-packages that don't exist as standalone ones). The package files should follow the naming convention of `[package_name].tar.gz`.

### Example
```
libs/
├── boost.tar.gz
├── glew.tar.gz
├── glfw.tar.gz
├── gtest.tar.gz
├── httplib.tar.gz
├── openssl.tar.gz
└── wt.tar.gz
```

<br>

Lastly, create an `cert_config.json` file where you will store the paths to your server's TLS certificate and private key. The names of the keys must match those in the example below. **Make sure this file is included in your project's** `.gitignore` **file!** 

### Example
```json
{
    "cert_path" : "/path/to/cert.pem",
    "priv_key_path" : "/path/to/privkey.pem"
}
```

<br>

## Compilation

After completing the setup tasks, compile the `main.cpp` file using your preferred compiler, but make sure you use the C++17 standard (or higher). You will need to link against `libboost_json`, `libssl` and `libcrypto`. Even though the last two aren't explicitly included in `main.cpp`, they are required for `yhirose/cpp-httplib`'s https features. Keep in mind that `libcrypto` is typically bundled with `libssl`, so you may not need to install it separately. Since `yhirose/cpp-httplib` is a single-file, header-only library, you won't need to link against it, but `httplib.h` needs to be present in one of your system's include directories. If you are including/linking libraries located in non-standard locations, additional flags may be required. The resulting executable's name does not matter.

### Example
```bash
g++ main.cpp -o server -std=c++17 -lboost_json -lssl -lcrypto
```

<br>

## Usage

After a successful compilation, run the resulting executable. Keep in mind that if you don't have read access to `cert_path` and `priv_key_path` files, the server will not run properly. Additionally, you may need root/admin privileges to start a server on ports 80 and 443. Upon a successful launch, the server should print `Listening on http://localhost:80 and https://localhost:443` to standard output.

**Disclaimer:** the dependency graph is built right after starting the server, so you don't need to recompile `main.cpp` upon changing `dependencies.json`. The same principle applies when changing `cert_config.json`.

Port 443 houses the https server that processes user requests while port 80 has an http server that simply redirects to 443, preserving the requests in their entirety.

From now on the https server will handle two kinds of requests:


### 1. POST requests on path /scout

This API takes a JSON object with the names of packages requested by users. It will search the dependency graph and send back a list of all the packages the client needs to acquire (including the original ones). Keep in mind that this process does not actually send any packages to the client, it only resolves dependencies (hence the /scout path). The input JSON object must contain a field of key `items`, where the value is an array of strings - desired packages. The packages may repeat.

### Example
```json
{
    "items" : ["gmock", "wt", "openssl", "glew", "openssl"]
}
```

<br>

The output data's format is identical, but it guarantees no duplicates and a reverse topological order, which means that for each package, all of its dependencies, if they exist, are located to its left. This ensures that installing the packages from left to right won't cause dependency errors. The server also swaps out sub-packages for their parents.
### Example
```json
{
    "items" : ["boost", "gtest", "wt", "openssl", "glew"]
}
```

<br>

### 2. GET requests on path /download

This API takes the name of the package to be downloaded as the `item` parameter. If the request succeeds, the server will immediately begin streaming the desired package. Otherwise, it will send an error code as a JSON file. Keep in mind that this process only serves one file at a time and doesn't resolve dependencies, so it is advisable to handle that with the aforementioned POST API.

### Example
```
https://[server_name/ip]/download?item=boost
```