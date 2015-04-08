// STD includes
#include <iostream>
#include <string>
#include <map>
#include <chrono>
#include <algorithm>
#include <utility>
#include <vector>
#include <cmath>
#include <stdlib.h>
#include <limits>

// boost simalator include
#include <boost/simulation.hpp>

// XML parser include
#include "tinyXML/tinyxml.h"

// model includes
#include "atomic-models/filter.hpp"
#include "atomic-models/reaction.hpp"

// data structure includes
#include "data-structures/unit_definition.hpp"
#include "data-structures/message.hpp"
#include "data-structures/types.hpp"

#define TIXML_USE_STL

using namespace boost::simulation;
using namespace boost::simulation::pdevs;
using namespace boost::simulation::pdevs::basic_models;
using namespace std;


/***************************************/
/********* Type definations ************/
/***************************************/

typedef double Time;
typedef chrono::high_resolution_clock hclock;
typedef vector< shared_ptr< model<Time> > > vectorOfModels;
typedef vector< pair< shared_ptr< model<Time> >, shared_ptr< model<Time> > > > vectorOfModelPairs;



/***************************************/
/******** End type definations *********/
/***************************************/


/**************************************************************************************************************/
/***************************************** Testing models *****************************************************/
/**************************************************************************************************************/


int main () {

  srand(time(NULL));
  SetOfMolecules react_stoichiometry, prod_stoichiometry;
  string reaction_name;
  vectorOfModels models, eic, eoc, m;
  vectorOfModelPairs ic;
  Message msg;
  string specie_names[26] = {"A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z"};

  cout << "Creating the reaction models" << endl;
  for (int i = 0; i < 20; ++i){
    for (int j = 0; j < 100; ++j){
      react_stoichiometry.clear();
      prod_stoichiometry.clear();
      reaction_name                                       = "r" + to_string((i*100)+j);
      react_stoichiometry[specie_names[((i*2)+0) % 26]]   = 1;
      react_stoichiometry[specie_names[((i*2)+1) % 26]]   = 1;
      prod_stoichiometry[specie_names[((i*2)+2) % 26]]    = 1;
      prod_stoichiometry[specie_names[((i*2)+3) % 26]]    = 1;

      models.push_back( make_atomic_ptr<reaction<Time, Message>, string, bool, Time, SetOfMolecules, SetOfMolecules, int, Time>(reaction_name, false, 0.000001, react_stoichiometry, prod_stoichiometry, 100, 0.000001) );
    }
  }

  // puting a lots of reactions together and look the performance
  cout << "Creating lists" << endl;
  for (int i = 0; i < 19; ++i){
    for (int j = 0; j < 100; ++j){
      for (int l = 0; l < 100; ++l){
        ic.push_back(make_pair(models[i*100+j], models[i*100+100+l]));
      }
    }
  }

  for (int i = 0; i < 100; ++i) {
    eic.push_back(models[i]);
  }
  
  for (int i = 1900; i < 2000; ++i) {
    eoc.push_back(models[i]);
  }
  cout << "Coupling the reaction models" << endl;
  shared_ptr< flattened_coupled<Time, Message> > cell( new flattened_coupled<Time, Message>{models, eic, ic, eoc});
  
  /*
  //tracing a reaction chain starting with A and B and ending with O and P.
  for (int i = 0; i < 1900; i +=100) {
    ic.push_back(make_pair(models[i], models[i+100]));
    m.push_back(models[i]);
  }

  m.push_back(models[1900]);

  eic.push_back(models[0]);
  eoc.push_back(models[1900]);
  cout << "Coupling the reaction models" << endl;
  shared_ptr< flattened_coupled<Time, Message> > cell( new flattened_coupled<Time, Message>{m, eic, ic, eoc});
  */
  
  cout << "Creating the model to insert the input from stream" << endl;
  auto piss = make_shared<istringstream>();
  piss->str("1 | A 1 \n 1 | B 1");
  
  auto pf = make_atomic_ptr<external_events<Time, Message, Time, string >, shared_ptr<istringstream>, Time>(piss, Time(0),
    [](const string& s, Time& t_next, Message& m_next)->void{ 

    // Parsing function
    // Intermediary vars for casting
    int delimiter;
    string send_to;
    string collector;
    string thrash;
    stringstream ss;
    Message msg_out;

    ss.str(s);
    ss >> t_next;

    ss >> collector;

    while (collector != "|") {     
      send_to = "";

      do {

        if (send_to != "") send_to += " ";
        send_to += collector;
        ss >> collector;

      } while (send_to.at(send_to.size()-1) != '}');
      
      send_to.erase(send_to.begin());
      send_to.erase(send_to.end()-1);
      delimiter = send_to.find(",");


      msg_out.sendTo(send_to.substr(0, delimiter), send_to.substr(delimiter+1, -1));
    }

    ss >> msg_out.specie;
    ss >> msg_out.amount;

    m_next = msg_out;
    ss >> thrash;
    if ( 0 != thrash.size()) throw exception();
  });


  cout << "Coupling the input to the model" << endl;
  shared_ptr< flattened_coupled<Time, Message> > root( new flattened_coupled<Time, Message>{{pf, cell}, {}, {{pf,cell}}, {cell}});

  cout << "Preparing runner" << endl;
  Time initial_time{0};
  runner<Time, Message> r(root, initial_time, cout, [](ostream& os, Message m){  os << "specie: " << m.specie << endl << "amount: " << m.amount << endl; });

  cout << "Starting simulation until passivate" << endl;

  auto start = hclock::now(); //to measure simulation execution time

  r.runUntilPassivate();

  auto elapsed = chrono::duration_cast< chrono::duration< Time, ratio<1> > > (hclock::now() - start).count();

  cout << "Simulation took:" << elapsed << "sec" << endl;

  /**************************************************************************************************************/
  /********************************************* End Models *****************************************************/
  /**************************************************************************************************************/

  return 0;
}


/**************************************************************************************************************/
/*************************************** End Testing models ***************************************************/
/**************************************************************************************************************/



