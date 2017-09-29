#include "types.hpp"

ostream& operator<<(ostream& os, const Address_t& to) {
  
  os << "[";
  Address_t::const_iterator i = to.cbegin();
  while(i != to.cend()){
    os << *i;
    ++i;
    if (i != to.cend()) os << ", ";
  }
  os << "]";
  return os;
}

ostream& operator<<(ostream& os, const Message_t& msg) {
  os << "To: " << msg.to << endl;
  os << "Reactant: " << msg.metabolites << endl;
  os << "reaction way: " << msg.react_direction << endl;
  os << "show request: " << (msg.show_request ? "true" : "false") << endl;
  os << "send biomass: " << (msg.biomass_request ? "true" : "false");
  return os;
}

ostream& operator<<(ostream& os, const vector<string>& m) {
  
  os << "[";
  vector<string>::const_iterator i = m.cbegin();
  while(i != m.cend()){
    os << *i;
    ++i;
    if (i != m.cend()) os << ", ";
  }
  os << "]";
  return os;
}

ostream& operator<<(ostream& os, const MetaboliteAmounts& m) {
  
  os << "[";
  MetaboliteAmounts::const_iterator i = m.cbegin();
  while(i != m.cend()){
    os << i->second << "-" << i->first;
    ++i;
    if (i != m.cend()) os << ", ";
  }
  os << "]";
  return os;
}

ostream& operator<<(ostream& os, const SpaceState& s) {

  switch(s) {
    case SpaceState::SENDING_BIOMASS:
      os << "SENDING_BIOMASS";
      break;
    case SpaceState::SENDING_REACTIONS:
      os << "SENDING_REACTIONS";
      break;
    case SpaceState::SELECTING_FOR_REACTION:
      os << "SELECTING_FOR_REACTION";
      break;
  }

  return os;
}

ostream& operator<<(ostream& os, const BState_t& s) {

  switch(s) {
    case BState_t::IDLE:        os << "IDLE";       break;
    case BState_t::WAITING:     os << "WAITING";    break;
    case BState_t::NOT_ENOUGH:  os << "NOT_ENOUGH"; break;
    case BState_t::ENOUGH:      os << "ENOUGH";     break;
  }

  return os;
}

ostream& operator<<(ostream& os, const Way& s) {

  switch(s) {
    case Way::STP:
      os << "STP";
      break;
    case Way::PTS:
      os << "PTS";
      break;
  }

  return os;
}

ostream& operator<<(ostream& os, const reaction_info_t& r) {

  cout << "id: " << r.id << endl;
  cout << "Address: " << r.location << endl;
  cout << "substrates: " << r.substrate_sctry << endl;
  cout << "products: " << r.products_sctry << endl;
  cout << "KonSTP: " << r.konSTP << endl;
  cout << "KonPTS" << r.konPTS << endl;
  cout << "reversible: " << ((r.reversible) ? "true" : "false");
  return os;
}