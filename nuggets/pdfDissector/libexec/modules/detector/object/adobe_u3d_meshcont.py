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
      if self.decodedData:
         for m in re.finditer("U3D\x00", self.decodedData, re.IGNORECASE | re.MULTILINE | re.DOTALL):
            h = self.decodedData.find("\x31\xff\xff\xff", m.end())

            if h >= 0:
               h += 4      # Skip our 4 matching bytes
               h += 8      # Skip 8 bytes of header 
               h += struct.unpack_from("<H", self.decodedData, h)[0] # Read in 2 bytes of size
               h += 2      # Jump the 2 bytes of size 
               h += 12     # Jump the rest of the header

               # Store the position count for later comparison
               position_count = struct.unpack_from("<I", self.decodedData, h)[0]

               # Find the next header
               h = self.decodedData.find("\x3c\xff\xff\xff", h)
               
               if h > 0:
                  h += 4      # Skip our 4 matching bytes
                  h += 8      # Skip 8 bytes of header
                  h += struct.unpack_from("<H", self.decodedData, h)[0] # Read in 2 bytes of size
                  h += 2      # Jump the 2 bytes of size 
                  h += 12     # Jump the rest of the header

                  # Final piece for our comparison
                  resolution_update = struct.unpack_from("<I", self.decodedData, h)[0]
                  
                  if(resolution_update > position_count):
                     alert("[CVE-2009-2990] Adobe Acrobat Reader U3D CLODMeshContinuation code execution attempt in object %s" % self.obj.id, '')
