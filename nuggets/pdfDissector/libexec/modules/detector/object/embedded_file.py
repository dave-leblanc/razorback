import pefile

from java.lang import String
from modules.utils import *

class VulnerabilityCheck():
   def __init__(self, pdf, obj, data, decodedData=None):
      self.pdf = pdf
      self.obj = obj
      self.data = data
      self.decodedData = decodedData

   def check(self):
      try:
         if self.decodedData:
            pe = pefile.PE(data=self.decodedData, fast_load=False)

            if(pe):
               alert("Embedded executable file detected in object %s" % self.obj.id, pe.dump_info())
               subBlock(self.decodedData, "PE_FILE")

      except pefile.PEFormatError:
         pass

