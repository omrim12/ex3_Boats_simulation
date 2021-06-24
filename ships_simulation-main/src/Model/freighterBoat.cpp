#include "freighterBoat.h"

freighterBoat::freighterBoat(string &boat_name, int cont_cap, int res) : Boat(boat_name, MAX_FRI_FUEL, res, cont_cap),
                                                                         MAX_CONTAINERS_CAPACITY(cont_cap) {};

/*************************************/
void freighterBoat::course(int deg, double speed) {
    status = Move_to_Course;
    direction = direction(deg);
    curr_speed = speed;
    dest_port.reset();
    type = None;
}

/*************************************/
void freighterBoat::position(double x, double y, double speed) {
    status = Move_to_Position;
    direction = Direction(Location(x, y));
    curr_speed = speed;
    dest_port.reset();
    type = None;
}

/*************************************/
void freighterBoat::destination(weak_ptr<Port> port, double speed) {
    if (dest_is_load(port)) type = load;
    else if (dest_is_unload(port)) type = unload;
    else type = None;
    status = Move_to_Dest;
    direction = Direction(port.lock()->get_Location());
    dest_port = std::move(port);
    curr_speed = speed;
}

/*************************************/
void freighterBoat::dock(weak_ptr<Port> port) {
    if (curr_Location.distance_from(port.lock()->get_Location()) <= 0.1) {
        curr_speed = 0;
        curr_Location = port.lock()->get_Location();
        if (status == Move_to_Dest) {
            if (*port.lock().get() == *dest_port.lock().get()) {    ///???operator==
                //case of docking port is same as destination port:
                //*check if destination is from load/unload type
                switch (type) {
                    case (load):
                        load_boat();
                        break;
                    case (unload):
                        unload_boat();
                        break;
                    default:
                        break;
                }
            }
        }
        status = Docked;
        type = None;

    } else cerr << "WARNING" << port.lock()->getPortName() << " is too far for Docking. " << endl;
}

/*************************************/
void freighterBoat::refuel() {
    std::shared_ptr<Boat> me(this);
    dest_port.lock()->addToQueue(weak_ptr<Boat>(me));
    waiting_in_fuel_queue = true;
    ask_fuel = false;
    return;
}

/*************************************/
void freighterBoat::stop() {
    status = Stopped;
    curr_speed = 0;
    dest_port.reset();
    available = true; //available for other orders
}

/*************************************/
void freighterBoat::unload_boat() {
    int to_unload;
    for (auto &p: ports_to_unload) {
        if (*p.port.lock().get() == *dest_port.lock().get()) {   ///???operator ==
            to_unload = p.capacity;
            break;
        }
    }

    if (curr_num_of_containers < to_unload) {
        dest_port.lock()->load_port(curr_num_of_containers);
        curr_num_of_containers = 0;
        type = None;
        cerr << "WARNING: containers capacity at " << name << " boat is smaller then required to unload. " << endl;
        return;
    }
    //else
    dest_port.lock()->load_port(to_unload);
    curr_num_of_containers -= to_unload;
    ports_to_unload.remove(ports_to_unload.begin(), ports_to_unload.end(), dest_port);
    type = None;
}

/*************************************/

void freighterBoat::load_boat() {
    dest_port.lock()->unload_port(MAX_CONTAINERS_CAPACITY - curr_num_of_containers);
    curr_num_of_containers = MAX_CONTAINERS_CAPACITY;
    //delete port from load ports list
    ports_to_load.remove(ports_to_load.begin(), ports_to_load.end(), dest_port);
}

/*************************************/

void freighterBoat::in_dock_status() {
    if (ask_fuel) {
        refuel();
    }
}

/*************************************/
void freighterBoat::in_move_status() {
    Location next_Location = curr_Location.next_Location(direction, curr_speed));
    double use_fuel = curr_Location.distance_from(next_Location) * FUEL_PER_NM;

    if (curr_fuel - use_fuel <= 0) {
        if (curr_Location != dest_Location) {
            status = Dead;
        }
    } else {
        curr_fuel -= use_fuel;
        curr_Location = next_Location;
    }
}

/*************************************/
ostream &operator<<(ostream &out, const freighterBoat &ship) {

    string stat_string = "";

    switch (ship.status) {
        case (Move_to_Dest):
            stat_string +=
                    "Moving to " + ship.dest_port.lock()->getPortName() + " on course " + ship.direction.get_degree() + " deg"+ ", speed " + ship.curr_speed + " nm/hr " + " Containers: " + ship.curr_num_of_containers;
            switch(type){
                case(load):
                    stat_string += "moving to loading destination. ";
                    break;
                case(unload):
                    stat_string +="moving to unloading destination. ";
                    break;
                case(None):
                    stat_string += "no cargo destinations. ";
                    break;
            }
            break;
        case (Move_to_Position):
            stat_string += "Moving to position " + ship.dest_Location + ", speed " + ship.curr_speed + " nm/hr " + " Containers: " + ship.curr_num_of_containers;
            break;
        case (Move_to_Course):
            stat_string += "Moving on course " + ship.direction.get_degree() + ", speed " + ship.curr_speed + " nm/hr " + " Containers: " + ship.curr_num_of_containers;
            break;
        case (Docked):
            stat_string += "Docked at " + ship.destPortName;
            break;

        case (Dead):
            stat_string += "Dead in the water";
            break;

        case (Stopped):
            stat_string += "Stopped";
            break;

        default:
            cerr << "WTFFFFF" << endl;
            break;
    }

    stat_string += ", ";

    out << "Freighter " << ship.name << " at " << ship.curr_Location << ", fuel: " << ship.curr_fuel << ", resistance: "
        << ship.resistance << ", " << stat_string;

    return out;
}

/*************************************/