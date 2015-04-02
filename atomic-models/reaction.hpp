#ifndef BOOST_SIMULATION_PDEVS_REACTION_H
#define BOOST_SIMULATION_PDEVS_REACTION_H
#include <string>
#include <vector>
#include <list>
#include <map>
#include <utility> // pair
#include <algorithm> // min, max, lowe_bound
#include <memory> // shared_ptr
#include <random> // rand
#include <limits> // numeric_limits
#include <boost/simulation/pdevs/atomic.hpp> // boost simalator include
#include "../data-structures/types.hpp" // SetOfMolecules, Task, Rstate, Way, TaskQueue



using namespace boost::simulation::pdevs;
using namespace std;


template<class TIME, class MSG>
class reaction : public atomic<TIME, MSG>
{

private:
  // enzyme information
  string          _name;
  bool            _reversible;
  TIME            _rate;
  SetOfMolecules  _reactants_sctry;
  SetOfMolecules  _products_sctry;
  int             _amount;
  TIME            _interval_time;
  // elements bound
  SetOfMolecules  _reactants;
  SetOfMolecules  _products;
  TaskQueue<TIME> _tasks;


public:

  explicit reaction(
    const string&           other_name,
    const bool&             other_reversible,
    const TIME&             other_rate,
    const SetOfMolecules&   other_reactants_sctry,
    const SetOfMolecules&   other_products_sctry,
    const int               other_amount,
    const TIME&             other_interval_time
  ) noexcept :
  _name(other_name),
  _reversible(other_reversible),
  _rate(other_rate),
  _reactants_sctry(other_reactants_sctry),
  _products_sctry(other_products_sctry),
  _amount(other_amount),
  _interval_time(other_interval_time) {

    for (SetOfMolecules::const_iterator it = _reactants_sctry.cbegin(); it != _reactants_sctry.cend(); ++it) {
      _reactants[it->first] = 0;
    }

    for (SetOfMolecules::const_iterator it = _products_sctry.cbegin(); it != _products_sctry.cend(); ++it) {
      _products[it->first] = 0;
    }
  }

  void internal() noexcept {
    bool reactants_clean, products_clean;
    int amount_leaving;
    Task<TIME> to_reject, new_programed_selection;
    bool already_selected_them  = false;
    Task<TIME> curr_task        = _tasks.front();
    TIME current_time           = curr_task.time_left;

    // Updating time left
    this->updateTaskTimeLefts(current_time);


    // Processing all the tasks with time left 0 (happening now)
    while(curr_task.time_left == 0) {

      if ((curr_task.task_kind == SELECTING) && !already_selected_them) {

        for (SetOfMolecules::iterator it = _reactants.begin(); it != _reactants.end(); ++it) {

          amount_leaving  = rand() % (it->second + 1);
          it->second      -= amount_leaving;
          addRejected(to_reject.rejected, it->first, amount_leaving);
        }

        for (SetOfMolecules::iterator it = _products.begin(); it != _products.end(); ++it) {

          amount_leaving  = rand() % (it->second + 1);
          it->second      -= amount_leaving;
          addRejected(to_reject.rejected, it->first, amount_leaving);
        }

        if (to_reject.rejected.size() > 0) {

          to_reject.time_left = TIME(0);
          to_reject.task_kind = REJECTING;
          this->innsertTask(to_reject);
        }

        reactants_clean = isClean(_reactants);
        products_clean  = isClean(_products);

        if (!reactants_clean || !products_clean) {

          new_programed_selection.time_left = _interval_time;
          new_programed_selection.task_kind = SELECTING;
          this->innsertTask(new_programed_selection);
        }

        already_selected_them = true;
      } else if (curr_task.task_kind == REACTING) {

        _amount += curr_task.reaction.second;
      }

      _tasks.pop_front();
      curr_task = _tasks.front();
    }
  }

  TIME advance() const noexcept {
    if (_tasks.size() > 0) return _tasks.front().time_left;
    else                   return atomic<TIME, MSG>::infinity;
  }

  vector<MSG> out() const noexcept {
    MSG new_message;
    const SetOfMolecules* curr_sctry;

    vector<MSG> result = {};
    TIME current_time  = _tasks.front().time_left;

    for (typename TaskQueue<TIME>::const_iterator it = _tasks.cbegin(); it != _tasks.cend(); ++it) {
      
      if (it->time_left > current_time) break;

      if (it->task_kind == REJECTING) {
        
        for (SetOfMolecules::const_iterator jt = it->rejected.cbegin(); jt != it->rejected.cend(); ++jt) {
          new_message.clear();
          new_message.specie = jt->first;
          new_message.amount = jt->second;
          result.push_back(new_message); 
        }
      } else if (it->task_kind == REACTING) {

        if (it->reaction.first == RTP)      curr_sctry = &_products_sctry;
        else if (it->reaction.first == PTR) curr_sctry = &_reactants_sctry;

        for (SetOfMolecules::const_iterator jt = curr_sctry->cbegin(); jt != curr_sctry->cend(); ++jt) {
          new_message.clear();
          new_message.specie = jt->first;
          new_message.amount = it->reaction.second * jt->second;
          result.push_back(new_message); 
        }
      }
    }

    return result;
  }

