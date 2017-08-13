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
            if entry.name == '/URI':
               m = re.search("(mailto|telnet|news|nntp|snews)\x3A[^\n]*\x25[^\n]*\x22\x2Ecmd", 
                     str(entry), re.IGNORECASE | re.MULTILINE | re.DOTALL)
      
               if m:
                  alert("[CVE-2007-3896] Microsoft Windows ShellExecute and IE7 url handling code execution attempt in object %s" % self.obj.id, str(entry))
      except:
         pass
