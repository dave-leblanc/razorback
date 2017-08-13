import struct

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
            for entry in self.obj.content.getEntries():
               if str(entry.name) == '/Filter':
                  if str(entry.value).find('JBIG2Decode') > 0:
                     if struct.unpack_from("B", self.decodedData, 4)[0] & 64:
                        if struct.unpack_from("B", self.decodedData, 5)[0] < 160:
                           if struct.unpack_from("<I", self.decodedData, 6) > 35256:
                              alert("[CVE-2009-0658] Attempted JBIG2Decode exploit in object %s" % self.obj.id, '')
                              break
      except:
         pass
