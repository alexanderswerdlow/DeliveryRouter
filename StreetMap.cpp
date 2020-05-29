#include "provided.h"
#include <string>
#include <vector>
#include <functional>
#include <fstream>
#include "ExpandableHashMap.h"
using namespace std;

unsigned int hasher(const GeoCoord &g) {
  return std::hash<string>()(g.latitudeText + g.longitudeText);
}

class StreetMapImpl {
 public:
  StreetMapImpl();
  ~StreetMapImpl();
  bool load(string mapFile);
  bool getSegmentsThatStartWith(const GeoCoord &gc, vector<StreetSegment> &segs) const;
 private:
  std::pair<GeoCoord, GeoCoord> get_geocoords(string s);
  ExpandableHashMap<GeoCoord, vector<StreetSegment>> map;
};

StreetMapImpl::StreetMapImpl() {

}

StreetMapImpl::~StreetMapImpl() {

}

bool StreetMapImpl::load(string mapFile) {
  ifstream infile(mapFile);
  if (!infile) { //We can't process file, return false
	return false;
  }
  string s;
  while (getline(infile, s)) { //Get Street Name
	string name = s;
	getline(infile, s); //Get Number Of Street Segments
	int num_attr = stoi(s);
	for (int i = 0; i < num_attr; i++) {
	  getline(infile, s); //Get Line Containing Two GeoCoords
	  std::pair<GeoCoord, GeoCoord> cur = get_geocoords(s); //Parse Line Into Pair of GeoCoords
	  if (map.find(cur.first) == nullptr) { //If we haven't recorded GeoCoord yet, init vector
		vector<StreetSegment> vec;
		map.associate(cur.first, vec);
	  }
	  map.find(cur.first)->push_back({cur.first, cur.second, name}); //Add other GeoCoord to vector
	  if (map.find(cur.second) == nullptr) { //If we haven't recorded GeoCoord yet, init vector
		vector<StreetSegment> vec;
		map.associate(cur.second, vec);
	  }
	  map.find(cur.second)->push_back({cur.second, cur.first, name}); //Add other GeoCoord to vector
	}
  }
  return true;
}

bool StreetMapImpl::getSegmentsThatStartWith(const GeoCoord &gc, vector<StreetSegment> &segs) const {
  auto v = map.find(gc);
  if (v == nullptr) { //If map returns nullptr, we can't locate the GeoCoord and segs is unchanged
	return false;
  } else {
	segs = *v; //We already have associated the GeoCoord with a vector of StreetSegs so just de-reference pointer and modify segs
	return true;
  }
}

std::pair<GeoCoord, GeoCoord> StreetMapImpl::get_geocoords(string s) {
  int pos = 0;

  pos = s.find(' '); //Finds position of first space indicating the end of the first coord
  string l1 = s.substr(0, pos); //Gets the first coordinate
  s.erase(0, pos + 1); //Erases first coordinate (so 3 remain)

  pos = s.find(' ');
  string l2 = s.substr(0, pos);
  s.erase(0, pos + 1);

  pos = s.find(' ');
  string r1 = s.substr(0, pos);
  s.erase(0, pos + 1);

  string r2 = s;

  return {{l1, l2}, {r1, r2}}; //Create and a pair of GeoCoords based on the 4 coordinates (long/lat) we found
}

//******************** StreetMap functions ************************************

// These functions simply delegate to StreetMapImpl's functions.
// You probably don't want to change any of this code.

StreetMap::StreetMap() {
  m_impl = new StreetMapImpl;
}

StreetMap::~StreetMap() {
  delete m_impl;
}

bool StreetMap::load(string mapFile) {
  return m_impl->load(mapFile);
}

bool StreetMap::getSegmentsThatStartWith(const GeoCoord &gc, vector<StreetSegment> &segs) const {
  return m_impl->getSegmentsThatStartWith(gc, segs);
}
