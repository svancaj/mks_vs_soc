/**************************************************/
/* compile with: g++ translator.cpp -o translator */
/**************************************************/

#include <iostream>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <tuple>
#include <queue>
#include <algorithm>

using namespace std;

void printHelp(char**);
void LoadMap(string, vector<vector<int> >&);
void CBS(vector<vector<int> >&, vector<pair<int,int> >&, vector<pair<int,int> >&,
			int, int, int, string, string);

int main(int argc, char** argv) 
{
	bool hflag = false;
	bool iflag = false;
	char *fvalue = NULL;
	char *cvalue = NULL;
	char *avalue = NULL;
	char *tvalue = NULL;

	int timeout = 300;
	string map_dir = "instances/maps/";

	// parse arguments
	opterr = 0;
	char c;
	while ((c = getopt (argc, argv, "hif:c:a:t:")) != -1)
	{
		switch (c)
		{
			case 'h':
				hflag = true;
				break;
			case 'i':
				iflag = true;
				break;
			case 'f':
				fvalue = optarg;
				break;
			case 'c':
				cvalue = optarg;
				break;
			case 'a':
				avalue = optarg;
				break;
			case 't':
				tvalue = optarg;
				break;
			case '?':
				if (optopt == 'f' || optopt == 'c' || optopt == 'a' || optopt == 't')
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

	if (hflag)
	{
		printHelp(argv);
		return 0;
	}

	if (string(cvalue).compare("soc") != 0 && string(cvalue).compare("mks") != 0)
	{
		cout << "Unknown cost function \"" << cvalue << "\"!" << endl;
		printHelp(argv);
		return -1;
	}

	if (tvalue != NULL)
		timeout = atoi(tvalue);

	int start_agents = 1;
	if (avalue != NULL)
		start_agents = atoi(avalue);

	// read input
	bool map_loaded = false;
	ifstream in;
	in.open(fvalue);
	if (!in.is_open())
	{
		cout << "Could not open " << fvalue << endl;
		return 0;
	}

	vector<pair<int,int> > starts;
	vector<pair<int,int> > goals;
	vector<vector<int> > map;

	char c_dump;
	string line;
	getline(in, line); // first line - version

	while (getline(in, line))
	{
		stringstream ssline(line);
		string part;
		vector<string> parsed_line;
		while (getline(ssline, part, '\t'))
			parsed_line.push_back(part);

		if (!map_loaded)
		{
			LoadMap(map_dir + parsed_line[1], map);
			map_loaded = true;
		}

		starts.push_back({stoi(parsed_line[5]), stoi(parsed_line[4])});
		goals.push_back({stoi(parsed_line[7]), stoi(parsed_line[6])});
	}
	in.close();

	int max_agents = start_agents;
	if (iflag)
		max_agents = starts.size();

	CBS(map, starts, goals, start_agents, max_agents, timeout, string(cvalue), string(fvalue));
	
	return 0;
}

void LoadMap(string map_path, vector<vector<int> >& map)
{
	ifstream in;
	in.open(map_path);
	if (!in.is_open())
	{
		cout << "Could not open " << map_path << endl;
		return;
	}

	char c_dump;
	string s_dump;
	int height, width;
	getline(in, s_dump); // first line - type

	in >> s_dump >> height;
	in >> s_dump >> width;
	in >> s_dump; // map
	
	// graph
	map = vector<vector<int> >(height, vector<int>(width, -1));
	int number_of_vertices = 0;

	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			in >> c_dump;
			if (c_dump == '.')
			{
				map[i][j] = number_of_vertices;
				number_of_vertices++;
			}
		}
	}

	in.close();
}

void printHelp(char** argv)
{
	cout << "Usage of this generator:" << endl;
	cout << argv[0] << " [-h] -i agents_file -c cost_function [-a agents] [-t timeout] [-i] [-S] [-R]" << endl;
	cout << "	-h               : prints help and exits" << endl;
	cout << "	-f agents_file   : path to an agents file" << endl;
	cout << "	-c cost_function : cost function to use - soc|mks" << endl;
	cout << "	-a agents        : number of agents (default 1)" << endl;
	cout << "	-t timeout       : timeour for solver in seconds (default 300s)" << endl;
	cout << "	-i incremental   : incrementaly call the solver with increasing number of agents (default false)" << endl;
}


