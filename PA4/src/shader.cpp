#include "shader.h"

using namespace std;

void Shader::loadFromFile(std::string fileName, char* &contents)
{
	ifstream fin;
	fin.open(fileName);
	if (fin.is_open())
	{
		// get length
		fin.seekg(0, fin.end);
		int length = fin.tellg();
		fin.seekg(0, fin.beg);

		if (length > 0)
		{
			contents = new char[length];
			fin.read(contents, length);
			contents[length] = 0;
			fin.close();
		}
	}
}
