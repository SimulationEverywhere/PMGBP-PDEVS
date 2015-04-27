#include "types.hpp"

ostream& operator<<(ostream& os, Address to) {
  
  os << "[";
  Address::iterator i = to.begin();
  while(i != to.end()){
    os << *i;
    ++i;
    if (i != to.end()) os << ", ";
  }
  os << "]";
  return os;
}

ostream& operator<<(ostream& os, Message msg) {
  os << "To: " << endl << msg.to << endl;
  os << "Specie: " << msg.specie << endl;
  os << "Amount: " << msg.amount << endl;
  return os;
}

ostream& operator<<(ostream& os, vector<string> m) {
  
  os << "[";
  vector<string>::iterator i = m.begin();
  while(i != m.end()){
    os << *i;
    ++i;
    if (i != m.end()) os << ", ";
  }
  os << "]";
  return os;
}
