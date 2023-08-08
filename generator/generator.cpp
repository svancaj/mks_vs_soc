/* compile with: g++ -std=c++11 generator.cpp -o gen */

#include <iostream>
#include <sstream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fstream>
#include <vector>
#include <algorithm>

using namespace std;

void createInstance(vector<vector<int> >&, char*, int, int);
bool contains(vector<pair<int, int> >& vc, int x, int y);
void printHelp(char**);

int main(int argc, char* argv[])
{
	bool hflag = false;
	char *avalue = NULL;
	char *svalue = NULL;
	char *tvalue = NULL;
	char *mvalue = NULL;

	// parse arguments
	opterr = 0;
	char c;
	while ((c = getopt (argc, argv, "ha:s:t:m:")) != -1)
	{
		switch (c)
		{
			case 'h':
				hflag = true;
				break;
			case 'a':
				avalue = optarg;
				break;
			case 's':
				svalue = optarg;
				break;
			case 't':
				tvalue = optarg;
				break;
			case 'm':
				mvalue = optarg;
				break;
			case '?':
				if (optopt == 'a' || optopt == 's' || optopt == 't' || optopt == 'm')
				{
					cout << "Option -" << (char)optopt << " requires an argument!" << endl;
					return -1;
				}
				// unknown option - ignore it
				break;
			default:
				return -1; // should not get here;
		}
	}

	// set problem specification
	if (hflag)
	{
		printHelp(argv);
		return 0;
	}

	bool all_good = true;

	int agents;
	int seed;
	int times;

	// agents
	if (avalue != NULL)
		agents = atoi(avalue);
	else
		all_good = false;

	// times
	if (tvalue != NULL)
		times = atoi(tvalue);
	else
		all_good = false;

	// random seed
	if (svalue != NULL)
		seed = atoi(svalue);
	else
		seed = time(NULL);

	if (!all_good)
	{
		cerr << "Something went wrong with arguments!" << endl;
		printHelp(argv);
		return -1;
	}

	// load map
	ifstream in;
	in.open(string(mvalue));
	if (!in.is_open())
	{
		cerr << "Could not open " << mvalue << endl;
		return -1;
	}

	char c_dump;
	string s_dump;
	int rows, columns;
	getline(in, s_dump); // first line - type

	in >> s_dump >> rows;
	in >> s_dump >> columns;
	in >> s_dump; // map
	
	vector<vector<int> > int_graph = vector<vector<int> >(rows, vector<int>(columns, -1));
	int nodes = 0;

	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < columns; j++)
		{
			in >> c_dump;
			if (c_dump == '.')
			{
				int_graph[i][j] = nodes;
				nodes++;
			}
		}
	}

	in.close();

	srand (seed);

	for (int i = 0; i < times; i++)
		createInstance(int_graph, mvalue, agents, i);

	return 0;
}

void createInstance(vector<vector<int> >& map, char* map_path, int agents, int instance_nr)
{
	string map_name = string(map_path).erase(0, string(map_path).find_last_of('/') + 1); // delete path parth
	map_name.erase(map_name.length() - 4, map_name.length());	// delete ".map"
	
	// name
	stringstream ss;
	ss << map_name << "-" << instance_nr + 1 << ".scen";
	string filename = ss.str();

	// start and goals
	vector<pair<int, int> > start;
	vector<pair<int, int> > goal;

	int placed = 0;
	while (placed < agents)
	{
		int x = rand() % map.size();
		int y = rand() % map.size();
		if (map[x][y] == -1)
			continue;
		if (contains(start, x, y))
			continue;
		start.push_back(make_pair(x, y));
		placed++;
	}

	placed = 0;
	while (placed < agents)
	{
		int x = rand() % map.size();
		int y = rand() % map.size();
		if (map[x][y] == -1)
			continue;
		if (contains(goal, x, y))
			continue;
		goal.push_back(make_pair(x, y));
		placed++;
	}

	// print
	ofstream out;
	out.open(string("instances/scenarios/").append(filename));

	out << "version 1" << endl;

	for (size_t i = 0; i < start.size(); i++)
	{
		out << "0\t" << map_name << ".map\t" << map.size() << "\t" << map.size() << "\t";
		out << start[i].second << "\t" << start[i].first << "\t" << goal[i].second << "\t" << goal[i].first << "\t";
		out << "0" << endl;
	}
	out.close();
}

bool contains(vector<pair<int, int> >& vc, int x, int y)
{
	for (size_t i = 0; i < vc.size(); i++)
		if (vc[i].first == x && vc[i].second == y)
			return true;
	return false;
}

void printHelp(char* argv[])
{
	cout << "Usage of this generator:" << endl;
	cout << argv[0] << " [-h] -m map -a agents -t times [-s seed]" << endl;
	cout << "	-h        : prints help and exits" << endl;
	cout << "	-m map    : path to a map file" << endl;
	cout << "	-a agents : number of agents to be generated in the given map" << endl;
	cout << "	-t times  : how many instances are to be generated" << endl;
	cout << "	-s seed   : a seed for the random generator" << endl;
}