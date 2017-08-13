import sys
import array
import os
import struct

from com.zynamics.pdf.api import Interpreter
from java.lang import String

def alert(msg, report=""):
    
   print "ALERT"
   print msg
   if len(report) != 0:
      print "REPORT"
      print len(report)
      print report

   print "ENDALERT"

def subBlock(data, type=""):
   if len(data) != 0:
      print "DATA"
      print type
      print len(data)
      print data
   else:
      print "Alert has no data"

def endBlock():
      print "ENDDATA"
    
def getJavaScript(pdf, obj, zynamics_path):
   result = None

   try:
      result = Interpreter.interpret(pdf, String(obj.stream.decodedData), getEmulatorScript(zynamics_path))
   except:
      pass

   return result

def unescape(data):
   return ''.join([(chr(int(chunk[0:2], 16)) + chr(int(chunk[2:4], 16))) for chunk in data.split('%u')[1:]])

def getEmulatorScript(zynamics_path):
   script_path = ("%s/emulator/emulator.js" % zynamics_path)

   return open(script_path, "r").read()

def convertUnicodeToUchar(data):
   try:
      return ''.join([struct.pack('H', ord(i)) for i in data])
   except:
      return None
      
def convertToUchar(data):
   try:
      return ''.join([chr(i & 0xff) for i in data])
   except:
      return None


def getModules(subdir):
   detector_path = 'modules/detector'
   modules = []

   for module in os.listdir("%s/%s" % (detector_path, subdir)):
      if module.endswith('.py'):
         if module != '__init__.py':
            path = ("%s/%s/%s" % (detector_path, subdir, module)).rstrip('.py').replace('/', '.')
            m = __import__(path, globals(), locals(), ['VulnerabilityCheck'], -1)
            modules.append(m)

   return modules

def checkHeapSpray(data, length=1000):
   try:
      f = ""
      s = ""

      for ctr in range(0, length, 4):
         f = data[ctr:(ctr + 4)]
         s = data[(ctr + 4):(ctr + 8)]

         if f == s:
            continue
         else:
            raise

      return f

   except:
      return None

