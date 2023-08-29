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
void ASP(vector<vector<int> >&, vector<pair<int,int> >&, vector<pair<int,int> >&,
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

	if (string(cvalue).compare("makespan") != 0 && string(cvalue).compare("iter") != 0 && string(cvalue).compare("jump-old") != 0)
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

	ASP(map, starts, goals, start_agents, max_agents, timeout, string(cvalue), string(fvalue));
	
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
	cout << "	-c cost_function : cost function to use - makespan|iter|jump-old" << endl;
	cout << "	-a agents        : number of agents (default 1)" << endl;
	cout << "	-t timeout       : timeour for solver in seconds (default 300s)" << endl;
	cout << "	-i incremental   : incrementaly call the solver with increasing number of agents (default false)" << endl;
}


void ASP(vector<vector<int> >& map, vector<pair<int,int> >& starts, vector<pair<int,int> >& goals,
			int start_agents, int max_agents, int timeout, string cvalue, string fvalue)
{
	string solver_input = "instance.lp";
	string solver_output = "result.out";
	string results_file = "results_asp.res";

	for (int agent = start_agents; agent < max_agents + 1; agent++)
	{
		// print instance
		ofstream ASP;
		ASP.open(solver_input);
		if (!ASP.is_open())
			cout << "fail" << endl;
		
		// graph - vertices
		for (size_t i = 0; i < map.size(); i++)
			for (size_t j = 0; j < map[i].size(); j++)
				if (map[i][j] != -1)
					ASP << "vertex((" << i + 1 << "," << j + 1 << ")). ";

		ASP << "\n";

		// graph - edges
		for (size_t i = 0; i < map.size(); i++)
		{
			for (size_t j = 0; j < map[i].size(); j++)
			{
				if (map[i][j] == -1)
					continue;
				if (i > 0 && map[i-1][j] != -1)
					ASP << "edge((" << i + 1 << "," << j + 1 << "),(" << i << "," << j + 1 << ")). ";
				if (i < map.size() - 1 && map[i+1][j] != -1)
					ASP << "edge((" << i + 1 << "," << j + 1 << "),(" << i + 2 << "," << j + 1 << ")). ";
				if (j > 0 && map[i][j-1] != -1)
					ASP << "edge((" << i + 1 << "," << j + 1 << "),(" << i + 1 << "," << j << ")). ";
				if (j < map[i].size() - 1 && map[i][j+1] != -1)
					ASP << "edge((" << i + 1 << "," << j + 1 << "),(" << i + 1<< "," << j + 2 << ")). ";
			}
		}
		ASP << "\n";

		// agents
		for (int i = 0; i < agent; i++)
			ASP << "start(" << i + 1 << ",(" << starts[i].first + 1 << "," << starts[i].second + 1 << ")). ";
		ASP << "\n";

		for (int i = 0; i < agent; i++)
			ASP << "goal(" << i + 1 << ",(" << goals[i].first + 1 << "," << goals[i].second + 1 << ")). ";
		ASP << "\n";

		for (int i = 0; i < agent; i++)
			ASP << "agent(" << i + 1 << ").";
		ASP << "\n";

		ASP.close();

		// solve by ASP
		stringstream ss_exec;
		ss_exec << "(cd mapf_soc/; timeout " << timeout << " mapf_soc -i ../" << solver_input << " --count-reach --strat " << cvalue << ") > " << solver_output;
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

		int solver_time = 0;
		int ground_time = 0;
		int total_time = 0;

		int max_delta = 0;
		int choices = 0;
		int conflicts = 0;
		int constraints  = 0;
		int vars = 0;
		int reach_atoms = 0;

		string line;
		while (getline(res, line))
		{
			if (line.rfind("SAT", 0) == 0) // solution found
			{
				valid_solution = true;
			}

			if (line.rfind("Solving for delta", 0) == 0)	// increase in delta
			{
				stringstream ssline(line);
				string part;
				vector<string> parsed_line;
				while (getline(ssline, part, ' '))
					parsed_line.push_back(part);

				max_delta = stoi(parsed_line[3]);
			}

			if (line.rfind("Jumping to delta", 0) == 0)	// increase in delta - jumping model
			{
				stringstream ssline(line);
				string part;
				vector<string> parsed_line;
				while (getline(ssline, part, ' '))
					parsed_line.push_back(part);

				max_delta = stoi(parsed_line[3]);
			}

			if (line.rfind("Solving  ", 0) == 0)	// solving time
			{
				stringstream ssline(line);
				string part;
				vector<string> parsed_line;
				while (getline(ssline, part, ':'))
					parsed_line.push_back(part);

				solver_time = stof(parsed_line[1])*1000;
			}

			if (line.rfind("grounding  ", 0) == 0)	// grounding time
			{
				stringstream ssline(line);
				string part;
				vector<string> parsed_line;
				while (getline(ssline, part, ':'))
					parsed_line.push_back(part);

				ground_time = stof(parsed_line[1])*1000;
			}

			if (line.rfind("time  ", 0) == 0)	// total time
			{
				stringstream ssline(line);
				string part;
				vector<string> parsed_line;
				while (getline(ssline, part, ':'))
					parsed_line.push_back(part);

				total_time = stof(parsed_line[1])*1000;
			}

			if (line.rfind("choices  ", 0) == 0)	// choices
			{
				stringstream ssline(line);
				string part;
				vector<string> parsed_line;
				while (getline(ssline, part, ':'))
					parsed_line.push_back(part);

				choices = stof(parsed_line[1]);
			}

			if (line.rfind("conflicts  ", 0) == 0)	// conflicts
			{
				stringstream ssline(line);
				string part;
				vector<string> parsed_line;
				while (getline(ssline, part, ':'))
					parsed_line.push_back(part);

				conflicts = stof(parsed_line[1]);
			}

			if (line.rfind("constraints  ", 0) == 0)	// constraints
			{
				stringstream ssline(line);
				string part;
				vector<string> parsed_line;
				while (getline(ssline, part, ':'))
					parsed_line.push_back(part);

				constraints = stof(parsed_line[1]);
			}

			if (line.rfind("vars  ", 0) == 0)	// vars
			{
				stringstream ssline(line);
				string part;
				vector<string> parsed_line;
				while (getline(ssline, part, ':'))
					parsed_line.push_back(part);

				vars = stof(parsed_line[1]);
			}

			if (line.rfind("reach atoms  ", 0) == 0)	// reachable atoms generated
			{
				stringstream ssline(line);
				string part;
				vector<string> parsed_line;
				while (getline(ssline, part, ':'))
					parsed_line.push_back(part);

				reach_atoms = stof(parsed_line[1]);
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
			<< max_delta << "\t"
			<< ground_time << "\t"
			<< solver_time << "\t"
			<< total_time << "\t"
			<< reach_atoms << "\t"
			<< choices << "\t"
			<< conflicts << "\t"
			<< constraints << "\t"
			<< vars << endl;

		out.close();
	}
}