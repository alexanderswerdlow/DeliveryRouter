#include "provided.h"
#include <vector>
#include <random>
using namespace std;

class DeliveryOptimizerImpl {
 public:
  DeliveryOptimizerImpl(const StreetMap *sm);
  ~DeliveryOptimizerImpl();
  void optimizeDeliveryOrder(
	  const GeoCoord &depot,
	  vector<DeliveryRequest> &deliveries,
	  double &oldCrowDistance,
	  double &newCrowDistance) const;

 private:
  PointToPointRouter router;
  double getActualCrowDist(const GeoCoord &depot, const vector<DeliveryRequest> &deliveries) const;
  double getApproxCrowDist(const GeoCoord &depot, const vector<DeliveryRequest> &deliveries) const;
  void randomlySwapDeliveries(vector<DeliveryRequest> &vec, int a, int b) const;
  pair<int, int> randomlySwapDeliveries(vector<DeliveryRequest> &vec) const;
  int randInt(int min, int max) const;
};

DeliveryOptimizerImpl::DeliveryOptimizerImpl(const StreetMap *sm) : router{sm} {

}

DeliveryOptimizerImpl::~DeliveryOptimizerImpl() {
}

void DeliveryOptimizerImpl::randomlySwapDeliveries(vector<DeliveryRequest> &vec, int a, int b) const {
  swap(vec[a], vec[b]); //Swaps item in vector
}

pair<int, int> DeliveryOptimizerImpl::randomlySwapDeliveries(vector<DeliveryRequest> &vec) const {
  int a = 0;
  int b = 0;
  while (a == b) { //Make sure we don't swap the same two items
	a = randInt(0, vec.size() - 1);
	b = randInt(0, vec.size() - 1);
  }
  randomlySwapDeliveries(vec, a, b);
  return {a, b};
}

void DeliveryOptimizerImpl::optimizeDeliveryOrder(const GeoCoord &depot, vector<DeliveryRequest> &deliveries, double &oldCrowDistance, double &newCrowDistance) const {
  oldCrowDistance = getActualCrowDist(depot, deliveries); //Calculate the total distance without optimization

  double t = sqrt(deliveries.size()); //Variable Start Temperature
  double cooling_rate = 0.995;
  double best_dist = getApproxCrowDist(depot, deliveries);
  vector<DeliveryRequest> currentSolution = deliveries;
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<> dis(0.0, 1.0);
  for (int i = 0; i < 10000; i++) { //Run for 10,000 iterations
	if (t > 1e-8) {
	  pair<int, int> swapped = randomlySwapDeliveries(currentSolution); //Randomly swap pairs of delivery locations
	  double cur_dist = getApproxCrowDist(depot, currentSolution);
	  if (cur_dist < best_dist) {
		best_dist = cur_dist; //This iteration is better, record the best distance
	  } else if (exp((best_dist - cur_dist) / t) < dis(gen)) { //If it's worse, swap back to what we had before
		randomlySwapDeliveries(currentSolution, swapped.second, swapped.first);
	  }
	  t *= cooling_rate;
	} else {
	  break;
	}
  }

  newCrowDistance = getActualCrowDist(depot, currentSolution); //Calculate our final distance traveled
  if (newCrowDistance < oldCrowDistance) { //If our optimization actually made things worse, just ignore it and stick with what we had. This should rarely happen.
	deliveries = currentSolution;
  } else {
	newCrowDistance = oldCrowDistance;
  }
}

//Generates the actual routes using PointToPointRouter and returns the exact distance
double DeliveryOptimizerImpl::getActualCrowDist(const GeoCoord &depot, const vector<DeliveryRequest> &deliveries) const {
  double temp_distance = 0;
  double total_dist = 0;
  list<StreetSegment> temp;
  router.generatePointToPointRoute(depot, deliveries[0].location, temp, temp_distance);
  total_dist += temp_distance;

  for (int i = 0; i < deliveries.size() - 1; i++) {
	router.generatePointToPointRoute(deliveries[i].location, deliveries[i + 1].location, temp, temp_distance);
	total_dist += temp_distance;
  }

  router.generatePointToPointRoute(deliveries[deliveries.size() - 1].location, depot, temp, temp_distance);
  total_dist += temp_distance;

  return total_dist;
}

//Approximate the distance of a route by summing the 'euclidean' distance between the delivery points/depot.
//This is much faster than generating the entire route and is generally a good approximation for the total distance the routes will take
double DeliveryOptimizerImpl::getApproxCrowDist(const GeoCoord &depot, const vector<DeliveryRequest> &deliveries) const {
  double total_approx_dist = 0;
  total_approx_dist += distanceEarthMiles(depot, deliveries[0].location);

  for (int i = 0; i < deliveries.size() - 1; i++) {
	total_approx_dist += distanceEarthMiles(deliveries[i].location, deliveries[i + 1].location);
  }
  total_approx_dist += distanceEarthMiles(deliveries[deliveries.size() - 1].location, depot);
  return total_approx_dist;
}

int DeliveryOptimizerImpl::randInt(int min, int max) const { //From Project 3 provided.h
  if (max < min)
	std::swap(max, min);
  static std::random_device rd;
  static std::default_random_engine generator(rd());
  std::uniform_int_distribution<> distro(min, max);
  return distro(generator);
}

//******************** DeliveryOptimizer functions ****************************

// These functions simply delegate to DeliveryOptimizerImpl's functions.
// You probably don't want to change any of this code.

DeliveryOptimizer::DeliveryOptimizer(const StreetMap *sm) {
  m_impl = new DeliveryOptimizerImpl(sm);
}

DeliveryOptimizer::~DeliveryOptimizer() {
  delete m_impl;
}

void DeliveryOptimizer::optimizeDeliveryOrder(
	const GeoCoord &depot,
	vector<DeliveryRequest> &deliveries,
	double &oldCrowDistance,
	double &newCrowDistance) const {
  return m_impl->optimizeDeliveryOrder(depot, deliveries, oldCrowDistance, newCrowDistance);
}
