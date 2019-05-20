#include <iostream>
#include <fstream>
#include <algorithm> //sort
#include <sstream>
#include <string>
#include <cstring> //memset
#include <set>
#include <cfloat> //DBL_MAX

using namespace std;

void readDataGraph(string);
void readQueryGraph(ifstream&, int);
void buildDAG();
void printDAG();
void clearMemory();

bool sortByDegreeData(int, int);
bool sortByLabel(int, int);
bool sortByDegreeQuery(int, int);
bool sortByLabelFreqQuery(int, int);

int binaryLowerBound(int, int, int);

int numDataNode = 0;
int numLabel = 0;
int* labelData = NULL; //labelData[i] contains label of vertex i
int* degreeData = NULL; //degreeData[i] contains degree of vertex i
int* sortedData = NULL; //sortedData contains V in sorted order first by label frequency and second by degree
int* idxSortedData = NULL; //idxSortedData[l] contains the last index in sortedData such that labelData[sortedData[index]] is l
int* labelFrequency = NULL; //labelFrequency[l] contains the number of vertices having label l
int* renamedLabel = NULL; //labels are renamed as consecutive numbers

int root = -1;
int numQueryNode = 0;
int sumQueryDegree = 0;
int* labelQuery = NULL;
int* degreeQuery = NULL;
int* adjListQuery = NULL;
int* adjIndexQuery = NULL;

int** dagChildQuery = NULL; //dagChildQuery[i]: children of node i
int** dagParentQuery = NULL; //dagParentQuery[i]: parent of node i
int* dagChildQuerySize = NULL; //dagChildQuerySize[i]: the number of children of node i
int* dagParentQuerySize = NULL; //dagParentQuerySize[i]: the number of parent on node i

void readDataGraph(string aFileName)
{
/////////////////
//1st read: set degreeData, and calculate the largest label and the number of labels
	ifstream inFile(aFileName);

	int largestLabel = -1;
	set<int> labelSet;
	string line;
	while( getline(inFile, line) ) {
		if( line[0] == 't' ) {
			istringstream iss(line);
			char tag;
			int id;
			iss >> tag >> id >> numDataNode;
			if( labelData != NULL) {
				delete[] labelData;
				labelData = NULL;
			}
			if( degreeData != NULL ) {
				delete[] degreeData;
				degreeData = NULL;
			}
			labelData = new int[numDataNode];
			degreeData = new int[numDataNode];
			memset(degreeData, 0, sizeof(int) * (numDataNode));
		}
		else if( line[0] == 'v' ) {
			istringstream iss(line);
			char tag;
			int id;
			int label;
			iss >> tag >> id >> label;
			if( labelData == NULL || numDataNode < id ) {
				cout << "ERROR: in readDataGraph, vertex id out of range" << endl;
				exit(-1);
			}
			//labelData[id] = label;
			if( largestLabel < label )
				largestLabel = label;
			labelSet.insert(label);
		}
		else if( line[0] == 'e' ) {
			istringstream iss(line);
			char tag;
			int left;
			int right;
			int label;
			iss >> tag >> left >> right >> label;

			if( degreeData == NULL || numDataNode < left || numDataNode < right ) {
				cout << "ERROR: in readDataGraph, vertex id out of range" << endl;
				exit(-1);
			}
			//Make sure not to increase two times
			++degreeData[left];
			++degreeData[right];
		}
	}

	numLabel = labelSet.size();

//////////////////
//2nd read: rename label, set labelData, and set labelFrequency
	int labelId = 0;

	if( renamedLabel != NULL ) {
		delete[] renamedLabel;
		renamedLabel = NULL;
	}
	renamedLabel = new int[largestLabel + 1];
	memset(renamedLabel, -1, sizeof(int) * (largestLabel + 1) );

	if( labelFrequency != NULL ) {
		delete[] labelFrequency;
		labelFrequency = NULL;
	}
	labelFrequency = new int[numLabel];
	memset(labelFrequency, 0, sizeof(int) * (numLabel));

	inFile.clear();
	inFile.seekg(0, ios::beg);

	while( getline(inFile, line) ) {
		if( line[0] == 't' ) {
		}
		else if( line[0] == 'v' ) {
			istringstream iss(line);
			char tag;
			int id;
			int label;
			iss >> tag >> id >> label;

			if( renamedLabel[label] == -1 ) {
				renamedLabel[label] = labelId;
				++labelId;
			}

			labelData[id] = renamedLabel[label];

			++labelFrequency[ renamedLabel[label] ];
		}
		else if( line[0] == 'e' ) {
		}
	}

	inFile.close();
//////////////////
//sort data vertices by label name.
//Then, sort by degree for each label group
	if(sortedData != NULL ) {
		delete[] sortedData;
		sortedData = NULL;
	}
	sortedData = new int[numDataNode];
	for(int i = 0; i < numDataNode; ++i)
		sortedData[i] = i;

	sort(sortedData, sortedData + numDataNode, sortByDegreeData);
	stable_sort(sortedData, sortedData + numDataNode, sortByLabel);

	if( idxSortedData != NULL ) {
		delete[] idxSortedData;
		idxSortedData = NULL;
	}
	idxSortedData = new int[numLabel + 1];

	if( numDataNode < 1 ) {
		cout << "ERROR: in readDataGraph, 0 vertices" << endl;
		exit(-1);
	}

	idxSortedData[ labelData[sortedData[0]] ] = 0;
	for(int i = 1; i < numDataNode; ++i) {
		if( labelData[sortedData[i - 1]] != labelData[sortedData[i]] )
			idxSortedData[ labelData[sortedData[i]] ] = i;
	}
	idxSortedData[ numLabel ] = numDataNode;
}

