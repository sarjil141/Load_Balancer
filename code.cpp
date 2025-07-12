#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <set>
#include <queue>
#include <algorithm>
#include <stdexcept>
#include <functional>
#include <memory>

using namespace std;

class Request {
public:
    string id;
    string requestType;
    unordered_map<string, string> parameters;
    // Paramaters - Additional information that might be needed to process the request
    // Like user preferences, special instructions, or any metadata
    // Resolution : 1080p
    // Format : MP4
    // Priority : High
};

class Destination {
public:
    string ipAddress;
    int requestsBeingServed = 0;
    int threshold;

    Destination(string ip, int thresh)
        : ipAddress(ip), threshold(thresh) {}

    bool acceptRequest(Request& request) {
        if (requestsBeingServed < threshold) {
            requestsBeingServed++;
            cout << "Request accepted by " << ipAddress << ". Currently serving: " 
                 << requestsBeingServed << " requests.\n";
            return true;
        }
        cout << "Request rejected by " << ipAddress << " (overloaded).\n";
        return false;
    }

    void completeRequest() {
        if (requestsBeingServed > 0) {
            requestsBeingServed--;
            cout << "Request completed by " << ipAddress << ". Currently serving: " 
                 << requestsBeingServed << " requests.\n";
        }
    }
};

class Service {
public:
    string name;
    set<Destination*> destinations;

    void addDestination(Destination* destination) {
        destinations.insert(destination);
    }

    void removeDestination(Destination* destination) {
        destinations.erase(destination);
    }
};

class LoadBalancer {
protected:
    unordered_map<string, Service*> serviceMap;

public:
    void registerService(const string& requestType, Service* service) {
        serviceMap[requestType] = service;
    }

    set<Destination*>& getDestinations(Request& request) {
        if (serviceMap.find(request.requestType) == serviceMap.end()) {
            throw runtime_error("No service found for the request type.");
        }
        return serviceMap[request.requestType]->destinations;
    }

    virtual Destination* balanceLoad(Request& request) = 0;
};

class LeastConnectionLoadBalancer : public LoadBalancer {
public:
    Destination* balanceLoad(Request& request) override {
        auto& destinations = getDestinations(request);
        if (destinations.empty()) throw runtime_error("No destinations available.");

        return *min_element(destinations.begin(), destinations.end(),
                            [](Destination* a, Destination* b) {
                                return a->requestsBeingServed < b->requestsBeingServed;
                            });
    }
};

class RoutedLoadBalancer : public LoadBalancer {
public:
    Destination* balanceLoad(Request& request) override {
        auto& destinations = getDestinations(request);
        if (destinations.empty()) throw runtime_error("No destinations available.");

        vector<Destination*> list(destinations.begin(), destinations.end());
        size_t index = hash<string>{}(request.id) % list.size();
        return list[index];
    }
};

class RoundRobinLoadBalancer : public LoadBalancer {
private:
    unordered_map<string, queue<Destination*>> destinationQueues;

public:
    Destination* balanceLoad(Request& request) override {
        auto& destinations = getDestinations(request);
        if (destinations.empty()) throw runtime_error("No destinations available.");

        if (destinationQueues.find(request.requestType) == destinationQueues.end()) {
            queue<Destination*> q;
            for (Destination* dest : destinations) q.push(dest);
            destinationQueues[request.requestType] = q;
        }

        Destination* destination = destinationQueues[request.requestType].front();
        destinationQueues[request.requestType].pop();
        destinationQueues[request.requestType].push(destination);
        return destination;
    }
};

int main() {
    // Setup services and destinations
    Service service;
    Destination dest1{"192.168.0.1", 12};
    Destination dest2{"192.168.0.2", 20};
    Destination dest3{"192.168.0.3", 15};

    service.addDestination(&dest1);
    service.addDestination(&dest2);
    service.addDestination(&dest3);

    // Setup Load Balancers
    LeastConnectionLoadBalancer leastConnectionLB;
    RoutedLoadBalancer routedLB;
    RoundRobinLoadBalancer roundRobinLB;

    leastConnectionLB.registerService("http", &service);
    routedLB.registerService("http", &service);
    roundRobinLB.registerService("http", &service);

    // Main interaction loop
    while (true) {
        cout << "\nChoose load balancing algorithm (1: Least Connection, 2: Routed, 3: Round Robin, 4: Exit): ";
        int choice;
        cin >> choice;

        if (choice == 4) {
            break;
        }

        LoadBalancer* lb = nullptr;
        switch (choice) {
            case 1:
                lb = &leastConnectionLB;
                break;
            case 2:
                lb = &routedLB;
                break;
            case 3:
                lb = &roundRobinLB;
                break;
            default:
                cout << "Invalid choice. Try again.\n";
                continue;
        }

        // Simulate request
        Request request;
        string id;
        cout << "Enter request ID: ";
        cin >> id;
        request.id = "REQ" + id;
        request.requestType = "http";

        try {
            Destination* destination = lb->balanceLoad(request);
            cout << "Request routed to: " << destination->ipAddress << "\n";

            // Simulate request completion (in real life, this would happen asynchronously)
            destination->completeRequest();
        } catch (const exception& e) {
            cout << "Error: " << e.what() << "\n";
            // e -> exception
            // what() -> function of exception that return description of error in terms of string
        }
    }

    return 0;
}
