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
         for entry in self.obj.content.getEntries():
            if re.search("/Colors.*1073741838", str(entry.value), re.IGNORECASE | re.MULTILINE | re.DOTALL):
               alert("[CVE-2009-3459] Adobe Reader malformed FlateDecode colors declaration in object %s" % self.obj.id, str(entry.value))
            elif re.search("/DecodeParms\s*\[[^\]]*Colors\s*\d\d\d\d", str(entry.value), re.IGNORECASE | re.MULTILINE | re.DOTALL): 
               alert("[CVE-2009-3459] Adobe Reader malformed FlateDecode colors declaration in object %s" % self.obj.id, str(entry.value))

      except:
         pass
