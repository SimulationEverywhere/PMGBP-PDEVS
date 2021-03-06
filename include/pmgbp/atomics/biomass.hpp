#ifndef BOOST_SIMULATION_PDEVS_BIOMASS_H
#define BOOST_SIMULATION_PDEVS_BIOMASS_H
#include <string>
#include <utility>
#include <map>
#include <limits>
#include <memory>

#include <boost/simulation/pdevs/atomic.hpp> // boost simalator include

#include "../data-structures/types.hpp"

using namespace boost::simulation::pdevs;
using namespace boost::simulation;
using namespace std;


template<class TIME, class MSG>
class biomass : public pdevs::atomic<TIME, MSG>
{
private:
  string                                _id;
  shared_ptr< map<string, Address_t> >  _addresses;
  SetOfMolecules_t                      _reactants_sctry;
  SetOfMolecules_t                      _products_sctry;
  SetOfMolecules_t                      _reactants;
  SetOfMolecules_t                      _rejected;
  Address_t                             _request_addresses;
  TIME                                  _interval_time;
  TIME                                  _rate;
  BState_t                              _s;
  map<string, bool>                     _compartments;

  // constant values
  TIME                                  ZERO;

public:

  explicit biomass(
    const string                                other_id,
    const shared_ptr< map<string, Address_t> >  other_addresses,
    const SetOfMolecules_t&                     other_reactants_sctry,
    const SetOfMolecules_t&                     other_products_sctry,
    const Address_t                             other_request_addresses,
    const TIME                                  other_interval_time,
    const TIME                                  other_rate,
    const map<string, bool>                     other_compartments
    ) noexcept :
  _id(other_id),
  _addresses(other_addresses),
  _reactants_sctry(other_reactants_sctry),
  _products_sctry(other_products_sctry),
  _request_addresses(other_request_addresses),
  _interval_time(other_interval_time),
  _rate(other_rate),
  _compartments(other_compartments),
  _s(BState_t::IDLE) {

    _reactants.clear();
    _rejected.clear();

    for (SetOfMolecules_t::const_iterator it = _reactants_sctry.cbegin(); it != _reactants_sctry.cend(); ++it) {
      _reactants.insert({it->first, 0});
    }

    this->restartCompartments();

    // Constant values;
    ZERO = TIME(0);
  }

  void internal() noexcept {
    comment("internal init.");
    _rejected.clear();
    
    for (SetOfMolecules_t::iterator it = _reactants.begin(); it != _reactants.end(); ++it) {
      it->second = 0;
    }

    this->restartCompartments();
    if (_s == BState_t::IDLE) _s = BState_t::WAITING;
    else if ((_s == BState_t::ENOUGH) || (_s == BState_t::NOT_ENOUGH)) _s = BState_t::IDLE;
    comment("internal end.");
  }

  TIME advance() const noexcept {
    comment("advance init.");

    TIME result;
    switch(_s) {
    case BState_t::WAITING: result = pdevs::atomic<TIME, MSG>::infinity; break;
    case BState_t::IDLE: result = _interval_time; break;
    case BState_t::NOT_ENOUGH:
    case BState_t::ENOUGH: result = _rate; break;
    }

    if (result <= TIME(0,1)) cout << _id << " " << result << endl;
    comment("advance time result " + result.toStringAsDouble());
    return result;
  }

  vector<MSG> out() const noexcept {
    comment("out init.");

    vector<MSG> output;
    vector<MSG> request;
    MSG curr_message;

    switch(_s) {
    case BState_t::IDLE:

      curr_message.to               = _request_addresses;
      curr_message.show_request     = false;
      curr_message.biomass_request  = true;
      request.push_back(curr_message);
      break;
    case BState_t::NOT_ENOUGH:

      for (SetOfMolecules_t::const_iterator it = _reactants.cbegin(); it != _reactants.cend(); ++it) {
        if (it->second > 0) {

          curr_message.to     = _addresses->at(it->first);
          curr_message.metabolites.insert({it->first, it->second});
          output.push_back(curr_message);
        }
      }
      break;
    case BState_t::ENOUGH:
      for (SetOfMolecules_t::const_iterator it = _products_sctry.cbegin(); it != _products_sctry.cend(); ++it) {
        curr_message.to     = _addresses->at(it->first);
        curr_message.metabolites.insert({it->first, it->second});
        output.push_back(curr_message);
      }
      break;
    }

    for (SetOfMolecules_t::const_iterator it = _rejected.cbegin(); it != _rejected.cend(); ++it) {
      
      curr_message.to     = _addresses->at(it->first);
      curr_message.metabolites.insert({it->first, it->second});
      output.push_back(curr_message);
    } 

    unifyMessages(output);
    output.insert(output.end(), request.begin(), request.end());

    comment("out end.");

    return output;
  }