bool sortByDegreeData(int aNode1, int aNode2)
{
	return (degreeData[aNode1] < degreeData[aNode2]);
}

bool sortByLabel(int aNode1, int aNode2)
{
	return (labelData[aNode1] < labelData[aNode2]);
}

bool sortByDegreeQuery(int aNode1, int aNode2)
{
	return (degreeQuery[aNode1] > degreeQuery[aNode2]);
}

bool sortByLabelFreqQuery(int aNode1, int aNode2)
{
	int label1 = labelQuery[aNode1];
	int label2 = labelQuery[aNode2];

	return (labelFrequency[label1] < labelFrequency[label2]);
}

//read one query graph
void readQueryGraph(ifstream& aInFile, int aSumDegree) 
{
//////////////////
	//allocate memory
	if( labelQuery == NULL )
		labelQuery = new int[numQueryNode];
	if( degreeQuery == NULL )
		degreeQuery = new int[numQueryNode];
	if( adjIndexQuery == NULL )
		adjIndexQuery = new int[numQueryNode + 1];
	if( adjListQuery == NULL ) {
		adjListQuery = new int[aSumDegree];
		sumQueryDegree = aSumDegree;
	}

	//(re)allocate memory for adjacency list of query graph
	if( sumQueryDegree < aSumDegree ) {
		if( adjListQuery != NULL ) {
			delete[] adjListQuery;
			adjListQuery = NULL;
		}
		
		adjListQuery = new int[aSumDegree];
		sumQueryDegree = aSumDegree;
	}
//////////////////
	//read query graph
	int index = 0;
	adjIndexQuery[0] = index;
	for(int i = 0; i < numQueryNode; ++i) {
		int id;
		int label;
		int degree;
		aInFile >> id >> label >> degree;
		labelQuery[i] = renamedLabel[label];
		degreeQuery[i] = degree;
		for(int j = 0; j < degree; ++j) {
			aInFile >> adjListQuery[index];
			++index;
		}
		adjIndexQuery[i + 1] = index;
	}
}

int selectRoot()
{
	int root = -1;
	int label;
	int degree;
	double rank;
	double rootRank = DBL_MAX;

	for (int i = 0; i < numQueryNode; ++i) {
		label = labelQuery[i];
		degree = degreeQuery[i];
		
		int start = idxSortedData[label];
		int end = idxSortedData[label + 1];
		int mid = binaryLowerBound(start, end - 1, degree);

		int numInitCand = end - mid;

		rank = numInitCand/(double)degree;

		if( rank < rootRank ) {
			root = i;
			rootRank = rank;
		}
	}

	return root;
}

int binaryLowerBound(int aLeft, int aRight, int aDegree)
{
	int left = aLeft;
	int right = aRight;
	while( left < right ) {
		int mid = left + (right - left) / 2;

		if( degreeData[ sortedData[mid] ] < aDegree )
			left = mid + 1;
		else
			right = mid;
	}
	return left;
}

