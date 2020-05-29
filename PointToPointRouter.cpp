#include "provided.h"
#include "ExpandableHashMap.h"
#include <list>
#include <queue>
#include <vector>
using namespace std;

class PointToPointRouterImpl {
 public:
  PointToPointRouterImpl(const StreetMap *sm);
  ~PointToPointRouterImpl();
  DeliveryResult generatePointToPointRoute(
	  const GeoCoord &start,
	  const GeoCoord &end,
	  list<StreetSegment> &route,
	  double &totalDistanceTravelled) const;
 private:
  const StreetMap *map;
};

PointToPointRouterImpl::PointToPointRouterImpl(const StreetMap *sm) : map{sm} {
}

PointToPointRouterImpl::~PointToPointRouterImpl() {
}

DeliveryResult PointToPointRouterImpl::generatePointToPointRoute(const GeoCoord &start, const GeoCoord &end, list<StreetSegment> &route, double &totalDistanceTravelled) const {
  route.clear(); //Make sure route is empty before we start

  vector<StreetSegment> valid;
  if (!map->getSegmentsThatStartWith(start, valid) || !map->getSegmentsThatStartWith(end, valid)) {
	return BAD_COORD; //If the start or end coords aren't in our mapping data, we can't do anything so return BAD_COORD
  }

  ExpandableHashMap<GeoCoord, double> cost_map; //Records the distance traveled to reach a given GeoCoord

  ExpandableHashMap<GeoCoord, StreetSegment> history; //Associated a GeoCoord with the StreetSeg taken to reach it
  // (ending at that GeoCoord, starting at another)

  priority_queue<std::pair<double, GeoCoord>, vector<std::pair<double, GeoCoord>>, greater<>> open_list; //Contains GeoCoords sorted by their
  // approximated cost so we can be efficient in looking at potential StreetSegs that will get us closer to the dest

  open_list.push({0, start}); //We start looking from the start location
  cost_map.associate(start, 0); //The cost to go from start -> start is zero
  bool success = false;

  while (!open_list.empty()) { //Run until we've run out of potential GeoCoords to look at
	GeoCoord current = open_list.top().second;
	open_list.pop();

	if (current == end) { //We've reached the destination
	  success = true;
	  break;
	}

	vector<StreetSegment> options;
	map->getSegmentsThatStartWith(current, options); //Get all StreetSegs that start at the current GeoCoord
	for (const auto &next_seg : options) {
	  auto next = next_seg.end;
	  //The cost to reach the new GeoCoord is the cost to get to the current coord plus the distance of that StreetSeg
	  double new_cost = *cost_map.find(current) + distanceEarthMiles(next_seg.start, next);
	  if (cost_map.find(next) == nullptr || new_cost < *cost_map.find(next)) { //if we haven't come across the coord or the path taken to it has a lower cost than before
		//Map the GeoCoord to the cost required to reach it
		cost_map.associate(next, new_cost);
		//Add the known cost to reach the new GeoCoord and approximate the distance to the destination using the 'euclidean' dist
		double approx_cost = new_cost + distanceEarthMiles(next, end);
		//Add to open list to be later looked at if necessary
		open_list.push({approx_cost, next});
		//Associate the new coord with the StreetSeg linking it and the 'current' GeoCoord so we can retrace the path later
		history.associate(next, next_seg);
	  }
	}
  }

  if (!success) { //Indicates we ran out of GeoCoords to look at without reaching the dest so we couldn't find a route to the destination
	return NO_ROUTE;
  }

  GeoCoord lastGeoCoord = end; //Start at the destination
  totalDistanceTravelled = 0;
  while (true) {
	if (lastGeoCoord == start || history.find(lastGeoCoord) == nullptr) {
	  break; //Once we reach the start coord (or if our came_from mapping got corrupted somehow which should never happen), we are done.
	}
	StreetSegment seg = (*history.find(lastGeoCoord)); //Finds the seg linking lastGeoCoord and the previous GeoCoord we traveled on
	totalDistanceTravelled += distanceEarthMiles(seg.start, seg.end);
	route.push_front(seg); //Always add to the front of the vector as we are going from the end of the path to the start
	lastGeoCoord = seg.start;
  }
  return DELIVERY_SUCCESS;
}

//******************** PointToPointRouter functions ***************************

// These functions simply delegate to PointToPointRouterImpl's functions.
// You probably don't want to change any of this code.

PointToPointRouter::PointToPointRouter(const StreetMap *sm) {
  m_impl = new PointToPointRouterImpl(sm);
}

PointToPointRouter::~PointToPointRouter() {
  delete m_impl;
}

DeliveryResult PointToPointRouter::generatePointToPointRoute(
	const GeoCoord &start,
	const GeoCoord &end,
	list<StreetSegment> &route,
	double &totalDistanceTravelled) const {
  return m_impl->generatePointToPointRoute(start, end, route, totalDistanceTravelled);
}
