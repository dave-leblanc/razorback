import sys
import re
import string
import itertools
import struct
import traceback

from com.zynamics.pdf.api import Interpreter
from java.lang import String
from modules.utils import *

class VulnerabilityCheck():
   def __init__(self, pdf, obj, result, data, decodedData=None):
      self.pdf = pdf
      self.obj = obj
      self.result = result
      self.data = data
      self.decodedData = decodedData

      self.debug                   = True
      self.alert_on_javascript     = True
      self.unescape_minimum_size   = 200
      self.shellcode_minimum_size  = 200

   def check(self):
      try:
         # Send an alert for the embedded javascriptA

         if self.alert_on_javascript:
            alert('Embedded javascript detected in object %d' % (int(self.obj.id)), self.decodedData);
            
         # Search for possible shellcode variables
         for variable_name in self.result.variables:

            if type(self.result.variables[variable_name]).__name__ == 'unicode':
               d = convertUnicodeToUchar(self.result.variables[variable_name])

               if d and len(d) >= self.shellcode_minimum_size:
                  alert("Possible shellcode detected in variable %s in object %s" % (variable_name, self.obj.id), '')
                  subBlock(d, "SHELL_CODE")
                  endBlock()
                  # Check for possible heapspray in the variable
                  for offset in range(0, 3):
                     address = checkHeapSpray(d[offset:], 400)
                     
                     if address:
                        alert('Attempted heap spray detected in self.object %s variable %s pointing to 0x%08x' % 
                           (self.obj.id, variable_name, struct.unpack_from("<L", address)[0]), '')

                        break

         # Parse the logs
         for log_name in self.result.logs:

            # Unescape() strings
            if log_name == 'Unescape':
               for un in self.result.logs[log_name]:

                  # Check for shellcode as well
                  sc = unescape(un)

                  # Do we want to alert on this?
                  if len(sc) > self.unescape_minimum_size:
                     short = ("Javascript unescape() usage\n" +
                        "Object: %d\n" +
                        "Length: %d\n" +
                        "Detection: unescape() function call was captured by the javascript emulator\n") % (int(self.obj.id), len(un))

                     alert("Javascript unescape() usage found", short)

            # Debug messages
            if log_name == 'Debug':
               for dbg in self.result.logs[log_name]:
                  if self.debug:
                     print dbg

            # Check for Eval statements
            if log_name == 'Eval':
               for e in self.result.logs[log_name]:

                  alert("JavaScript eval() of %d bytes detected in object %s" % (len(e), self.obj.id), '')

            # Finally check for the javascript exploit checks
            if log_name == 'Exploit':
               for exploit in self.result.logs[log_name]:
                  alert("%s in object %s" % (exploit.rstrip('\n'), self.obj.id), '')

      except Exception, e:
         traceback.print_exc(file=sys.stdout)
