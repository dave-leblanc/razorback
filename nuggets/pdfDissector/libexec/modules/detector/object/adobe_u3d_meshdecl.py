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
               h += 4      # Skip the 4 bytes of the field we just found
               h += 8      # Skip our 8 bytes of header
               h += struct.unpack_from("<H", self.decodedData, h)[0] # Read in 2 bytes of size
               h += 2      # Jump the two bytes of size we just read
               h += 32     # Jump the rest of the header

               # Store the position count for later comparison
               shading_count = struct.unpack_from("<I", self.decodedData, h)[0]

               # Finally check the shading count size
               if shading_count >= 0x05d1745e:
                  alert("[CVE-2010-0196] Adobe Acrobat Reader U3D CLODMeshDeclaration code execution attempt in object %s" % self.obj.id, '')
