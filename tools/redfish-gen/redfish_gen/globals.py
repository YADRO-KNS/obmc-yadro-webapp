import os

import redfish_gen.generator as gen

__BASE_PATH__ = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(gen.__file__))))
__RFG_PATH__ = __BASE_PATH__ + "/tools/redfish-gen/"
