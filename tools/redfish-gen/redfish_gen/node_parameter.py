from .globals import __BASE_PATH__
from .property import EntityCondition

class NodeParameter():
    @staticmethod
    def build(parameters_definition):
        p_type = parameters_definition["Type"]
        p_class = {
            "Static": StaticNodeParameter,
            "Entity": EntityNodeParameter,
        }.get(p_type)
        return p_class(parameters_definition)
    
    def __init__(self, definition):
        self._definition = definition
    
    def name(self):
        return self._definition["Parameter"]
    
    def value(self):
        return self._definition["Value"]
    
    def type(self):
        return self._definition["Type"]
    
class StaticNodeParameter(NodeParameter):
    def __init__(self, definition):
        super(StaticNodeParameter, self).__init__(definition)
        
class EntityNodeParameter(NodeParameter):        
    def __init__(self, definition):
        super(EntityNodeParameter, self).__init__(definition)
        self._conditions = definition['Conditions'] if 'Conditions' in definition else []
        
    def source(self):
        return self._definition["Source"]
    
    def conditions(self):
        return [EntityCondition(condtition) for condtition in self._conditions]