  void external(const vector<MSG>& mb, const TIME& t) noexcept {
    int reactant_ready, product_ready, free_of_reactants, free_of_products, intersection;
    bool reactants_clean, products_clean, selection_already_programed;
    Task<TIME> to_reject, rtp, ptr, new_programed_selection;
    // Updating time left
    this->updateTaskTimeLefts(t);

    // inserting new metaboolits and rejecting the surplus
    this->bindMetabolitsAndDropSurplus(mb, to_reject);

    if (to_reject.rejected.size() > 0) {

      to_reject.time_left = TIME(0);
      to_reject.task_kind = REJECTING;
      this->innsertTask(to_reject);
    }

    // looking for new reactions
    reactant_ready  = this->totalReadyFor(RTP);
    product_ready   = this->totalReadyFor(PTR);
    intersection    = rand() % (min(reactant_ready, product_ready) + 1); // tengo que mejorar este random
    reactant_ready  -= intersection;
    product_ready   -= intersection;

    this->deleteUsedMetabolics(reactant_ready, product_ready);

    if (reactant_ready > 0) {
      rtp.time_left = _rate;
      rtp.task_kind = REACTING;
      rtp.reaction  = make_pair(RTP, reactant_ready);
      this->innsertTask(rtp);
    }

    if (product_ready > 0) {
      ptr.time_left = _rate;
      ptr.task_kind = REACTING;
      ptr.reaction  = make_pair(PTR, product_ready);
      this->innsertTask(ptr);
    }

    reactants_clean = isClean(_reactants);
    products_clean  = isClean(_products);
    if ((!reactants_clean || !products_clean) && !this->thereIsNextSelection()) {

      new_programed_selection.time_left = _interval_time;
      new_programed_selection.task_kind = SELECTING;
      this->innsertTask(new_programed_selection);
    }
  }

  virtual void confluence(const vector<MSG>& mb, const TIME& t) noexcept {

    internal();
    external(mb, t);
  }

  /***************************************
  ********* helper functions *************
  ***************************************/

  // It take the needed number of each specie in mb and rejected (by calling addRejected) the not needed part.
  void bindMetabolitsAndDropSurplus(const vector<MSG>& mb, Task<TIME>& tr) {
    int free_space, metabolites_taken_r, metabolites_taken_p, amount_for_r, amount_for_p;
    bool is_reactant, is_product;

    for (typename vector<MSG>::const_iterator it = mb.cbegin(); it != mb.cend(); ++it) {
      
      metabolites_taken_r = 0;
      metabolites_taken_p = 0;
      is_reactant         = _reactants.find(it->specie) != _reactants.end();
      is_product          = _products.find(it->specie) != _products.end();

      // dividing the metabolit between reactants and products
      if (is_reactant && is_product && _reversible) {
        amount_for_r = rand() % (it->amount + 1);
        amount_for_p = it->amount - amount_for_r;
      } else {
        amount_for_r = it->amount;
        amount_for_p = it->amount;
      }

      // binding the allowed amount of metabolits
      if (is_reactant){

        free_space                =   (_amount * _reactants_sctry.at(it->specie)) - _reactants.at(it->specie);
        metabolites_taken_r       =   min(amount_for_r, free_space);
        _reactants.at(it->specie) +=  metabolites_taken_r;
      } 

      if (is_product && _reversible) {
      
        free_space                =   (_amount * _products_sctry.at(it->specie)) - _products.at(it->specie);
        metabolites_taken_p       =   min(amount_for_p, free_space);
        _products.at(it->specie)  +=  metabolites_taken_p;
      }
      
      // placing the surplus metabolites in the rejected list
      addRejected(tr.rejected, it->specie, it->amount - (metabolites_taken_p + metabolites_taken_r));
    }
  }

  // Decrease the time left of all the current tasks in _tasks by the parameter t.
  void updateTaskTimeLefts(const TIME& t){

    for (typename TaskQueue<TIME>::iterator it = _tasks.begin(); it != _tasks.end(); ++it) {
      it->time_left -= t;
    }
  }

  // Add the rejected amount of the species specified in n by inserting/increasing the amount a in the set Of Molecule (garbage set) t.
  void addRejected(SetOfMolecules& t, string n, int a){

    if (a > 0) {
      if (t.find(n) != t.end()) {
        t.at(n) += a;
      } else {
        t[n] = a;
      }
    }
  }

  // It decrease the number of reactants and products removing the specified number by the parameters r and p.
  void deleteUsedMetabolics(int r, int p) {

    for (SetOfMolecules::iterator it = _reactants.begin(); it != _reactants.end(); ++it) {
      it->second -= r * _reactants_sctry.at(it->first); 
    } 

    for (SetOfMolecules::iterator it = _products.begin(); it != _products.end(); ++it) {
      it->second -= p * _products_sctry.at(it->first); 
    }

    _amount -= r + p;
  }

