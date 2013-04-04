// MapSak.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <tchar.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>


using std::string;

struct MapSymbol
{
	unsigned long address;
	unsigned long size;
	std::string name;
};

struct MapSymbolSizeLess
{
	operator()(const MapSymbol &a, const MapSymbol &b)
	{
		return a.size < b.size;
	}
};

struct MapSymbolSizeGreater
{
	operator()(const MapSymbol &a, const MapSymbol &b)
	{
		return a.size > b.size;
	}
};

struct MapSymbolAddressLess
{
	operator()(const MapSymbol &a, const MapSymbol &b)
	{
		return a.address < b.address;
	}
};

int _tmain(int argc, _TCHAR* argv[])
{
	std::ifstream infile;
	std::string oneLine;
	std::string oneToken;
	MapSymbol ms;
	std::vector<MapSymbol> symbols;

	infile.open(argv[1]);
	if (!infile)
		return -1;

	// Always skip the first seven lines
	for (int i = 0; i < 7; ++i)
		std::getline(infile, oneLine);

	// Now skip over all CODE/DATA lines, find "Address" header line
	do
	{
		std::getline(infile, oneLine);
	} while(oneLine.find("Address") == string::npos);

	// OK. Get one more blank line
	std::getline(infile, oneLine);

	// Now we have to get a whole bunch of lines, and actually do things
	while(1)
	{
		// Read the first token
		infile >> oneToken;

		// Did we finally reach the blank line?
		if (oneToken == "entry")
			break;

		// OK. That was the wacky format address. Get the symbol name
		infile >> ms.name;
		// And the full address
		infile >> oneToken;
		ms.address = strtoul(oneToken.c_str(), NULL, 16);

		// Add to the vector
		symbols.push_back(ms);

		// Now we just grab the rest of the line. We don't need it
		std::getline(infile, oneLine);
	}

	// Sort what we have by address
	std::sort(symbols.begin(), symbols.end(), MapSymbolAddressLess());

	// Calculate sizes for everything we've read thus far
	// (except for the actual last synmbol, which we can't figure out)
	size_t j = 0;
	size_t lastSym = symbols.size() - 1;
	for ( ; j < lastSym; ++j)
		symbols[j].size = symbols[j+1].address - symbols[j].address;

	// No reason to keep the last symbol around, we know nothing
	symbols.pop_back();

	// OK. Skip a couple lines
	do
	{
		std::getline(infile, oneLine);
	} while (oneLine.find("Static") == string::npos);

	// Skip another blank line
	std::getline(infile, oneLine);

	// Now we do it again, for the second batch of symbols...
	while(!infile.eof())
	{
		// Read the first token
		infile >> oneToken;

		// Did we finally reach the blank line?
		if (oneToken.empty())
			break;

		// OK. That was the wacky format address. Get the symbol name
		infile >> ms.name;
		// And the full address
		infile >> oneToken;
		ms.address = strtoul(oneToken.c_str(), NULL, 16);

		// Add to the vector
		symbols.push_back(ms);

		// Now we just grab the rest of the line. We don't need it
		std::getline(infile, oneLine);
	}

	// Sort the new batch
	std::sort(symbols.begin() + lastSym, symbols.end(), MapSymbolAddressLess());

	// OK. Do it again
	j = lastSym;
	lastSym = symbols.size() - 1;
	for( ; j < lastSym; ++j)
		symbols[j].size = symbols[j+1].address - symbols[j].address;

	// No reason to keep the last symbol around, we know nothing
	symbols.pop_back();

	// Close off the file
	infile.close();

	// Sort the list by size
	std::sort(symbols.begin(), symbols.end(), MapSymbolSizeGreater());

	// Print out the ten largest symbols?
	for (j = 0; j < symbols.size(); ++j)
		std::cout << symbols[j].name << "    [" << symbols[j].size << "]\n";
	std::cout << std::endl;

	return 0;
}

