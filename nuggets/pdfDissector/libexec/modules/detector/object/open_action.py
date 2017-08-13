import struct
import re
import array

from java.lang import String
from java.util import Arrays
from modules.utils import *

class VulnerabilityCheck():
   def __init__(self, pdf, obj, data, decodedData=None):
      self.pdf = pdf
      self.obj = obj
      self.data = data
      self.decodedData = decodedData

   def check(self):
      try:
         for entry in self.obj.content.entries:
            if entry.name == '/OpenAction':
               if re.search("/(JavaScript|JS|Launch)", str(entry.value), re.IGNORECASE | re.MULTILINE | re.DOTALL):
                  alert("OpenAction usage found in object %s -> %s" % (self.obj.id, entry.value), str(entry.value))
      except:
         pass