void CBS(vector<vector<int> >& map, vector<pair<int,int> >& starts, vector<pair<int,int> >& goals,
			int start_agents, int max_agents, int timeout, string cvalue, string fvalue)
{
	string solver_input = "instance.in";
	string solver_output = "result.out";
	string results_file = "results_cbs.res";

	for (int agent = start_agents; agent < max_agents + 1; agent++)
	{
		// print instance
		ofstream CBS;
		CBS.open(solver_input);
		if (!CBS.is_open())
			cout << "fail" << endl;
		
		// graph
		CBS << "0" << endl << "Grid:" << endl;
		CBS << map.size() << "," << map[0].size() << endl;
		for (size_t i = 0; i < map.size(); i++)
		{
			for (size_t j = 0; j < map[i].size(); j++)
			{
				if (map[i][j] == -1)
					CBS << "@";
				else
					CBS << ".";
			}
			CBS << endl;
		}

		// agents
		CBS << "Agents:" << endl << agent << endl;
		for (size_t i = 0; i < agent; i++)
		{
			CBS << i << ",";
			// goal
			CBS << goals[i].first << "," << goals[i].second << ",";
			// start
			CBS << starts[i].first << "," << starts[i].second;

			CBS << endl;
		}

		// avoid - legacy
		CBS << "CurrentPlan:" << endl;
		CBS << "0,0" << endl;
		CBS.close();

		// solve by CBS
		stringstream ss_exec;
		string cost_function = "";
		if (string(cvalue).compare("mks") == 0)
			cost_function = "makespan";

		ss_exec << "./cbs.exe " << solver_input << " " << timeout * 1000 << " " << cost_function << " > " << solver_output;
		string execute = ss_exec.str();
		cout << execute << endl;
		system(execute.c_str());

		// read results
		ifstream res;
		res.open(solver_output);
		if (!res.is_open())
		{
			cout << "Could not open result file " << solver_output << endl;
			return;
		}

		bool valid_solution = false;
		int found_makespan = 0;
		float total_time = 0;
		int found_cost = 0;
		int solution_depth = 0;

		string line;
		while (getline(res, line))
		{
			if (line.rfind("agents | timesteps", 0) == 0)
			{
				valid_solution = true;
				getline(res, line);
				stringstream ss(line);
				ss >> found_makespan;
				ss >> found_makespan;
			}

			if (line.rfind("Total cost:", 0) == 0)	// found soc/mks
			{
				valid_solution = true;
				stringstream ssline(line);
				string part;
				vector<string> parsed_line;
				while (getline(ssline, part, ' '))
					parsed_line.push_back(part);

				found_cost = stof(parsed_line[2]);
			}

			if (line.rfind("Solution depth:", 0) == 0)	// found solution depth
			{
				valid_solution = true;
				stringstream ssline(line);
				string part;
				vector<string> parsed_line;
				while (getline(ssline, part, ' '))
					parsed_line.push_back(part);

				solution_depth = stof(parsed_line[2]);
			}

			if (line.rfind("Time In milliseconds:", 0) == 0)	// time of computation
			{
				stringstream ssline(line);
				string part;
				vector<string> parsed_line;
				while (getline(ssline, part, ' '))
					parsed_line.push_back(part);

				total_time = stof(parsed_line[3]);
			}
		}

		res.close();

		// timeout or error
		if (!valid_solution)
			return;

		// note the results
		ofstream out;
		out.open(results_file, ios::app);
		if (!out.is_open())
		{
			cout << "Could not open results file " << results_file << endl;
			return;
		}
		out << fvalue << "\t"
			<< cvalue << "\t"
			<< agent << "\t"
			<< found_makespan << "\t"
			<< found_cost << "\t"
			<< solution_depth << "\t"
			<< total_time << endl;

		out.close();
	}
}