  void external(const vector<MSG>& mb, const TIME& t) noexcept {
    comment("external init.");

    Integer_t needed_amount, taked_amount, rejected_amount;
    bool is_needed;
    for (typename vector<MSG>::const_iterator it = mb.cbegin(); it != mb.cend(); ++it) {

      _compartments.at(it->from) = true;

      for(SetOfMolecules_t::const_iterator specie = it->metabolites.begin(); specie != it->metabolites.end(); ++specie) {
        is_needed = _reactants_sctry.find(specie->first) != _reactants_sctry.end();
        
        if (is_needed) {

          needed_amount = _reactants_sctry.at(specie->first) - _reactants.at(specie->first);
          taked_amount = min(specie->second, needed_amount);
          this->addReactant(specie->first, taked_amount);

          rejected_amount = specie->second - taked_amount;
        } else {
          
          rejected_amount = specie->second;
        }

        if (rejected_amount > 0)
          this->addRejectedMolecules(specie->first, rejected_amount);
      }
    }

    if (this->thereIsEnoughReactants()) _s = BState_t::ENOUGH;
    else if (this->noMoreCompartmentsToWait()) _s = BState_t::NOT_ENOUGH;
    comment("external end.");
  }

  virtual void confluence(const std::vector<MSG>& mb, const TIME& t) noexcept {
    comment("confluece init.");

    internal();
    external(mb, ZERO);
    
    comment("confluece end.");
  }

  /***************************************
  ********* helper functions *************
  ***************************************/

  void comment(string msg) const {
    if (COMMENTS) cout << "[biomass " << _id << "] " << msg << endl;
  }

  bool noMoreCompartmentsToWait() const {
    bool result = true;
    for (map<string, bool>::const_iterator c = _compartments.cbegin(); c != _compartments.cend(); ++c) {
      result = result && c->second;
    }
    return result;
  }

  void restartCompartments() {
    for (map<string, bool>::iterator c = _compartments.begin(); c != _compartments.end(); ++c) {
      c->second = false;
    }
  }

  void addReactant(const string& e, const Integer_t& a) {

    _reactants.at(e) += a;
  }

  void addRejectedMolecules(const string& e, const Integer_t& a) {

    if (_rejected.find(e) != _rejected.end())
      _rejected.at(e) += a;
    else
      _rejected.insert({e, a});
  }

  bool thereIsEnoughReactants() const {

    bool result = true;

    for (SetOfMolecules_t::const_iterator it = _reactants_sctry.cbegin(); it != _reactants_sctry.cend(); ++it) {      
      if (_reactants.at(it->first) < it->second) {
        result = false;
        break;
      }
    }

    return result;
  }

  bool thereIsSomeReactants() const {

    bool result = false;

    for (SetOfMolecules_t::const_iterator it = _reactants.cbegin(); it != _reactants.cend(); ++it) {      
      if (it->second > 0) {
        result = true;
        break;
      }
    }

    return result;
  }

  void unifyMessages(vector<MSG>& m) const {

    map<Address_t, MSG> unMsgs;

    for (typename vector<MSG>::iterator it = m.begin(); it != m.end(); ++it) {
      insertMessage(unMsgs, *it);
    }

    m.clear();

    for (typename map<Address_t, MSG>::iterator it = unMsgs.begin(); it != unMsgs.end(); ++it) {
      m.push_back(it->second);
    }
  }

  void insertMessage(map<Address_t, MSG>& ms, MSG& m) const {

    if (ms.find(m.to) != ms.end()) {
      addMultipleMetabolites(ms.at(m.to).metabolites, m.metabolites);
    } else {
      ms.insert({m.to, m}); // TODO: change all the initializer_list because they don't work on windows
    }
  }

  void addMultipleMetabolites(SetOfMolecules_t& m, const SetOfMolecules_t& om) const {
  
    for (SetOfMolecules_t::const_iterator it = om.cbegin(); it != om.cend(); ++it) {
      addMetabolite(m, it->first, it->second);
    }
  }

  void addMetabolite(SetOfMolecules_t& m, string n, Integer_t a) const {

    if (a > 0) {
      if (m.find(n) != m.end()) {
        m.at(n) += a;
      } else {
        m.insert({n, a}); // TODO: change all the initializer_list because they don't work on windows
      }
    }
  }
};

#endif // BOOST_SIMULATION_PDEVS_BIOMASS_H