  // Insert a copy of the parameter t in the TaskQueue in an ordered way.
  void innsertTask(const Task<TIME>& t) {

    typename TaskQueue<TIME>::iterator it = lower_bound(_tasks.begin(), _tasks.end(), t);
    _tasks.insert(it, t);
  }

  // It return the total number of enzymes that are ready to react in the way specified by the parameter d.
  int totalReadyFor(Way d) const {
    int fr;

    const SetOfMolecules *curr_metabolics; 
    const SetOfMolecules *curr_sctry;

    if (d == RTP) {
      curr_metabolics = &_reactants;
      curr_sctry      = &_reactants_sctry;
      fr              = this->freeFor(RTP);
    } else {
      curr_metabolics = &_products;
      curr_sctry      = &_products_sctry;
      fr              = this->freeFor(PTR);
    }

    int m = numeric_limits<int>::max();
    for (SetOfMolecules::const_iterator it = curr_metabolics->cbegin(); it != curr_metabolics->cend(); ++it)
      m = min(m, (it->second / curr_sctry->at(it->first)));

    return min(m,fr);
  }

  //usando la stoichiometry y el _amount calcula cuanto es la cantidad de enzymas que estan libres de
  // ese set de elementos. mira la maximo elemento que aparece y cuantas enzymas este ocupa.
  int freeFor(Way d) const {
    
    const SetOfMolecules *curr_metabolics;
    const SetOfMolecules *curr_sctry;

    if (d == RTP) {
      curr_metabolics = &_reactants;
      curr_sctry      = &_reactants_sctry;
    } else {
      curr_metabolics = &_products;
      curr_sctry      = &_products_sctry;
    }

    int m = 0;
    for (SetOfMolecules::const_iterator it = curr_metabolics->cbegin(); it != curr_metabolics->cend(); ++it)
      m = max(m, (it->second / curr_sctry->at(it->first)) + 1); 

    return m;
  }

  bool isClean(const SetOfMolecules& t) {

    bool result = true;
    for (SetOfMolecules::const_iterator it = t.cbegin(); it != t.cend(); ++it) {
      if (it->second != 0) {
        result = false;
        break;
      }
    }

    return result;
  }

  bool thereIsNextSelection() {
    bool result = false;

    for (typename TaskQueue<TIME>::iterator it = _tasks.begin(); it != _tasks.end(); ++it) {
      if (it->task_kind == SELECTING) {
        result = true;
        break;
      }
    }

    return result;
  }

  /*********************************************/
  /************** Testing functions ************/
  /*********************************************/

  ostream& show(ostream& os, const SetOfMolecules& to) {

    os << "[";

    SetOfMolecules::const_iterator it = to.cbegin();
    while ( it != to.cend()) {
      os << "(" << it->first << "," << it->second << ")";
      ++it;
      if (it != to.cend()) os << ",";
    }
    os << "]";
    return os;
  }

  ostream& show(ostream& os, const Task<TIME>& to) {

    string kind, w;
    if (to.task_kind == SELECTING)        kind = "SELECTING";
    else if (to.task_kind == REJECTING)   kind = "REJECTING";
    else if (to.task_kind == REACTING)    kind = "REACTING";

    os << "Task Kind: " << kind << endl;
    os << "Time left: " << to.time_left << endl;

    if(to.task_kind == REJECTING) {
      show(os, to.rejected);
    } else if (to.task_kind == REACTING) {

      if (to.reaction.first == RTP)       w = "RTP";
      else if (to.reaction.first == PTR)  w = "PTR";

      os << "way: " << w << " amount: " << to.reaction.second;
    }

    return os;
  }

  ostream& show(ostream& os, const TaskQueue<TIME>& to) {

    os << "Current Tasks in the queue: ";
    for (typename TaskQueue<TIME>::const_iterator it = to.cbegin(); it != to.cend(); ++it) {
      os << endl << endl;
      show(cout, *it);
    }
    return os;
  }

  ostream& show(ostream& os) {

    os << "name: "          << _name                            << endl;
    os << "rate: "          << _rate                            << endl;
    os << "reversible: "    << (_reversible ? "true" : "false") << endl;
    os << "interval time: " << _interval_time                   << endl;
    os << "free enzymes: "  << _amount                          << endl;
    
    os << "react sctry: ";
    show(os, _reactants_sctry);
    os << endl;
    
    os << "prod sctry: ";
    show(os, _products_sctry);
    os << endl;
    
    os << "reactants: ";
    show(os, _reactants);
    os << endl;
    
    os << "products: ";
    show(os, _products);
    os << endl;
    
    os << "schedulled tasks: " << endl;
    show(os, _tasks);
    os << endl;
    
    return os;
  }

};

#endif // BOOST_SIMULATION_PDEVS_REACTION_H
