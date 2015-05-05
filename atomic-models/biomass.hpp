#ifndef BOOST_SIMULATION_PDEVS_CONTROLER_H
#define BOOST_SIMULATION_PDEVS_CONTROLER_H
#include <string>
#include <utility>
#include <map>
#include <math.h>
#include <random>
#include <limits>
#include <memory>

#include <boost/simulation/pdevs/atomic.hpp> // boost simalator include

#include "../data-structures/types.hpp" // metabolite_info_t, enzyme_info_t, SState_t, Integer_t


using namespace boost::simulation::pdevs;
using namespace std;

template<class TIME, class MSG>
class biomass : public atomic<TIME, MSG>
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
  BState                                _s;

public:

  explicit biomass(
    const string                                other_id,
    const shared_ptr< map<string, Address_t> >  other_addresses,
    const SetOfMolecules_t&                     other_reactants_sctry,
    const SetOfMolecules_t&                     other_products_sctry,
    const Address_t                             other_request_addresses,
    const TIME                                  other_interval_time,
    const TIME                                  other_rate,
    ) noexcept :
  _id(other_id),
  _addresses(other_addresses),
  _reactants_sctry(other_reactants_sctry),
  _products_sctry(other_products_sctry),
  _request_addresses(other_request_addresses),
  _interval_time(other_interval_time),
  _rate(other_rate),
  _s(BState::NOTHING) {

    _reactants.clear();
    _rejected.clear();

    for (SetOfMolecules_t::const_iterator it = _reactants_sctry.cbegin(); it != _reactants_sctry.cend(); ++it) {
      _reactants.insert({it->first, 0});
    }
  }

  void internal() noexcept {
    
    _rejected.clear()
    
    for (SetOfMolecules_t::iterator it = _reactants.begin(); it != _reactants.end(); ++it) {
      it->second = 0;
    }

    _s = BState::NOTHING;
  }

  TIME advance() const noexcept {

    return (_s != BState::NOTHING) ? _rate : _interval_time;
  }

  vector<MSG> out() const noexcept {
    vector<MSG> output;
    MSG curr_message;

    if (_s == BState::ENOUGH) {
      for (SetOfMolecules_t::cosnt_iterator it = _products_sctry.cbegin(); it != _products_sctry.cend(); ++it) {

        curr_message.to     = _addresses->at(it->first);
        curr_message.specie = it->first;
        curr_message.amount = it->second;
        output.push_back(curr_message);
      }
    } else if (_s == BState::NOT_ENOUGH) {
      for (SetOfMolecules_t::cosnt_iterator it = _reactants.cbegin(); it != _reactants.cend(); ++it) {

        curr_message.to     = _addresses->at(it->first);
        curr_message.specie = it->first;
        curr_message.amount = it->second;
        output.push_back(curr_message);
      }
    } else if (_s == BState::NOTHING) {
      curr_message.to               = _request_addresses;
      curr_message.specie           = "";
      curr_message.amount           = 0;
      curr_message.biomass_request  = true;
      output.push_back(curr_message);
    }

    for for (SetOfMolecules_t::cosnt_iterator it = _rejected.cbegin(); it != _rejected.cend(); ++it) {
      
      curr_message.to     = _addresses->at(it->first);
      curr_message.specie = it->first;
      curr_message.amount = it->second;
      output.push_back(curr_message);
    }

    return output;
  }

  void external(const vector<MSG>& mb, const TIME& t) noexcept {

    Integer_t needed_amount, taked_amount, rejected_amount;
    bool is_needed;
    for (typename vector<MSG>::const_iterator it = mb.cbegin(); it != mb.cend(); ++it) {

      is_needed = _stoichiometry.find(it->specie) != _stoichiometry.end();
      
      if (is_needed) {

        needed_amount   = _stoichiometry.at(it->specie) - _reactants.at(it->specie);
        taked_amount    = min(it->amount, needed_amount)
        rejected_amount = it-amount - taked_amount;

        this->addReactant(it->specie, taked_amount);
        this->addRejectedMolecules(it->specie, rejected_amount);
      }
    }

    if (this->thereIsEnoughReactants())     _s = BState::ENOUGH;
    else if (this->thereIsSomeReactants())  _s = BState::NOT_ENOUGH;
    else                                    _s = BState::NOTHING;
  }

  virtual void confluence(const std::vector<MSG>& mb, const TIME& t) noexcept {
    
    internal();
    external(mb, TIME(0));
    
  }

  /***************************************
  ********* helper functions *************
  ***************************************/

  void addReactant(const string& e, const Integer_t& a) {

    _reactants.at(e) += a;
  }

  void addReactant(const string& e, const Integer_t& a) {

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
  

#endif // BOOST_SIMULATION_PDEVS_CONTROLER_H