void buildDAG()
{
//////////////////
//allocate memory for dag data structure
	if( dagChildQuery == NULL ) {
		dagChildQuery = new int*[numQueryNode];
		for(int i = 0; i < numQueryNode; ++i)
			dagChildQuery[i] = NULL;
	}
	if( dagParentQuery == NULL ) {
		dagParentQuery = new int*[numQueryNode];
		for(int i = 0; i < numQueryNode; ++i)
			dagParentQuery[i] = NULL;
	}
	if( dagChildQuerySize == NULL )
		dagChildQuerySize = new int[numQueryNode];
	if( dagParentQuerySize == NULL )
		dagParentQuerySize = new int[numQueryNode];

	memset(dagChildQuerySize, 0, sizeof(int) * numQueryNode);
	memset(dagParentQuerySize, 0, sizeof(int) * numQueryNode);

	for(int i = 0; i <numQueryNode; ++i) {
		if( dagChildQuery[i] != NULL) {
			delete[] dagChildQuery[i];
			dagChildQuery[i] = NULL;
		}
		dagChildQuery[i] = new int[degreeQuery[i]];
		
		if( dagParentQuery[i] != NULL ) {
			delete[] dagParentQuery[i];
			dagParentQuery[i] = NULL;
		}
		dagParentQuery[i] = new int[degreeQuery[i]];
	}
//////////////////
//construct dag data structure
	char* popped = new char[numQueryNode];
	memset(popped, 0, sizeof(char) * numQueryNode);
	char* visited = new char[numQueryNode];
	memset(visited, 0, sizeof(char) * numQueryNode);
	int* queue = new int[numQueryNode];
	int currQueueStart = 0;
	int currQueueEnd = 1;
	int nextQueueStart = 1;
	int nextQueueEnd = 1;

	//visit root
	root = selectRoot();
	visited[ root ] = 1;
	queue[0] = root;

	//BFS traversal using queue
	while(true) {
		sort(queue + currQueueStart, queue + currQueueEnd, sortByDegreeQuery);
		stable_sort(queue + currQueueStart, queue + currQueueEnd, sortByLabelFreqQuery);
		while( currQueueStart != currQueueEnd ) {
			int currNode = queue[ currQueueStart ];
			++currQueueStart;
			popped[currNode] = 1;
			for(int i = adjIndexQuery[currNode]; i < adjIndexQuery[currNode + 1]; ++i) {
				int childNode = adjListQuery[i];
				if(popped[childNode] == 0) {
					dagChildQuery[currNode][ dagChildQuerySize[currNode] ] = childNode;
					dagParentQuery[childNode][ dagParentQuerySize[childNode] ] = currNode;

					++dagChildQuerySize[currNode];
					++dagParentQuerySize[childNode];
				}
				if(visited[childNode] == 0) {
					visited[childNode] = 1;
					queue[nextQueueEnd] = childNode;
					++nextQueueEnd;
				}
			}
		}

		if(currQueueEnd == nextQueueEnd) //no nodes have been pushed in
			break;

		currQueueStart = currQueueEnd;
		currQueueEnd = nextQueueEnd;
	}
	delete[] popped;
	delete[] visited;
	delete[] queue;
}

void printDAG()
{
	int* queue = new int[numQueryNode];
	char* visited = new char[numQueryNode];
	memset(visited, 0, sizeof(char) * numQueryNode);

	queue[0] = root;
	visited[root] = 1;
	int queueStart = 0;
	int queueEnd = 1;

	while( queueStart != queueEnd ) {
		int currNode = queue[queueStart];
		++queueStart;

		cout << currNode << " ";
		for(int i = 0; i < dagChildQuerySize[currNode]; ++i) {
			if(visited[ dagChildQuery[currNode][i] ] == 0) {
				visited[ dagChildQuery[currNode][i] ] = 1;
				queue[queueEnd] = dagChildQuery[currNode][i];
				++queueEnd;
			}
		}
	}
	cout << endl;

	delete[] visited;
	delete[] queue;
}

void clearMemory()
{
	if(labelData != NULL)
		delete[] labelData;
	if(degreeData != NULL)
		delete[] degreeData;
	if(sortedData != NULL)
		delete[] sortedData;
	if(idxSortedData != NULL)
		delete[] idxSortedData;
	if(labelFrequency != NULL)
		delete[] labelFrequency;
	if(renamedLabel != NULL)
		delete[] renamedLabel;
	if(labelQuery != NULL)
		delete[] labelQuery;
	if(degreeQuery != NULL)
		delete[] degreeQuery;
	if(adjListQuery != NULL)
		delete[] adjListQuery;
	if(adjIndexQuery != NULL)
		delete[] adjIndexQuery;
	if(dagChildQuerySize != NULL)
		delete[] dagChildQuerySize;
	if(dagParentQuerySize != NULL)
		delete[] dagParentQuerySize;
	if(dagChildQuery != NULL) {
		for(int i = 0; i < numQueryNode; ++i) {
			if(dagChildQuery[i] != NULL) 
				delete[] dagChildQuery[i];
		}
		delete[] dagChildQuery;
	}
	if(dagParentQuery != NULL) {
		for(int i = 0; i < numQueryNode; ++i) {
			if(dagParentQuery[i] != NULL) 
				delete[] dagParentQuery[i];
		}
		delete[] dagParentQuery;
	}
}

//Usage: ./program dataGraph queryGraph numQuery
int main(int argc, char *argv[]) {

	if(argc != 4) {
		cout << "Usage: ./program dataFile queryFile numQuery" << endl;
		return -1;
	}
	cout << "Data File : " << argv[1] << endl;
	cout << "Query file: " << argv[2] << endl;
	cout << "Num Query: " << argv[3] << endl;

	int numQuery = atoi(argv[3]);

	readDataGraph(argv[1]);

	ifstream queryFile(argv[2]);
	for(int i = 0; i < numQuery; ++i) {
		char tag;
		int id;
		int num;
		int sumDegree;
		queryFile >> tag >> id >> num >> sumDegree;
		numQueryNode = num;

		readQueryGraph(queryFile, sumDegree);
		buildDAG();
		printDAG();
	}
	queryFile.close();

	clearMemory();
	return 0;
}