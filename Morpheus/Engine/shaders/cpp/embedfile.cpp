#include <string>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <unordered_map>
#include <set>

namespace fs = std::filesystem;

using namespace std;

set<string> important_ext = {
	".psh",
	".vsh",
	".json",
	".hlsl",
	".csh"
};

void write_into_lookup(const fs::__cxx11::directory_entry& path,
	unordered_map<string, string>* map, ofstream& out) {
	string name = string("g_") + path.path().filename().c_str() + "_data";
	std::replace(name.begin(), name.end(), '.', '_');
	std::replace(name.begin(), name.end(), '/', '_');
	out << "const char* " << name << " = R\"(";
	ifstream f(path.path().c_str());
	if (!f.is_open()) {
		throw std::runtime_error(string("Failed to open file: ") + path.path().c_str());
	}

	std::string str((std::istreambuf_iterator<char>(f)),
		std::istreambuf_iterator<char>());

	out << str << ")\";";
	out << endl << endl;

	(*map)[path.path().filename().c_str()] = name;
}

void do_search(const fs::directory_iterator& it,
	unordered_map<string, string>* map, ofstream& out) {
	for (const auto& entry : it) {
		if (entry.is_regular_file()) {
			if (important_ext.find(entry.path().extension()) != important_ext.end()) {
				write_into_lookup(entry, map, out);
			}
		} else if (entry.is_directory()) {
			do_search(fs::directory_iterator(entry.path()), map, out);
		}
	}
}

int main(int argc, char* argv[])
{   
	if (argc != 3) {
		throw std::runtime_error("Incorrect number of runtime arguments!");
	}

	string path(argv[1]);
	ofstream f_out(argv[2]);
	unordered_map<string, string> map;
	f_out << "#include <unordered_map>" << endl;
	f_out << "#include <string>" << endl;
	f_out << endl;

	do_search(fs::directory_iterator(path), &map, f_out);

	f_out << "void MakeSourceMap(std::unordered_map<std::string, const char*>* map) {" << endl;
	for (auto& it : map) {
		f_out << "\t(*map)[\"/internal/" << it.first << "\"] = " << it.second << ";" << endl;
		f_out << "\t(*map)[\"internal/" << it.first << "\"] = " << it.second << ";" << endl;
	}
	f_out << "}" << endl;
	return 0;
} 