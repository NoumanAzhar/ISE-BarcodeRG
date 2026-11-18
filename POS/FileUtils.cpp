#include<fstream>
#include"FileUtils.h"
using namespace std;
string FileUtils::readLastId(const string& filename) {
	ifstream fin(filename);
	if (!fin.is_open()) {
		return "";
	}
	string line, lastLine;
	while (getline(fin, line)) {
		if (!line.empty())
			lastLine = line;
	}
	size_t pos = lastLine.find('|');
	if (pos == string::npos)
		return "";
	return lastLine.substr(0, pos);
}
