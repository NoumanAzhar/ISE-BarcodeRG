#include "Data_Files.h"
#include <string>
#include <vector>
#include <fstream>
using namespace std;

namespace pos {

	bool isSpaceChar(char c)
	{
		return c == ' ' || c == '\t' || c == '\r' || c == '\n';
	}
	string trimSpaces(const string& value)
	{
		int start = 0;
		while (start < value.length() && isSpaceChar(value[start]))
			start++;
		int end = value.length();
		while (end > start && isSpaceChar(value[end - 1]))
			end--;
		return value.substr(start, end - start);
	}
	std::string encodeField(const std::string& value)
	{
		string final = trimSpaces(value);
		for (int i = 0; i < final.length(); i++)
		{
			if (final[i] == ' ')
				final[i] = '_';
		}
		if (final == "")
			return "-";
		return final;
	}
	std::string decodeField(const std::string& token)
	{
		string final = trimSpaces(token);
		for (int i = 0; i < final.length(); i++)
		{
			if (final[i] == '_')
				final[i] = ' ';
		}
		if (final == "-")
			return "";
		return final;
	}
	Record splitRecord(const std::string& line)
	{
		Record rec;
		string text = trimSpaces(line);
		int at;
		while ((at = text.find(' ')) != std::string::npos)
		{
			string part = text.substr(0, at);
			rec.push_back(decodeField(part));
			text.erase(0, at + 1);
		}
		if (text != "" && text != "#")
			rec.push_back(decodeField(text));
		return rec;
	}
	std::string joinRecord(const Record& fields)
	{
		string final;
		for (int i = 0; i < fields.size(); i++)
		{
			final += encodeField(fields[i]);
			final += " ";
		}
		final += "#";
		return final;
	}
	std::vector<Record> readRecords(const std::string& path)
	{
		vector<Record> Data;
		ifstream input(path);
		if (!input.is_open())
		{
			return Data;
		}
		string line;
		while (getline(input, line))
		{
			Record rec = splitRecord(line);
			if (rec.empty())
				continue;
			Data.push_back(rec);
		}
		return Data;
	}
	bool writeRecords(const std::string& path, const std::vector<Record>& rows)
	{
		ofstream output(path);
		if (!output.is_open())
			return 0;	// does nothing if file is not open
		for (int i = 0; i < rows.size(); i++)
		{
			output << joinRecord(rows[i]) << endl;
		}
		return 1;
	}
	bool appendRecord(const std::string& path, const Record& row)
	{
		fstream append(path, ios::out | ios::app);
		if (!append.is_open())
			return false;	//sem 
		append << joinRecord(row) << endl;
		return 1;
	}
} // namespace pos