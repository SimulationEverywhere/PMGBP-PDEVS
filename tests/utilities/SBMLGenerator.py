#!/usr/bin/python
# -*- coding: utf-8 -*-

from lxml import etree
from random import randint, choice


class SBMLTestGenerator():

    def __init__(self, output, model_id, compartments, reaction_amounts, species_amounts):
        self.output_file = open(output, 'w+')
        self.compartments = compartments
        self.reaction_amounts = reaction_amounts
        self.species_amounts = species_amounts

        self.sbml = etree.Element('sbml')
        self.sbml.set('xmlns', 'http://www.sbml.org/sbml/level2')
        self.sbml.set('level', '2')
        self.sbml.set('version', '1')

        self.model = etree.Element('model')
        self.model.set('id', model_id)
        self.sbml.append(self.model)

    def create_listOfCompartments(self):
        self.listOfCompartments = etree.Element('listOfCompartments')
        self.model.append(self.listOfCompartments)

        for cid in self.compartments:
            self.listOfCompartments.append(self.create_compartment(cid, cid + '_name'))

    def create_listOfSpecies(self):
        self.listOfSpecies = etree.Element('listOfSpecies')
        self.model.append(self.listOfSpecies)

        for cid, amount in self.species_amounts.iteritems():
            for sid in (str(i) for i in range(amount)):
                self.listOfSpecies.append(self.create_specie(sid, cid))

    def create_listOfReactions(self):
        self.listOfReactions = etree.Element('listOfReactions')
        self.model.append(self.listOfReactions)

        for (cid, rsn), amount in self.reaction_amounts.iteritems():
            for i in range(amount):
                rid = '_'.join([str(i), cid, rsn])
                self.listOfReactions.append(self.create_reaction(rid, choice(['true', 'false'])))

    def create_compartment(self, cid, name):
        compartment = etree.Element('compartment')
        compartment.set('id', cid)
        compartment.set('name', name)
        return compartment

    def create_specie(self, sid, compartment):
        species = etree.Element('species')
        species.set('id', sid + '_' + compartment)
        species.set('name', sid + '_name_' + compartment)
        species.set('compartment', compartment)
        return species

    def create_reaction(self, rid, reversible, cid, rsn):
        reaction = etree.Element('reaction')
        reaction.set('id', rid)
        reaction.set('name', rid + '_name')
        reaction.set('reversible', reversible)

        reaction.append(self.create_note())

        listOfReactants = etree.Element('listOfReactants')
        for reactant in self.random_stoichiometry(cid, rsn):
            listOfReactants.append(reactant)
        reaction.append(listOfReactants)

        listOfProducts = etree.Element('listOfProducts')
        for reactant in self.random_stoichiometry(cid, rsn):
            listOfProducts.append(reactant)
        reaction.append(listOfProducts)
        return reaction

    def random_stoichiometry(self, cid, rsn):

        related_compartments = [cid]
        if rsn == 'outer':
            related_compartments.append('e')
        elif rsn == 'inner':
            related_compartments.append('c')
        elif rsn == 'trans':
            related_compartments.append('c')
            related_compartments.append('e')

        related_compartments = list(set(related_compartments))  # remove duplicated values

        for compartment in related_compartments:
            specie_ids = [str(i) + '_' + compartment for i in range(self.species_amounts[compartment])]

            for i in range(randint(0, 10)):
                if len(specie_ids) == 0:
                    break
                s = choice(specie_ids)
                specie_ids = list(set(specie_ids) - set([s]))
                specie = etree.Element('speciesReference')
                specie.set('species', s)
                specie.set('stoichiometry', str(choice(range(3))))
                yield specie

    def save_xml(self):
        self.output_file.write(etree.tostring(self.sbml, encoding='UTF-8', pretty_print=True))
        self.output_file.flush()