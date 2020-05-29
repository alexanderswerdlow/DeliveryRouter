#include "provided.h"
#include <vector>
using namespace std;

class DeliveryPlannerImpl {
 public:
  DeliveryPlannerImpl(const StreetMap *sm);
  ~DeliveryPlannerImpl();
  DeliveryResult generateDeliveryPlan(
	  const GeoCoord &depot,
	  const vector<DeliveryRequest> &deliveries,
	  vector<DeliveryCommand> &commands,
	  double &totalDistanceTravelled) const;
 private:
  PointToPointRouter router;
  DeliveryOptimizer opt;
  string get_direction(double angle) const;
  string get_street_direction(const StreetSegment &seg) const;
  double get_street_dist(const StreetSegment &seg) const;
  DeliveryResult addStreetSegsToRoutes(const GeoCoord &start, const GeoCoord &end, const string &item, list<list<std::pair<StreetSegment, string>>> &routes) const;
};

DeliveryPlannerImpl::DeliveryPlannerImpl(const StreetMap *sm) : router{sm}, opt{sm} {
}

DeliveryPlannerImpl::~DeliveryPlannerImpl() {
}

DeliveryResult DeliveryPlannerImpl::generateDeliveryPlan(const GeoCoord &depot, const vector<DeliveryRequest> &deliveries, vector<DeliveryCommand> &commands, double &totalDistanceTravelled) const {
  double old = 0;
  vector<DeliveryRequest> mod = deliveries;
  opt.optimizeDeliveryOrder(depot, mod, old, totalDistanceTravelled);
  list<list<std::pair<StreetSegment, string>>> routes;

  DeliveryResult res1 = addStreetSegsToRoutes(depot, mod[0].location, mod[0].item, routes);
  for (int i = 0; i < mod.size() - 1; i++) {
	DeliveryResult res = addStreetSegsToRoutes(mod[i].location, mod[i + 1].location, mod[i + 1].item, routes);
	if (res != DELIVERY_SUCCESS) {
	  return res;
	}
  }
  DeliveryResult res2 = addStreetSegsToRoutes(mod[mod.size() - 1].location, depot, "", routes);

  if (res1 != DELIVERY_SUCCESS && res2 != DELIVERY_SUCCESS) {
	return res1;
  }

  commands.clear();
  int i_index = 0;
  int i_size = routes.size();
  int j_index = 0;
  int j_size = 0;
  DeliveryCommand current;
  for (auto &route : routes) {
	j_size = route.size();
	j_index = 0;
	if (route.size() == 1) {
	  StreetSegment seg = route.front().first;
	  if (seg.start == seg.end) { //If we start and end at the delivery point just add the delivery command
		current.initAsDeliverCommand(route.front().second);
		commands.push_back(current);
	  } else { //If we don't start at the delivery point but there is only  one street seg then just proceed (for the length of the seg) then deliver
		current.initAsProceedCommand(get_street_direction(seg), seg.name, get_street_dist(seg));
		commands.push_back(current);
		current.initAsDeliverCommand(route.front().second);
		commands.push_back(current);
	  }
	  continue;
	}
	StreetSegment lastSeg;
	for (auto &jt : route) {
	  StreetSegment seg = jt.first;
	  if (j_index == 0) { //The first command (if it isn't a deliver command) is always to proceed
		current.initAsProceedCommand(get_street_direction(seg), seg.name, get_street_dist(seg));
		commands.push_back(current);
	  } else {
		if (lastSeg.name == seg.name) { //If we're on the same street, just increase the distance of the last proceed command
		  commands.back().increaseDistance(get_street_dist(seg));
		} else {
		  double ang = angleBetween2Lines(lastSeg, seg);
		  if (ang < 1.0 || ang > 359.0) { //Angle is too small just proceed as described in the spec
			current.initAsProceedCommand(get_street_direction(seg), seg.name, get_street_dist(seg));
			commands.push_back(current);
		  } else { //Turn and add proceed command for the distance of the seg
			current.initAsTurnCommand(ang < 180.0 ? "left" : "right", seg.name);
			commands.push_back(current);
			current.initAsProceedCommand(get_street_direction(seg), seg.name, get_street_dist(seg));
			commands.push_back(current);
		  }
		}
	  }

	  if (j_index == j_size - 1 && i_index != i_size - 1) { //If we're at the end of the route (and we're not returning to the depot) intitiate delivery command
		current.initAsDeliverCommand(jt.second);
		commands.push_back(current);
	  }

	  lastSeg = seg;
	  j_index++;
	}
	i_index++;
  }
  return DELIVERY_SUCCESS;
}

DeliveryResult DeliveryPlannerImpl::addStreetSegsToRoutes(const GeoCoord &start, const GeoCoord &end, const string &item, list<list<std::pair<StreetSegment, string>>> &routes) const {
  list<StreetSegment> temp;
  double temp_distance = 0;
  DeliveryResult res = router.generatePointToPointRoute(start, end, temp, temp_distance);
  list<std::pair<StreetSegment, string>> l;
  for (const auto &i : temp) { //Pair each street seg with the item being delivered
	l.emplace_back(i, item);
  }
  routes.push_back(l); //Add to central route list
  return res;
}

string DeliveryPlannerImpl::get_street_direction(const StreetSegment &seg) const {
  return get_direction(angleOfLine(seg));
}

string DeliveryPlannerImpl::get_direction(double angle) const {
  if (angle >= 0 && angle <= 22.5) {
	return "east";
  } else if (angle >= 22.5 && angle < 67.5) {
	return "northeast";
  } else if (angle >= 67.5 && angle < 112.5) {
	return "north";
  } else if (angle >= 112.5 && angle < 157.5) {
	return "northwest";
  } else if (angle >= 157.5 && angle < 202.5) {
	return "west";
  } else if (angle >= 202.5 && angle < 247.5) {
	return "southwest";
  } else if (angle >= 247.5 && angle < 292.5) {
	return "south";
  } else if (angle >= 292.5 && angle < 337.5) {
	return "southeast";
  } else if (angle >= 337.5) {
	return "east";
  }
  return "";
}

double DeliveryPlannerImpl::get_street_dist(const StreetSegment &seg) const {
  return distanceEarthMiles(seg.start, seg.end);
}

//******************** DeliveryPlanner functions ******************************

// These functions simply delegate to DeliveryPlannerImpl's functions.
// You probably don't want to change any of this code.

DeliveryPlanner::DeliveryPlanner(const StreetMap *sm) {
  m_impl = new DeliveryPlannerImpl(sm);
}

DeliveryPlanner::~DeliveryPlanner() {
  delete m_impl;
}

DeliveryResult DeliveryPlanner::generateDeliveryPlan(
	const GeoCoord &depot,
	const vector<DeliveryRequest> &deliveries,
	vector<DeliveryCommand> &commands,
	double &totalDistanceTravelled) const {
  return m_impl->generateDeliveryPlan(depot, deliveries, commands, totalDistanceTravelled);
}
